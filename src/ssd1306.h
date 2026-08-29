#pragma once

#include "ch32fun.h"
#include <stdint.h>

// SSD1306 I2C 7-bit Address = 0x3C -> 8-bit Address = 0x78
#define SSD1306_I2C_ADDR        0x78

// Pin Config: PC6 = SCL, PC7 = SDA (อยู่ฝั่งขวาล่างของบอร์ด)
#define SSD1306_SCL_PIN_POS     6
#define SSD1306_SDA_PIN_POS     7

// Hardware init & clear
void ssd1306_init(void);
void ssd1306_clear(void);

// Portrait Mode Framebuffer Functions (64x128 pixels)
void oled_clear_buffer(void);
void oled_draw_pixel(int16_t x, int16_t y, uint8_t color);
void oled_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void oled_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void oled_draw_char(int16_t x, int16_t y, char c, uint8_t color);
void oled_draw_str(int16_t x, int16_t y, const char *str, uint8_t color);
void oled_draw_char_large(int16_t x, int16_t y, char c);
void oled_draw_header_title(int16_t y, const char *str);

// แปลงตัวเลข 32-bit เป็น Hex String (เช่น 0x20DF10EF)
void hex_to_str(uint32_t val, char *out);

// ฟังก์ชันเรนเดอร์หน้าจอ 3 ส่วน: บน (Header) / กลาง (ตาราง 3x4 พร้อมสถานะช่องจำ/ว่าง) / ล่าง (Footer)
void oled_render_grid_screen(const char *header, const char *footer, uint8_t active_key, uint8_t blink_state, const uint32_t *profile_codes);

// ฟังก์ชันเรนเดอร์หน้าจอเมื่อรับรหัส IR ได้ในโหมด LEARN
void oled_render_ir_captured_screen(const char *header, uint32_t ir_code);

// ฟังก์ชันเรนเดอร์หน้าจอเมื่อกวาดยิงรหัสในโหมด NEW (แสดงลำดับ รหัส และวิธีเซฟ)
void oled_render_new_code_screen(const char *header, uint32_t code, uint16_t cur_idx, uint16_t total_count);

void oled_update(void);
