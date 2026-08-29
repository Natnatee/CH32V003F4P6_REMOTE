#pragma once

#include <stdint.h>

// เริ่มต้นโมดูล INA226 และตัวแปรวัดไฟ
void meter_init(void);

// อัปเดตการอ่านค่าจากเซนเซอร์ INA226 (เรียกจาก Main Loop)
void meter_update(void);

// จัดการปุ่มกดในโหมด MULTI (4/8/12: สลับหน้าระหว่าง REALTIME กับ PEAK, 13: Reset, 16: Exit)
void meter_handle_key(uint8_t key);

// วาดภาพหน้าจอ MULTI Meter บน OLED 64x128
void meter_render(void);

// ตรวจสอบว่าต้องการออกจากโหมดหรือไม่ (เมื่อกด 16)
uint8_t meter_should_exit(void);
