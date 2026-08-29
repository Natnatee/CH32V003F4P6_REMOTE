#include "ir_send.h"
#include <stdio.h>

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

    printf("[IR TX] Transmitting 38kHz Code: 0x%08lX...\r\n", code);

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
