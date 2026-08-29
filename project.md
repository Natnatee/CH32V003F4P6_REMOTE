# CH32V003F4P6 Remote Test Project

## Overview
โปรเจกต์ทดสอบเริ่มต้นสำหรับบอร์ดพัฒนา CH32V003F4P6 (TSSOP20) พร้อมพอร์ต Type-C USB โดยใช้เฟรมเวิร์ก `ch32v003fun` บน PlatformIO และดีบั๊กผ่าน `SDI Debug Printf` (สาย SWDIO เส้นเดียว)

## Goals
- ทดสอบการคอมไพล์และแฟลชโปรแกรมลงบอร์ด CH32V003F4P6
- ทดสอบ Blink LED (PC1 / PD4)
- ทดสอบการส่งข้อความผ่าน SDI Debug Printf ด้วยเครื่องมือ `minichlink -T`
- เตรียมโครงสร้างมาตรฐานสำหรับพัฒนาฟังก์ชัน Remote / Controller ต่อไป

## Hardware
- **MCU**: WCH CH32V003F4P6 (32-bit RISC-V QingKe V2A, 48MHz, 16KB Flash, 2KB SRAM)
- **Package**: TSSOP-20
- **Board**: CH32V003F4P6 Dev Board (Type-C USB, Onboard Reset Button, Power & Test LEDs)
- **Programmer**: WCH-LinkE (โหมด WCH-LinkRV)

## Pin Map

### ขาเชื่อมต่อ WCH-LinkE (3 สายหลัก)
| ขา WCH-LinkE | ขาบนบอร์ด CH32V003F4P6 | หน้าที่ | Direction | Notes |
|---|---|---|---|---|
| 3.3V | V (VDD) | จ่ายไฟ 3.3V | Power | ไฟเลี้ยงระบบ |
| GND | G (GND) | กราวด์ | Power | จุดกราวด์ร่วม |
| SWDIO / SDI | PD1 (SWIO) | สื่อสารโปรแกรมและดีบั๊ก | Bidirectional | ⚠️ ห้ามต่อโหลดพ่วง |

### Pin Map ของบอร์ด TSSOP-20
| Function | Pin | Device | Direction | Notes |
|---|---|---|---|---|
| LED / GPIO | PC1 | Onboard / Ext LED | Output | พินทดสอบไฟกระพริบ |
| LED / GPIO | PD4 | Onboard / Ext LED | Output | พินทดสอบไฟกระพริบสำรอง |
| SWIO / SDI | PD1 | WCH-LinkE | In/Out | ขาโปรแกรมและ SDI Printf |
| UART1_TX | PD5 (TX) | Serial TX | Output | ขาสื่อสารซีเรียล |
| UART1_RX | PD6 (RX) | Serial RX | Input | ขาสื่อสารซีเรียล |
| RESET | PD7 (NRST)| Reset Button | Input | ปุ่มรีเซ็ตบนบอร์ด |
| GPIO / AIN | PA1, PA2, PC0..PC7, PD0..PD3 | Header Pins | TBD | รอการกำหนดในเฟสถัดไป |

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

