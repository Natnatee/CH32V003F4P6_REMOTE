# CH32V003F4P6 Smart Remote Project

## Overview
โปรเจกต์พัฒนาระบบ **Smart Remote** บนไมโครคอนโทรลเลอร์ **CH32V003F4P6** (TSSOP-20):
1. **16 Profiles แบรนด์อุปกรณ์:**
   - 01: `SAMSUNG`
   - 02: `LG`
   - 03: `HIKVI`
   - 04: `SHARP`
   - 05: `SONY`
   - 06 .. 16: `06` .. `16`
2. **3 โหมดการทำงาน:**
   - **`SEND` (โหมดปกติ):** แสดง `SEND 05/16` กดปุ่ม 4/8 เพื่อเปลี่ยน Profile และกดปุ่มตาราง 1-15 เพื่อยิงสัญญาณ
   - **`LEARN` (โหมดเรียนรู้):** แสดง `LRN  05/16` บันทึกโค้ดจากรีโมทจริง
   - **`NEW` (โหมดกวาดโค้ด):** แสดง `NEW  05/16` กวาดเทสโค้ดอัตโนมัติ
3. **การสลับโหมด (UX Flow):**
   - กดปุ่ม **`16 (BACK)`** ➡️ Footer ด้านล่างจะกระพริบไฮไลต์โหมด `[ SEND ]` 🔁 `[ LEARN ]` 🔁 `[ NEW ]`
   - ใช้ปุ่ม **`4 (UP)` / `8 (DOWN)`** เพื่อเลื่อนเลือกโหมด
   - กดปุ่ม **`12 (OK)`** เพื่อยืนยัน หรือกด **`16 (BACK)`** เพื่อยกเลิก



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

### 3. ตารางการต่อระบบอินฟราเรด (IR Module)
| อุปกรณ์ IR | ขาบอร์ด CH32V003 | โหมดการทำงาน | คำอธิบาย |
|---|---|---|---|
| **IR TX (หลอดส่ง IR)** | **`PD4`** | Output (TIM2 PWM / Bit-Bang) | ยิงสัญญาณ IR 38kHz |
| **IR RX (ตัวรับ VS1838B)** | **`PD0`** | Input (EXTI / Polling) | รับสัญญาณ IR 38kHz |

### 4. โปรแกรมเมอร์ WCH-LinkE
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
- โครงสร้างหน้าจอ UI 3 ส่วน พร้อมตาราง 3x4 (12 ช่องปุ่ม) และ Header ตัวใหญ่สมบูรณ์
- ระบบ 16 Profiles แบรนด์ และ 3 โหมด (SEND / LEARN / NEW) พร้อมระบบสลับโหมดด้วยปุ่ม 16 (BACK)
- **ระบบ Flash Storage (NVM @ 0x08003C00):** บันทึกค่า 16 Profiles ลง Flash ROM อัตโนมัติเมื่อกดบันทึกในโหมด LEARN (ปิดเครื่องแล้วค่าไม่หาย)
- **RAM Optimization:** ถือเฉพาะ Active Profile ปัจจุบันใน RAM (12 ปุ่ม x 4B = 48 ไบต์) ลดการใช้ RAM ลงมหาศาล
- **ระบบ IR Transmitter (PD4):** ยิงคลื่นพาหะ 38kHz PWM ส่งชุดรหัส 32-bit (NEC / Samsung) ในโหมด SEND ทันทีเมื่อกดปุ่มที่มีรหัส
- **ระบบ IR Receiver (PD0):** รับสัญญาณและถอดรหัส 32-bit จากรีโมทจริงในโหมด LEARN

## Open Questions
- การพัฒนาโหมด NEW (Auto Brute-Force Code Search) สำหรับกวาดฐานข้อมูลรหัสทีวี/แอร์



## Issue Log
- **WLink Open Error**: เกิดจาก PlatformIO ค่าเริ่มต้นใช้ OpenOCD/WLink Driver ซึ่งไม่ตรงกับ WinUSB ของระบบ แก้ไขโดยกำหนด `upload_protocol = minichlink` ใน `platformio.ini`
- **ไฟ LED บนบอร์ดติดค้างขณะเปิด Monitor**: เกิดจากการเปิดคำสั่ง `minichlink -T` ดักฟังข้อมูลค้างไว้ ซึ่งตัวโปรแกรมเมอร์จะจับคู่สาย SWDIO ตลอดเวลา ทำให้ไฟ LED แสดงสถานะติดค้าง และหากจะอัปโหลดโค้ดใหม่ต้องกด `Ctrl + C` ปิด minichlink ก่อนทุกครั้ง

