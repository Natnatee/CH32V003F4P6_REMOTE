#pragma once

#include <stdint.h>

// เริ่มต้นเกม Flappy Bird
void flappy_init(void);

// อัปเดตฟิสิกส์เกม (เรียกทุก 20ms จาก main loop)
void flappy_update(void);

// ส่งสัญญาณปุ่มกดเข้าสู่เกม (key: 1..16, is_new: 1 เมื่อเพิ่งกด)
void flappy_handle_key(uint8_t key, uint8_t is_new);

// วาดภาพหน้าจอเกม Flappy Bird
void flappy_render(void);

// ตรวจสอบว่าต้องการออกจากเกมหรือไม่ (เมื่อกด 16)
uint8_t flappy_should_exit(void);
