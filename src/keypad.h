#pragma once

#include "ch32fun.h"
#include <stdint.h>

// 4x4 Keypad Initialization (ใช้ 8 พินเรียงฝั่งซ้ายของบอร์ด CH32V003)
// Row 1..4 = PD6, PA1, PA2, PC0
// Col 1..4 = PC1, PC2, PC3, PC4
void keypad_init(void);

// Scan Keypad: คืนค่าหมายเลขปุ่ม 1..16 (หรือ 0 ถ้าไม่ได้กด)
// พร้อมส่งออกแถว (row 1..4) และหลัก (col 1..4)
uint8_t keypad_scan(uint8_t *row, uint8_t *col);
