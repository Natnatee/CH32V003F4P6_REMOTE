#include "ir_recv.h"
#include <stdio.h>

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
    // ถ้าขา PD0 ยังเป็น HIGH แสดงว่ายังไม่มีสัญญาณเข้ามา -> คืนค่า 0 ทันที ไม่บล็อกระบบ
    if ((GPIOD->INDR & (1 << IR_RX_PIN)) != 0) {
        return 0;
    }

    // 1. วัด Leader Pulse (ช่วง LOW แรก)
    uint32_t leader_low = measure_pulse(0, 15000);
    if (leader_low < 2000) {
        return 0; // สัญญาณรบกวนสั้นเกินไป
    }

    // 2. วัด Leader Space (ช่วง HIGH ถัดมา)
    uint32_t leader_high = measure_pulse(1, 8000);
    if (leader_high < 1500) {
        return 0;
    }

    // 3. ถอดรหัสบิตข้อมูล 32 บิต (NEC / Samsung / Universal)
    uint32_t data = 0;
    uint8_t bits_received = 0;

    for (int i = 0; i < 32; i++) {
        uint32_t mark = measure_pulse(0, 2500);
        if (mark < 150) break;

        uint32_t space = measure_pulse(1, 3500);
        if (space == 0) break;

        bits_received++;

        // มาตรฐาน NEC: ช่วง Space ยาว ~1680µs คือ บิต 1, ช่วง Space สั้น ~560µs คือ บิต 0
        if (space > 1000) {
            data |= (1UL << (31 - i));
        }
    }

    // ถ้าได้รับข้อมูลครบ 32 บิต หรือมากกว่า 16 บิตขึ้นไป
    if (bits_received >= 16 && data != 0) {
        if (code_out) *code_out = data;
        return 1;
    }

    return 0;
}
