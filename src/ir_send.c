#include "ir_send.h"

#ifndef IR_DEBUG_LOG
#define IR_DEBUG_LOG 0
#endif
#if IR_DEBUG_LOG
#include <stdio.h>
#define IR_LOG(...) printf(__VA_ARGS__)
#else
#define IR_LOG(...) ((void)0)
#endif

void ir_send_init(void) {
    // 1. เปิด Clock ให้ GPIOD
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;

    // 2. กำหนดขา PD4 เป็น Output Push-Pull 10MHz (0x1)
    GPIOD->CFGLR &= ~(0xf << (4 * IR_TX_PIN));
    GPIOD->CFGLR |=  (0x1 << (4 * IR_TX_PIN));
    GPIOD->BCR = (1 << IR_TX_PIN); // ค่าเริ่มต้น LOW
}

// ยิงคลื่นพาหะ 38kHz (เปิด 13µs / ปิด 13µs) เป็นเวลา duration_us ไมโครวินาที
static inline void ir_carrier_burst(uint32_t duration_us) {
    uint32_t start = SysTick->CNT;
    uint32_t ticks = duration_us * 6; // 6 ticks ต่อ 1 µs

    while ((SysTick->CNT - start) < ticks) {
        GPIOD->BSHR = (1 << IR_TX_PIN); // HIGH
        Delay_Us(13);
        GPIOD->BCR  = (1 << IR_TX_PIN); // LOW
        Delay_Us(13);
    }
    GPIOD->BCR = (1 << IR_TX_PIN); // ดับไฟเสมอเมื่อจบ
}

// เว้นช่วงว่าง (Space / Silent)
static inline void ir_space(uint32_t duration_us) {
    GPIOD->BCR = (1 << IR_TX_PIN);
    Delay_Us(duration_us);
}

// ส่งสัญญาณอินฟราเรด 32-bit (NEC / Samsung Protocol)
void ir_send_code(uint32_t code) {
    if (code == 0) return;

    // 1. Leader Pulse: 9000µs Burst + 4500µs Space
    ir_carrier_burst(9000);
    ir_space(4500);

    // 2. ส่ง 32 Data Bits (MSB first)
    for (int i = 0; i < 32; i++) {
        ir_carrier_burst(560);
        if (code & (1UL << (31 - i))) {
            ir_space(1680); // บิต 1 = Space 1680µs
        } else {
            ir_space(560);  // บิต 0 = Space 560µs
        }
    }

    // 3. Stop Bit: 560µs Burst
    ir_carrier_burst(560);
    ir_space(1000);
}

// NEC standard: leader 9000/4500us, 32 bits LSB-first
void ir_send_nec(uint32_t code) {
    if (code == 0) return;
    IR_LOG("TX NEC %08lX\r\n", code);

    ir_carrier_burst(9000);
    ir_space(4500);
    for (uint8_t bit = 0; bit < 32; bit++) {
        ir_carrier_burst(560);
        ir_space((code & (1UL << bit)) ? 1690 : 560);
    }
    ir_carrier_burst(560);
    ir_space(40000);
}

void ir_send_nec_repeat(void) {
    IR_LOG("TX NEC REPEAT\r\n");
    ir_carrier_burst(9000);
    ir_space(2250);
    ir_carrier_burst(560);
    ir_space(40000);
}

// Samsung: leader 4500/4500us, 32 bits LSB-first
void ir_send_samsung(uint32_t code) {
    if (code == 0) return;
    IR_LOG("TX SAMSUNG %08lX\r\n", code);

    ir_carrier_burst(4500);
    ir_space(4500);
    for (uint8_t bit = 0; bit < 32; bit++) {
        ir_carrier_burst(560);
        ir_space((code & (1UL << bit)) ? 1690 : 560);
    }
    ir_carrier_burst(560);
    ir_space(40000);
}

void ir_send_lg(uint32_t code) {
    if (code == 0) return;
    IR_LOG("TX LG %08lX\r\n", code);
    ir_carrier_burst(9000);
    ir_space(4500);
    for (uint8_t bit = 0; bit < 28; bit++) {
        ir_carrier_burst(560);
        ir_space((code & (1UL << bit)) ? 1690 : 560);
    }
    ir_carrier_burst(560);
    ir_space(40000);
}

void ir_send_sony(uint32_t code, uint8_t bits) {
    if (code == 0 || (bits != 12 && bits != 15 && bits != 20)) return;
    IR_LOG("TX SONY %08lX/%u\r\n", code, bits);
    ir_carrier_burst(2400);
    ir_space(600);
    for (uint8_t bit = 0; bit < bits; bit++) {
        ir_carrier_burst((code & (1UL << bit)) ? 1200 : 600);
        ir_space(600);
    }
    ir_space(25000);
}

void ir_send_jvc(uint16_t code) {
    IR_LOG("TX JVC %04X\r\n", code);
    ir_carrier_burst(8400);
    ir_space(4200);
    for (uint8_t bit = 0; bit < 16; bit++) {
        ir_carrier_burst(525);
        ir_space((code & (1U << bit)) ? 1575 : 525);
    }
    ir_carrier_burst(525);
    ir_space(45000);
}

static void ir_send_sharp_frame(uint16_t raw_code) {
    for (uint8_t bit = 0; bit < 15; bit++) {
        ir_carrier_burst(260);
        ir_space((raw_code & (1U << bit)) ? 1820 : 780);
    }
    ir_carrier_burst(260);
}

// Sharp/Denon: normal -> inverted -> normal, gap 45ms, LSB-first
void ir_send_sharp(uint16_t raw_code) {
    uint16_t inverted = raw_code ^ 0x7FE0;
    IR_LOG("TX SHARP %04X\r\n", raw_code);

    ir_send_sharp_frame(raw_code);
    ir_space(45000);
    ir_send_sharp_frame(inverted);
    ir_space(45000);
    ir_send_sharp_frame(raw_code);
}
