#pragma once

#include "ch32fun.h"
#include <stdint.h>

// ขาสำหรับต่อเซนเซอร์รับสัญญาณ IR VS1838B (PD0)
#define IR_RX_PIN   0

// เริ่มต้นขา PD0 สำหรับรับสัญญาณ IR
void ir_recv_init(void);

// ตรวจจับและถอดรหัสสัญญาณ IR (Non-blocking / Polling)
// คืนค่า 1 เมื่อได้รับโค้ดสมบูรณ์ พร้อมเก็บรหัส 32-bit ลงใน *code_out
// คืนค่า 0 เมื่อไม่มีสัญญาณ
uint8_t ir_recv_poll(uint32_t *code_out);
