#pragma once

#include <stdint.h>

// เริ่มต้นเกมใหม่
void tetris_init(void);

// อัปเดตลูปเกม (เรียกทุก 20ms จาก main loop)
void tetris_update(void);

// ส่งสัญญาณปุ่มกดเข้าสู่เกม (key: 1..16, is_new: 1 เมื่อเพิ่งกด, 0 เมื่อกดแช่)
void tetris_handle_key(uint8_t key, uint8_t is_new);

// วาดภาพหน้าจอเกม Tetris
void tetris_render(void);

// ตรวจสอบว่าต้องการออกจากเกมหรือไม่ (เมื่อกด 16)
uint8_t tetris_should_exit(void);
