# Hardware & Library Specs

## Hardware

| Part | Model | Interface | Notes |
|---|---|---|---|
| MCU | CH32V003F4P6 | TSSOP20 | 32-bit RISC-V QingKe V2A, 48MHz, 16KB Flash, 2KB SRAM |
| Dev Board | CH32V003F4P6 Type-C Devboard | USB Type-C, Headers | Onboard Reset Button, Power LED, User LED |
| Display | 0.96" OLED (SSD1306) | I2C Bit-Bang (PC6=SCL, PC7=SDA) | Portrait Framebuffer 64x128 pixels (1KB) |
| Keypad | 4x4 Matrix Keypad | 8-Pin Header (RX..PC4) | Rows: PD6, PA1, PA2, PC0; Cols: PC1..PC4 |
| Programmer | WCH-LinkE | 1-Wire SDI (PD1) | โหมด WCH-LinkRV (RISC-V) |

## Libraries

| Purpose | Library | Status | Notes |
|---|---|---|---|
| Core Framework | ch32v003fun | active | Minimalist RISC-V framework with SDI printf |


## Datasheet / References
- CH32V003 Reference Manual (WCH)
- CH32V003 Datasheet (WCH)
- ch32v003fun Framework Repository
- PlatformIO Community CH32V Platform

## Hardware Issues
- ในระบบ Windows หาก WCH-LinkE อยู่ในโหมด ARM ให้สลับเป็นโหมด RISC-V ผ่านโปรแกรม WCH-LinkUtility
- หากพบปัญหา `libusb_open() = -5` ให้ใช้โปรแกรม Zadig ติดตั้งไดรเวอร์ WinUSB ให้กับ WCH-Link
- ⚠️ ขา `PD1` (SWIO) ห้ามเขียนโค้ดเปลี่ยนเป็น Output หรือดึงกระแส เพื่อป้องกันชิปล็อกการสื่อสาร
- **พฤติกรรมไฟ LED ติดแช่ขณะเปิด Monitor**: ขณะรัน `minichlink -T` ตัวโปรแกรมเมอร์จะสื่อสารทาง SWDIO ตลอดเวลา ทำให้ไฟ LED แสดงผลติดแช่ และไม่สามารถสั่ง `upload` ซ้ำได้จนกว่าจะกด `Ctrl + C` ปิด minichlink ก่อน

## Test Notes
- ทดสอบ Blink LED ที่พิน PC1 และ PD4
- ทดสอบ SDI Debug Printf แสดงผลออก Terminal
- ต้องหยุดมอนิเตอร์ก่อนอัปโหลดเฟิร์มแวร์ใหม่เสมอ

