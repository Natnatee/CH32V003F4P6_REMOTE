#pragma once

#include "ch32fun.h"
#include <stdint.h>

// ขาสำหรับต่อหลอดส่งสัญญาณอินฟราเรด IR LED (PD4)
#define IR_TX_PIN   4

// เริ่มต้นขา PD4 สำหรับส่งสัญญาณ IR (Output Push-Pull 10MHz)
void ir_send_init(void);

// ส่งสัญญาณอินฟราเรด 32-bit (NEC / Samsung Protocol) ความถี่ 38kHz ออกขา PD4
void ir_send_code(uint32_t code);
