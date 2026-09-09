# CH32V003F4P6 Smart Remote Project

โปรเจกต์ **Smart Remote** บนบอร์ดพัฒนา **CH32V003F4P6** (TSSOP-20) ผ่านเฟรมเวิร์ก `ch32v003fun`:
- **16 Profiles พร้อม Factory Presets 9 แบรนด์ดัง:**
  - `01: SAMSUNG`, `02: LG`, `03: HIKVI`, `04: SHARP`, `05: SONY`, `06: TCL`, `07: MIBOX`, `08: APPLE`, `09: PANAS`, `10..16: 10..16`
- **ผังปุ่มควบคุม D-Pad สากล (ตาราง 3x4):**
  - `1: ON / Power` | `2: UP (บน)` | `3: OFF / MUTE`
  - `5: LEFT (ซ้าย)` | `6: OK (ตกลง)` | `7: RIGHT (ขวา)`
  - `9: BACK (ย้อนกลับ)` | `10: DOWN (ลง)` | `11: HOME / MENU`
  - `13: INPUT / HDMI` | `14: VOL - (ลดเสียง)` | `15: VOL + (เพิ่มเสียง)`
- **7 โหมดการทำงาน:**
  - `SEND`: โหมดยิงสัญญาณปกติ (`SEND 01/16`) ยิงคลื่น 38kHz ออกขา `PD4`
  - `LEARN`: โหมดเรียนรู้จากรีโมทจริง (`LRN  01/16`) รับสัญญาณทางขา `PD0`
  - `NEW`: โหมดล่ารหัส / 256-Command Brute-Force Hunter (`NEW  01/16`) กด `4/8` กวาดยิงคำสั่ง 0x00 ถึง 0xFF (`[ 001/256 ]`) + โชว์หน้าจอ 5 วิ + กด `1..15` เพื่อ Save
  - `RENAME`: โหมดแก้ไขชื่อโปรไฟล์ (`NAME 01/16`) กด 4/8 เลือก Profile กด 12 เพื่อแก้ไขชื่อ (พิมพ์ด้วยปุ่มตาราง 1..15 ผัง `AB`, `CD`..`WZ`) และกด 12 บันทึกชื่อลง Flash ROM ถาวร
  - `TETRIS`: โหมดเล่นเกม Tetris คลาสสิก (`[ TETRIS ]`) พร้อม High Score ถาวรใน Flash (`HI:xxxxx`)
  - `CALC`: โหมดเครื่องคิดเลขทศนิยม (`[  CALC  ]`) คิดเลขบวก ลบ คูณ หาร และทศนิยมด้วยตาราง 4x4 (+-, */, =, EX)
  - `METER`: โหมดมัลติมิเตอร์ INA226 (`[ METER ]`) วัดแรงดันและกระแสไฟฟ้าแบบ 3 หน้าย่อย (`REAL TIME`, `PEAK`, `AVG`) พร้อมรองรับตัวต้านทาน Shunt 10Ω (วัดละเอียดระดับ µA/mA)


> 📘 **อ่านคู่มือการใช้งานและการกดปุ่มฉบับเต็มได้ที่ [DOCUMENT.md](file:///c:/MCU/WCH/CH32V003F4P6_REMOTE/DOCUMENT.md)**









## Hardware
- **MCU**: WCH CH32V003F4P6 (32-bit RISC-V QingKe V2A @ 48MHz, 16KB Flash, 2KB SRAM)
- **Package**: TSSOP-20
- **Display**: 0.96" SSD1306 OLED (I2C: PC6=SCL, PC7=SDA)
- **Keypad**: 4x4 Matrix Keypad (16 Buttons: RX..PC4)
- **IR Receiver**: VS1838B 38kHz (Data: PD0)
- **IR Transmitter**: 940nm IR LED (PWM/Bit-Bang: PD4)
- **Programmer**: WCH-LinkE (โหมด RISC-V / WCH-LinkRV)

## Pin Map

### 1. แผนผังพินของบอร์ด (ASCII Pinout Diagram)

```text
               CH32V003F4P6 Dev Board Pin Header Layout
                    +-----------------------+
                    |  [  USB TYPE-C  ]     |
            G (GND) | [ ]               [ ] | G (GND)
          V (3V3/5) | [ ]  [PWR] [LED]  [ ] | V (3V3/5)
       TX (PD5/UART)| [ ]      [RST]    [ ] | PD7 (NRST - ปุ่ม Reset)
 [K4]  RX (PD6/UART)| [ ]               [ ] | PD4 (📡 IR TX / หลอด IR Send)
 [K3]           PA1 | [ ]  +---------+  [ ] | PD3
 [K2]           PA2 | [ ]  |  CH32   |  [ ] | PD2
 [K1]           PC0 | [ ]  | V003F4P6|  [ ] | PD1 (SWIO แฟลช)
 [K5]           PC1 | [ ]  +---------+  [ ] | PD0 (📥 IR RX / ตัวรับ VS1838)
 [K6]           PC2 | [ ]               [ ] | PC7 (OLED SDA)
 [K7]           PC3 | [ ]               [ ] | PC6 (OLED SCL)
 [K8]           PC4 | [ ]               [ ] | PC5
                    +-----------------------+
```

### 2. ตารางการต่อสาย Keypad 4x4 (8 พินเรียงฝั่งซ้าย)
| ขา Keypad | ขาบอร์ด CH32V003 | โหมดการทำงาน | คำอธิบาย |
|---|---|---|---|
| **Pin 1 (K1)** | **`PC0`** | Output | Row 1 (แถว 1: ปุ่ม S1..S4) |
| **Pin 2 (K2)** | **`PA2`** | Output | Row 2 (แถว 2: ปุ่ม S5..S8) |
| **Pin 3 (K3)** | **`PA1`** | Output | Row 3 (แถว 3: ปุ่ม S9..S12) |
| **Pin 4 (K4)** | **`RX (PD6)`** | Output | Row 4 (แถว 4: ปุ่ม S13..S16) |
| **Pin 5 (K5)** | **`PC1`** | Input Pull-Up | Col 1 (หลัก 1) |
| **Pin 6 (K6)** | **`PC2`** | Input Pull-Up | Col 2 (หลัก 2) |
| **Pin 7 (K7)** | **`PC3`** | Input Pull-Up | Col 3 (หลัก 3) |
| **Pin 8 (K8)** | **`PC4`** | Input Pull-Up | Col 4 (หลัก 4) |

### 3. ตารางการต่อจอ OLED SSD1306 (I2C)
| ขา OLED | ขาบอร์ด CH32V003 | คำอธิบาย |
|---|---|---|
| **SCL** | **`PC6`** | I2C Clock |
| **SDA** | **`PC7`** | I2C Data |
| **VCC** | **`V (3.3V)`** | ไฟเลี้ยง 3.3V |
| **GND** | **`G (GND)`** | กราวด์ |

### 4. ตารางการต่อระบบอินฟราเรด (IR Module)
| อุปกรณ์ IR | ขาบอร์ด CH32V003 | คำแนะนำการต่อวงจร |
|---|---|---|
| **IR TX (หลอดส่ง IR)** | **`PD4`** | ต่อผ่านตัวต้านทาน 100Ω - 220Ω เข้าขา Anode (+), ขา Cathode ต่อลง GND |
| **IR RX (ตัวรับ VS1838B)** | **`PD0` (OUT), `V` (VCC), `G` (GND)** | หันตุ่มนูนเข้าหาตัว: ขาซ้าย=PD0, ขากลาง=GND, ขาขวา=3.3V |

### 5. การต่อสายโปรแกรมเมอร์ WCH-LinkE (3 สาย)
| ขา WCH-LinkE | ขาบอร์ด CH32V003 |
|---|---|
| **3.3V** | **`V (3.3V)`** |
| **GND** | **`G (GND)`** |
| **SWDIO** | **`PD1 (SWIO)`** |



## Getting Started

## Sharp IR Protocol (ทดสอบสำเร็จ)

การรับและส่งรีโมท Sharp ทดสอบกับทีวีจริงสำเร็จแล้ว โดยใช้เฉพาะ logic decoder แบบ Sharp/Denon ที่พอร์ตจากโปรเจกต์ STM32 ไม่ได้นำไลบรารี IR ทั้งชุดมาใช้

- รูปแบบ: 15-bit, LSB-first, pulse-distance
- Timing โดยประมาณ: mark `260µs`, zero space `780µs`, one space `1820µs`
- ข้อมูลทดสอบ: raw `0x2D71`
- Address: `0x11`
- Command: `0x6B`
- Frame marker: `01`
- การส่งซ้ำ: `normal → inverted → normal` เว้นช่วงประมาณ `45ms`
- โค้ดที่เกี่ยวข้อง: `src/ir_recv.c` และ `src/ir_send.c`

โหมดทดสอบชั่วคราวถูกถอดออกแล้ว ปุ่ม `12 (OK)` ถูกสงวนไว้สำหรับฟังก์ชัน Profile/IR ในอนาคต
Profile รุ่นเก่าถูก invalidate เฉพาะรหัสปุ่ม โดยคงชื่อ Profile เดิมไว้ เพื่อเตรียมระบบ LEARN รุ่นใหม่

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
