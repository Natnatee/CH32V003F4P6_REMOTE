# CH32V003F4P6 Remote Test Project

โปรเจกต์ทดสอบบอร์ดพัฒนา **CH32V003F4P6** (TSSOP-20) ผ่านเฟรมเวิร์ก `ch32v003fun` และดีบั๊กผ่าน `SDI Debug Printf` (สาย SWDIO เส้นเดียว)

## Features
- รองรับการคอมไพล์และอัปโหลดเฟิร์มแวร์ด้วย **PlatformIO** ร่วมกับ **`ch32v003fun`**
- ไฟกระพริบ LED Blink Test ที่ขา `PC1` และ `PD4`
- ระบบ Serial Debug Monitor ผ่านสาย SWDIO (ไม่ต้องต่อสาย TX/RX แยก)

## Hardware
- **MCU**: WCH CH32V003F4P6 (32-bit RISC-V QingKe V2A @ 48MHz, 16KB Flash, 2KB SRAM)
- **Package**: TSSOP-20
- **Dev Board**: บอร์ด CH32V003F4P6 Type-C พร้อมปุ่ม Reset และหลอด LED
- **Programmer**: WCH-LinkE (โหมด RISC-V / WCH-LinkRV)

## Pin Map

### 1. แผนผังพินของชิปและบอร์ด (ASCII Pinout Diagram)

```text
                         CH32V003F4P6 (TSSOP-20)
                            +-------u-------+
                  PD4 ( 1) -| 1          20 |- (20) PD3
             (TX) PD5 ( 2) -| 2          19 |- (19) PD2
             (RX) PD6 ( 3) -| 3          18 |- (18) PD1 (SWIO/SDI)
           (NRST) PD7 ( 4) -| 4          17 |- (17) PC7
                  PA1 ( 5) -| 5          16 |- (16) PC6
                  PA2 ( 6) -| 6          15 |- (15) PC5
            (GND) VSS ( 7) -| 7          14 |- (14) PC4
                  PD0 ( 8) -| 8          13 |- (13) PC3
            (VDD) VDD ( 9) -| 9          12 |- (12) PC2
                  PC0 (10) -| 10         11 |- (11) PC1 (LED)
                            +---------------+

               CH32V003F4P6 Dev Board Pin Header Layout
                    +-----------------------+
                    |  [  USB TYPE-C  ]     |
            G (GND) | [ ]               [ ] | G (GND)
          V (3V3/5) | [ ]  [PWR] [LED]  [ ] | V (3V3/5)
       TX (PD5/UART)| [ ]      [RST]    [ ] | PD7
       RX (PD6/UART)| [ ]               [ ] | PD4
                PA1 | [ ]  +---------+  [ ] | PD3
                PA2 | [ ]  |  CH32   |  [ ] | PD2
                PC0 | [ ]  | V003F4P6|  [ ] | PD1 (SWIO)
          (LED) PC1 | [ ]  +---------+  [ ] | PD0
                PC2 | [ ]               [ ] | PC7
                PC3 | [ ]               [ ] | PC6
                PC4 | [ ]               [ ] | PC5
                    +-----------------------+
```

### 2. การต่อสายโปรแกรมเมอร์ WCH-LinkE (3 สายหลัก)

| ขา WCH-LinkE | ขาบอร์ด CH32V003F4P6 | หน้าที่ |
|---|---|---|
| **3.3V** | **V (VDD)** | จ่ายแรงดันไฟเลี้ยง 3.3V |
| **GND** | **G (GND)** | กราวด์ร่วม |
| **SWDIO / SDI** | **PD1 (SWIO)** | สายโปรแกรมข้อมูลและ SDI Debug Printf |

## Getting Started

### 1. คอมไพล์โปรเจกต์ (Build)
```bash
pio run
```

### 2. อัปโหลดเฟิร์มแวร์ลงบอร์ด (Upload)
```bash
pio run -t upload
```

### 3. เปิดดู Serial Monitor ผ่านสาย SWDIO (Terminal)
> ⚠️ **ข้อควรระวัง:** ห้ามใช้ `pio run -t upload -t monitor` ให้รัน upload ให้เสร็จก่อน แล้วใช้คำสั่งนี้ใน Git Bash:
```bash
/c/Users/natna/.platformio/packages/tool-minichlink/minichlink -T
```

## Safety & Notes
- ⚠️ **ห้ามเขียนโค้ดทับหรือยุ่งกับขา `PD1` (SWIO)** เพื่อไม่ให้บล็อกสัญญาณของโปรแกรมเมอร์
- **พฤติกรรมขณะเปิด Monitor (`minichlink -T`):** ตัวโปรแกรมเมอร์จะจับคู่สื่อสารผ่าน SWDIO ตลอดเวลา ทำให้ไฟ LED แสดงผลติดแช่ได้ และหากต้องการอัปโหลดเฟิร์มแวร์ใหม่ จะต้องกด `Ctrl + C` เพื่อหยุด Monitor ก่อนเสมอ
- แนะนำต่อตัวต้านทาน 220Ω - 1kΩ อนุกรมกับ LED เมื่อต่อพ่วงภายนอก


## License
MIT
