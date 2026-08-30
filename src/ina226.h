#pragma once

#include "ch32fun.h"
#include <stdint.h>

// เริ่มต้นฮาร์ดแวร์ INA226
uint8_t ina226_init(void);

// อัปเดตอ่านค่า Voltage / Current และคำนวณ Peak / 5s Average
void ina226_update(void);

// เรนเดอร์หน้าจอ Multimeter (REAL TIME / PEAK / AVG 5s)
void ina226_render(void);

// รับและประมวลผลปุ่มกดในโหมด Multimeter
void ina226_handle_key(uint8_t key);

// เช็คสถานะการออกจากโหมด
uint8_t ina226_should_exit(void);
