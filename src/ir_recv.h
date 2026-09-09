#pragma once

#include "ch32fun.h"
#include <stdint.h>

// ขาสำหรับต่อเซนเซอร์รับสัญญาณ IR VS1838B (PD0)
#define IR_RX_PIN   0
#define IR_PROTOCOL_SHARP 22
#define IR_SHARP_BITS 15
#define IR_PROTOCOL_NEC 7
#define IR_PROTOCOL_SAMSUNG 19
#define IR_PROTOCOL_JVC 5
#define IR_PROTOCOL_LG 6
#define IR_PROTOCOL_SONY 23

// เริ่มต้นขา PD0 สำหรับรับสัญญาณ IR
void ir_recv_init(void);

// ตรวจจับเฟรม IR แบบ polling
// จะพิมพ์ raw pulse/space ของทุกเฟรมผ่าน SDI printf แยกกัน
// คืนค่า 1 เฉพาะเมื่อถอดรหัสแบบ NEC ได้ พร้อมเก็บรหัส 32-bit ลงใน *code_out
// เฟรม protocol อื่นยังถูกพิมพ์เป็น raw แม้ฟังก์ชันจะคืนค่า 0
// คืนค่า 0 เมื่อไม่มีสัญญาณ
uint8_t ir_recv_poll(uint32_t *code_out);
uint8_t ir_recv_last_protocol(void);
uint8_t ir_recv_last_bits(void);
