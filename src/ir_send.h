#pragma once

#include "ch32fun.h"
#include <stdint.h>

// ขาสำหรับต่อหลอดส่งสัญญาณอินฟราเรด IR LED (PD4)
#define IR_TX_PIN   4

// เริ่มต้นขา PD4 สำหรับส่งสัญญาณ IR (Output Push-Pull 10MHz)
void ir_send_init(void);

// ส่งสัญญาณอินฟราเรด 32-bit (NEC / Samsung Protocol) ความถี่ 38kHz ออกขา PD4
void ir_send_code(uint32_t code);
void ir_send_nec(uint32_t code);
void ir_send_nec_repeat(void);
void ir_send_samsung(uint32_t code);
void ir_send_lg(uint32_t code);
void ir_send_sony(uint32_t code, uint8_t bits);
void ir_send_jvc(uint16_t code);
void ir_send_sharp(uint16_t raw_code);
