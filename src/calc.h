#pragma once

#include <stdint.h>

// เริ่มต้นเครื่องคิดเลข
void calc_init(void);

// ส่งสัญญาณปุ่มกดเข้าสู่เครื่องคิดเลข (key: 1..16)
void calc_handle_key(uint8_t key);

// วาดภาพหน้าจอเครื่องคิดเลข OLED
void calc_render(void);

// ตรวจสอบว่าต้องการออกจากโหมดเครื่องคิดเลขหรือไม่ (เมื่อกด 16)
uint8_t calc_should_exit(void);
