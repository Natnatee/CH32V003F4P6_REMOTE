#include "keypad.h"

// Row Pins (Output Push-Pull):
// Row 1 = PD6
// Row 2 = PA1
// Row 3 = PA2
// Row 4 = PC0

// Col Pins (Input Pull-up):
// Col 1 = PC1
// Col 2 = PC2
// Col 3 = PC3
// Col 4 = PC4

static inline void delay_us(uint32_t count) {
    Delay_Us(count);
}

void keypad_init(void) {
    // 1. เปิด Clock ให้ GPIOA, GPIOC, GPIOD
    RCC->APB2PCENR |= (RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD);

    // 2. ตั้งค่า Rows เป็น Output Push-Pull (10MHz)
    // Row 1: PD6
    GPIOD->CFGLR &= ~(0xf << (4 * 6));
    GPIOD->CFGLR |=  (0x1 << (4 * 6));
    GPIOD->BSHR = (1 << 6); // default HIGH

    // Row 2: PA1
    GPIOA->CFGLR &= ~(0xf << (4 * 1));
    GPIOA->CFGLR |=  (0x1 << (4 * 1));
    GPIOA->BSHR = (1 << 1); // default HIGH

    // Row 3: PA2
    GPIOA->CFGLR &= ~(0xf << (4 * 2));
    GPIOA->CFGLR |=  (0x1 << (4 * 2));
    GPIOA->BSHR = (1 << 2); // default HIGH

    // Row 4: PC0
    GPIOC->CFGLR &= ~(0xf << (4 * 0));
    GPIOC->CFGLR |=  (0x1 << (4 * 0));
    GPIOC->BSHR = (1 << 0); // default HIGH

    // 3. ตั้งค่า Cols (PC1, PC2, PC3, PC4) เป็น Input Pull-up
    for (int pin = 1; pin <= 4; pin++) {
        GPIOC->CFGLR &= ~(0xf << (4 * pin));
        GPIOC->CFGLR |=  (0x8 << (4 * pin)); // 0x8 = Input with Pull-Up/Pull-Down
        GPIOC->BSHR = (1 << pin);            // BSHR = 1 เพื่อเปิด Pull-Up
    }
}

static inline void set_row(uint8_t r, uint8_t level) {
    if (level) {
        // High
        switch(r) {
            case 0: GPIOD->BSHR = (1 << 6); break; // Row 1: PD6
            case 1: GPIOA->BSHR = (1 << 1); break; // Row 2: PA1
            case 2: GPIOA->BSHR = (1 << 2); break; // Row 3: PA2
            case 3: GPIOC->BSHR = (1 << 0); break; // Row 4: PC0
        }
    } else {
        // Low
        switch(r) {
            case 0: GPIOD->BCR = (1 << 6); break; // Row 1: PD6
            case 1: GPIOA->BCR = (1 << 1); break; // Row 2: PA1
            case 2: GPIOA->BCR = (1 << 2); break; // Row 3: PA2
            case 3: GPIOC->BCR = (1 << 0); break; // Row 4: PC0
        }
    }
}

static inline uint8_t read_col(uint8_t c) {
    switch (c) {
        case 0: return (GPIOC->INDR & (1 << 1)) ? 1 : 0; // Col 1: PC1
        case 1: return (GPIOC->INDR & (1 << 2)) ? 1 : 0; // Col 2: PC2
        case 2: return (GPIOC->INDR & (1 << 3)) ? 1 : 0; // Col 3: PC3
        case 3: return (GPIOC->INDR & (1 << 4)) ? 1 : 0; // Col 4: PC4
        default: return 1;
    }
}

uint8_t keypad_scan(uint8_t *out_row, uint8_t *out_col) {
    // ให้ทุก Row เป็น HIGH ก่อน
    for (int r = 0; r < 4; r++) {
        set_row(r, 1);
    }
    delay_us(5);

    for (uint8_t r = 0; r < 4; r++) {
        // ดึง Row ปัจจุบันลง LOW (0V)
        set_row(r, 0);
        delay_us(10); // หน่วงเวลาให้สัญญาณนิ่ง

        // ตรวจสอบทั้ง 4 Columns
        for (uint8_t c = 0; c < 4; c++) {
            if (read_col(c) == 0) { // เจอขาลอจิก LOW = มีการกดปุ่ม
                // ดึง Row กลับเป็น HIGH
                set_row(r, 1);

                if (out_row) *out_row = r + 1;
                if (out_col) *out_col = c + 1;

                // คำนวณรหัสปุ่ม S1 .. S16
                return (r * 4 + c + 1);
            }
        }

        // ดึง Row กลับเป็น HIGH
        set_row(r, 1);
    }

    if (out_row) *out_row = 0;
    if (out_col) *out_col = 0;
    return 0; // ไม่มีการกดปุ่ม
}
