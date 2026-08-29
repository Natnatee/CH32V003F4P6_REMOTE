# CH32V003F4P6 Smart Remote Project

## Overview
โปรเจกต์พัฒนาระบบ **Smart Remote** บนไมโครคอนโทรลเลอร์ **CH32V003F4P6** (TSSOP-20) โดยย้ายโลจิกมาจากบอร์ด PY32F002A:
1. จอแสดงผล 0.96" OLED (SSD1306) โหมดแนวตั้ง Portrait (64x128) แสดง Header `SAMSUNG` + ตัวเลข 7-Segment ขนาดใหญ่ (1-16)
2. สวิตช์ปุ่มกด 4x4 Matrix Keypad (16 ปุ่ม) เสียบพินเรียงแถวยาวฝั่งซ้ายของบอร์ด
3. ระบบ SDI Debug Printf ผ่านสาย SWDIO (`PD1`)

## Hardware
- **MCU**: WCH CH32V003F4P6 (32-bit RISC-V QingKe V2A @ 48MHz, 16KB Flash, 2KB SRAM)
- **Package**: TSSOP-20
- **Display**: 0.96" I2C OLED SSD1306 (128x64 pixels ใช้วาดแบบ Portrait 64x128)
- **Input**: 4x4 Matrix Keypad (16 สวิตช์ปุ่มกด)
- **Programmer**: WCH-LinkE (โหมด RISC-V)

## Pin Map

### 1. Keypad 4x4 (8 พินเรียงแถวยาวฝั่งซ้ายของบอร์ด)
| ขา Keypad | พินบน CH32V003 | โหมดการทำงาน | คำอธิบาย |
|---|---|---|---|
| **Pin 1 (Row 1)** | **`PC0`** | Output Push-Pull | แถวที่ 1 (ปุ่ม S1..S4) |
| **Pin 2 (Row 2)** | **`PA2`** | Output Push-Pull | แถวที่ 2 (ปุ่ม S5..S8) |
| **Pin 3 (Row 3)** | **`PA1`** | Output Push-Pull | แถวที่ 3 (ปุ่ม S9..S12) |
| **Pin 4 (Row 4)** | **`PD6 (RX)`** | Output Push-Pull | แถวที่ 4 (ปุ่ม S13..S16) |
| **Pin 5 (Col 1)** | **`PC1`** | Input Pull-Up | หลักที่ 1 |
| **Pin 6 (Col 2)** | **`PC2`** | Input Pull-Up | หลักที่ 2 |
| **Pin 7 (Col 3)** | **`PC3`** | Input Pull-Up | หลักที่ 3 |
| **Pin 8 (Col 4)** | **`PC4`** | Input Pull-Up | หลักที่ 4 |

### 2. จอ OLED SSD1306 (I2C)
| ขา OLED | พินบน CH32V003 | คำอธิบาย |
|---|---|---|
| **SCL** | **`PC6`** | I2C Clock (Push-Pull Bit-Bang) |
| **SDA** | **`PC7`** | I2C Data (Push-Pull Bit-Bang) |
| **VCC** | **`V (3.3V)`** | ไฟเลี้ยงจอ 3.3V |
| **GND** | **`G (GND)`** | กราวด์ |

### 3. โปรแกรมเมอร์ WCH-LinkE
| ขา WCH-LinkE | พินบน CH32V003 | คำอธิบาย |
|---|---|---|
| **3.3V** | **`V (3.3V)`** | ไฟเลี้ยงระบบ |
| **GND** | **`G (GND)`** | กราวด์ร่วม |
| **SWDIO** | **`PD1 (SWIO)`** | ขาแฟลชและ SDI Debug Monitor |


## Electrical / Safety Notes
- แรงดันไฟฟ้าทำงาน: 3.3V หรือ 5V ผ่านพอร์ต Type-C หรือขา V (3V3)
- ห้ามดึงกระแสเกินสเปก GPIO แต่ละขา (แนะนำต่อผ่านตัวต้านทาน 220Ω - 1kΩ สำหรับ LED)
- ⚠️ ห้ามเขียนโค้ดเปลี่ยนโหมดหรือทับพิน `PD1` (SWIO) เด็ดขาด เพื่อป้องกันบอร์ดถูกล็อกการแฟลช

## Firmware Architecture
- Framework: `ch32v003fun` (เน้นความเร็ว ขนาดเล็ก ควบคุม register โดยตรง)
- Clock: 48MHz (Internal HSI + PLL)
- Debug Console: SDI Debug Printf ผ่านสาย SWDIO (PD1)

## Libraries
| Purpose | Library | Status | Notes |
|---|---|---|---|
| Core Framework | ch32v003fun | Active | Light-weight RISC-V framework |

## Build & Upload
```bash
# คอมไพล์และอัปโหลด
pio run -t upload

# มอนิเตอร์ดู SDI Debug Printf (Git Bash)
/c/Users/natna/.platformio/packages/tool-minichlink/minichlink -T
```

## Current State
- โครงสร้างโปรเจกต์ตั้งต้นเสร็จสมบูรณ์
- เฟิร์มแวร์ทดสอบ Blink LED + SDI Debug Printf พร้อมใช้งาน

## Open Questions
- การกำหนดขาและโมดูลเพิ่มเติมสำหรับงาน Remote (เช่น RF 433MHz, NRF24L01, IR, หรือปุ่ม Matrix)

## Issue Log
- **WLink Open Error**: เกิดจาก PlatformIO ค่าเริ่มต้นใช้ OpenOCD/WLink Driver ซึ่งไม่ตรงกับ WinUSB ของระบบ แก้ไขโดยกำหนด `upload_protocol = minichlink` ใน `platformio.ini`
- **ไฟ LED บนบอร์ดติดค้างขณะเปิด Monitor**: เกิดจากการเปิดคำสั่ง `minichlink -T` ดักฟังข้อมูลค้างไว้ ซึ่งตัวโปรแกรมเมอร์จะจับคู่สาย SWDIO ตลอดเวลา ทำให้ไฟ LED แสดงสถานะติดค้าง และหากจะอัปโหลดโค้ดใหม่ต้องกด `Ctrl + C` ปิด minichlink ก่อนทุกครั้ง

