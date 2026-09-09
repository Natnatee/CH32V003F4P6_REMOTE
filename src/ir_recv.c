#include "ir_recv.h"
#include <stdio.h>

#define IR_RAW_MAX_PULSES 96
#define IR_RAW_LOW_TIMEOUT_US 12000
#define IR_RAW_HIGH_TIMEOUT_US 20000
#define IR_SYNC_IDLE_US 8000
#define IR_SYNC_TIMEOUT_US 80000

static uint16_t raw_pulses[IR_RAW_MAX_PULSES];
static uint16_t raw_frame_number;
static uint8_t last_protocol;
static uint8_t last_bits;

static inline uint8_t ir_rx_level(void) {
    return (GPIOD->INDR & (1 << IR_RX_PIN)) ? 1 : 0;
}

// รอ HIGH ที่ยาวพอจะเป็นช่วงว่างระหว่างเฟรม แล้วรอขอบตกแรกของเฟรมถัดไป
// ทำให้การอ่านไม่เริ่มกลางเฟรม แม้ ir_recv_poll() จะถูกเรียกช้ากว่าจังหวะเริ่มจริง
static uint8_t wait_for_frame_start(void) {
    uint32_t overall_start = SysTick->CNT;
    uint32_t overall_timeout = IR_SYNC_TIMEOUT_US * 6;

    while ((SysTick->CNT - overall_start) < overall_timeout) {
        // ถ้าเริ่มถูกเรียกกลาง mark ให้รอจนกลับไป HIGH ก่อน
        while (!ir_rx_level()) {
            if ((SysTick->CNT - overall_start) >= overall_timeout) return 0;
        }

        // HIGH สั้น ๆ คือ space ภายในเฟรม ให้ข้ามไปจนเจอช่วง idle จริง
        uint32_t high_start = SysTick->CNT;
        while (ir_rx_level()) {
            if ((SysTick->CNT - overall_start) >= overall_timeout) return 0;
            if ((SysTick->CNT - high_start) >= (IR_SYNC_IDLE_US * 6)) {
                break;
            }
        }

        // HIGH ยาวครบเกณฑ์แล้ว: รอ mark แรกของเฟรมใหม่
        if (ir_rx_level()) {
            while (ir_rx_level()) {
                if ((SysTick->CNT - overall_start) >= overall_timeout) return 0;
            }
            return 1;
        }
    }

    return 0;
}

void ir_recv_init(void) {
    // 1. เปิด Clock ให้ GPIOD
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;

    // 2. ตั้งค่า PD0 เป็น Input พร้อมเปิด Pull-Up (0x8)
    GPIOD->CFGLR &= ~(0xf << (4 * IR_RX_PIN));
    GPIOD->CFGLR |=  (0x8 << (4 * IR_RX_PIN));
    GPIOD->BSHR = (1 << IR_RX_PIN); // Pull-Up
}

// วัดความยาวช่วงเวลาของลอจิก (target_level = 0 หรือ 1) ในหน่วยไมโครวินาที (µs)
static inline uint32_t measure_pulse(uint8_t target_level, uint32_t timeout_us) {
    uint32_t start = SysTick->CNT;
    // ที่ Core Clock 48MHz และ SysTick หาร 8 -> 1 ไมโครวินาที = 6 ticks
    uint32_t timeout_ticks = timeout_us * 6;

    while (((GPIOD->INDR & (1 << IR_RX_PIN)) ? 1 : 0) == target_level) {
        if ((SysTick->CNT - start) > timeout_ticks) {
            return 0; // หมดเวลา (Timeout)
        }
    }
    return (SysTick->CNT - start) / 6;
}

uint8_t ir_recv_poll(uint32_t *code_out) {
    // ซิงก์จากช่วง idle ก่อนเริ่มเก็บ frame เพื่อไม่อ่านกลางสัญญาณ
    if (!wait_for_frame_start()) return 0;

    // เก็บทุกช่วง LOW/HIGH เป็นหนึ่งเฟรมก่อนตัดสิน protocol
    uint8_t pulse_count = 0;
    while (pulse_count < IR_RAW_MAX_PULSES) {
        uint8_t level = pulse_count & 1; // IR receiver idle HIGH; frame เริ่ม LOW
        uint32_t timeout_us = level ? IR_RAW_HIGH_TIMEOUT_US : IR_RAW_LOW_TIMEOUT_US;
        uint32_t duration = measure_pulse(level, timeout_us);
        if (duration == 0) break;

        raw_pulses[pulse_count++] = (duration > 65535) ? 65535 : (uint16_t)duration;

        // HIGH ยาวคือช่วงว่างหลังจบเฟรม ไม่ควรไปรอจับ noise ต่อ
        if (level && duration > 12000) break;
    }

    if (pulse_count < 3) return 0;

    raw_frame_number++;
    printf("[IR RAW FRAME %u] pulses=%u:", raw_frame_number, pulse_count);
    for (uint8_t i = 0; i < pulse_count; i++) {
        printf(" %u", raw_pulses[i]);
    }
    printf("\r\n");

    // NEC: leader 9000/4500us, 32 bits LSB-first
    if (pulse_count >= 66 && raw_pulses[0] > 7000 && raw_pulses[0] < 11000 &&
        raw_pulses[1] > 3000 && raw_pulses[1] < 6000) {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 32; i++) {
            uint16_t mark = raw_pulses[2 + (i * 2)];
            uint16_t space = raw_pulses[3 + (i * 2)];
            if (mark < 300 || mark > 900 || space == 0) return 0;
            if (space > 1000) data |= (1UL << i);
        }

        last_protocol = IR_PROTOCOL_NEC;
        last_bits = 32;
        printf("[IR NEC] raw=0x%08lX\r\n", data);
        if (code_out) *code_out = data;
        return 1;
    }

    // Samsung: leader 4500/4500us, 32 bits LSB-first
    if (pulse_count >= 66 && raw_pulses[0] > 3500 && raw_pulses[0] < 6000 &&
        raw_pulses[1] > 3500 && raw_pulses[1] < 6000) {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 32; i++) {
            uint16_t mark = raw_pulses[2 + (i * 2)];
            uint16_t space = raw_pulses[3 + (i * 2)];
            if (mark < 300 || mark > 900 || space == 0) return 0;
            if (space > 1000) data |= (1UL << i);
        }

        last_protocol = IR_PROTOCOL_SAMSUNG;
        last_bits = 32;
        printf("[IR SAMSUNG] raw=0x%08lX\r\n", data);
        if (code_out) *code_out = data;
        return 1;
    }

    // LG: leader 9000/4500us, 28 bits LSB-first
    if (pulse_count >= 58 && raw_pulses[0] > 7000 && raw_pulses[0] < 11000 &&
        raw_pulses[1] > 3000 && raw_pulses[1] < 6000) {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 28; i++) {
            uint16_t mark = raw_pulses[2 + (i * 2)];
            uint16_t space = raw_pulses[3 + (i * 2)];
            if (mark < 300 || mark > 900 || space == 0) return 0;
            if (space > 1000) data |= (1UL << i);
        }
        last_protocol = IR_PROTOCOL_LG;
        last_bits = 28;
        printf("[IR LG] raw=0x%08lX\r\n", data);
        if (code_out) *code_out = data;
        return 1;
    }

    // Sony/SIRC: header 2400/600us, pulse-width bits, LSB-first
    if (pulse_count >= 27 && raw_pulses[0] > 1800 && raw_pulses[0] < 3000 &&
        raw_pulses[1] > 300 && raw_pulses[1] < 1000) {
        uint8_t bits = (pulse_count - 3) / 2;
        if (bits != 12 && bits != 15 && bits != 20) return 0;

        uint32_t data = 0;
        for (uint8_t i = 0; i < bits; i++) {
            uint16_t mark = raw_pulses[2 + (i * 2)];
            uint16_t space = raw_pulses[3 + (i * 2)];
            if (mark < 400 || mark > 1500 || space < 300 || space > 1000) return 0;
            if (mark > 900) data |= (1UL << i);
        }
        last_protocol = IR_PROTOCOL_SONY;
        last_bits = bits;
        printf("[IR SONY] raw=0x%08lX bits=%u\r\n", data, bits);
        if (code_out) *code_out = data;
        return 1;
    }

    // JVC: leader 8400/4200us, 16 bits LSB-first
    if (pulse_count >= 34 && raw_pulses[0] > 6500 && raw_pulses[0] < 10000 &&
        raw_pulses[1] > 3000 && raw_pulses[1] < 5500) {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 16; i++) {
            uint16_t mark = raw_pulses[2 + (i * 2)];
            uint16_t space = raw_pulses[3 + (i * 2)];
            if (mark < 300 || mark > 800 || space == 0) return 0;
            if (space > 1000) data |= (1UL << i);
        }
        last_protocol = IR_PROTOCOL_JVC;
        last_bits = 16;
        printf("[IR JVC] raw=0x%04lX\r\n", data);
        if (code_out) *code_out = data;
        return 1;
    }

    // Sharp/Denon: 15 bits, LSB-first, ไม่มี header แยก
    if (pulse_count < 31 || raw_pulses[0] < 150 || raw_pulses[0] > 500) {
        return 0;
    }

    uint32_t data = 0;
    for (uint8_t i = 0; i < 15; i++) {
        uint16_t mark = raw_pulses[i * 2];
        uint16_t space = raw_pulses[(i * 2) + 1];
        if (mark < 150 || space == 0) return 0;

        // Sharp/Denon: Space ~1820us = 1, Space ~780us = 0
        if (space > 1000) {
            data |= (1UL << i);
        }
    }

    uint8_t address = data & 0x1F;
    uint8_t command = (data >> 5) & 0xFF;
    uint8_t frame_marker = (data >> 13) & 0x03;
    printf("[IR SHARP] raw=0x%04lX addr=0x%02X cmd=0x%02X marker=%u\r\n",
           data, address, command, frame_marker);

    if (frame_marker != 1) return 0;
    last_protocol = IR_PROTOCOL_SHARP;
    last_bits = IR_SHARP_BITS;
    if (code_out) *code_out = data;
    return 1;
}

uint8_t ir_recv_last_protocol(void) { return last_protocol; }
uint8_t ir_recv_last_bits(void) { return last_bits; }
