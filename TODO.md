---
status: active
type: troubleshooting-plan
platform: ch32v003f4p6
tags:
  - flash
  - ram
  - ir
  - sharp
  - minichlink
  - raw-capture
related:
  - "[[project]]"
  - "[[DOCUMENT]]"
  - "[[hardware_lib]]"
---

# TODO: แก้ปัญหา Flash และ IR Sharp

## เป้าหมาย

ทำให้ Smart Remote มีพื้นที่ Flash เหลือเพียงพอ และหาสาเหตุที่รีโมท Sharp ยิงไม่ทำงาน โดยเก็บข้อมูล timing/raw จากรีโมทจริงก่อนตัดสิน protocol

## สถานะเริ่มต้น

- Flash: ใช้ `15708 / 16384 bytes (95.9%)`
- RAM: ใช้ `1508 / 2048 bytes (73.6%)`
- อัปโหลดผ่าน `minichlink` ได้แล้ว หลังติดตั้ง WinUSB ให้ WCH-Link Interface 0
- Sharp ยังยิงไม่ทำงาน
- สมมติฐานเบื้องต้น: รูปแบบที่บันทึกไว้ไม่ตรงกับสัญญาณจากรีโมท Sharp จริง และอาจไม่ใช่ NEC
- ต้องคงโมดูลอ่าน/พิมพ์ข้อมูลผ่าน serial/SDI ไว้ชั่วคราว เพื่อใช้วิเคราะห์ raw signal

## แผนงาน

### Phase 1 — ลดการใช้ Flash

- [x] เอาโมดูล Tetris ออกจาก build และจุดเรียกใช้งานใน `src/main.c`
- [x] ตรวจ dependency ที่เกี่ยวข้องกับ `src/tetris.c` และ `src/tetris.h`
- [ ] ตรวจไฟล์ที่อาจไม่ได้ใช้งานจริง เช่น `src/flappy.c` / `src/flappy.h`
- [ ] รีแฟกเตอร์โค้ดที่ซ้ำ โดยเฉพาะ UI, string, profile และโหมดที่ไม่จำเป็นต่อการทดสอบ IR
- [ ] ลด string/debug message ที่ไม่จำเป็น แต่คง raw serial output ไว้
- [ ] Build และบันทึกค่า Flash/RAM ใหม่
- [ ] ตั้งเป้า Flash ต่ำกว่า 85% ก่อนเริ่มวิเคราะห์ protocol

### Phase 2 — เตรียม minichlink

- [x] ตรวจว่า `minichlink.exe` จาก PlatformIO ใช้งานได้
- [x] ตรวจ path สำหรับเรียก `minichlink` ใน PowerShell
- [x] ยืนยันว่าใช้ WCH-LinkE โหมด RISC-V และ WinUSB ที่ Interface 0
- [x] อ่านข้อมูลชิปด้วย `minichlink -i` สำเร็จ: CH32V003, Flash 16 kB, Read protection disabled

คำสั่งที่ตรวจสอบแล้ว:

```powershell
& 'C:\Users\natna\.platformio\packages\tool-minichlink\minichlink.exe' -i
```

สำหรับ Git Bash:

```bash
/c/Users/natna/.platformio/packages/tool-minichlink/minichlink.exe -i
```

### Phase 3 — ทดสอบโหมดรับและเก็บ raw signal

- [x] ปรับ `src/ir_recv.c` ให้เก็บ pulse/space เป็น raw frame ก่อน decode
- [x] เพิ่ม frame synchronization โดยรอ idle HIGH อย่างน้อย 8 ms ก่อนเริ่มจับ
- [x] แยกการพิมพ์แต่ละเฟรม รวมถึงเฟรม repeat เป็นบรรทัดแยกกัน
- [ ] Flash firmware รุ่นทดสอบที่เปิดโหมดรับ IR และ SDI/raw serial
- [ ] เปิด `minichlink -T`
- [ ] ใช้รีโมท Sharp กดปุ่มเดิมซ้ำอย่างน้อย 3 ครั้ง
- [x] ใช้รีโมท Sharp กดปุ่มเดิมซ้ำ 3 ครั้ง และได้ 2 เฟรมต่อครั้ง
- [x] บันทึก raw timing จริงของปุ่มทดสอบ
- [x] ตรวจว่า repeat frame, bit timing และจำนวนบิตเหมือนกัน
- [x] ยืนยันว่าไม่ใช่ NEC: ได้ 15-bit pulse-distance timing แบบ Sharp

ผลการทดสอบปุ่ม Sharp:

- ได้ทั้งหมด 6 เฟรม (`2 เฟรม/การกด 1 ครั้ง`)
- ทุกเฟรมมี 31 pulses = 15 bit pairs + trailing mark
- Bit pattern ที่อ่านตามลำดับเวลาเหมือนกันทุกเฟรม: `100011101011010`
- Timing โดยประมาณ: mark `245–343 us`, zero space `~760–830 us`, one space `~1760–1890 us`
- รอบนี้ repeat เป็นข้อมูลเหมือนกัน ไม่พบเฟรม inverted จากการกดทดสอบ
- ค่าที่ต้องเก็บต่อไปควรเป็น Sharp raw 15-bit ไม่ใช่ NEC 32-bit

### Phase 4 — แก้รูปแบบบันทึกและ decoder

- [x] เพิ่ม Sharp decoder 15-bit แบบ LSB-first ตาม logic ของ IRremote/Denon
- [x] เพิ่ม `ir_send_sharp()` แบบ normal/inverted/normal
- [x] ทดสอบ Sharp decoder และการยิง protocol จริงสำเร็จ
- [x] เอาโหมดทดสอบยิงชั่วคราวออกจากปุ่ม `12 (OK)`
- [x] ทำให้ Profile เก่าถูกล้างรหัสปุ่มและคงชื่อเดิม ด้วย storage magic version ใหม่
- [x] บันทึก protocol และจำนวนบิตของ Profile ใน reserved Flash word
- [x] ให้ SEND เลือก `ir_send_sharp()` สำหรับ Profile ที่เรียนเป็น Sharp
- [ ] เปรียบเทียบ raw timing กับข้อมูลที่ `flash_storage` บันทึกอยู่
- [x] เพิ่ม NEC decoder/sender แบบ 32-bit LSB-first
- [ ] ทดสอบเรียนและยิง NEC พร้อมวัด Flash/RAM
- [x] เพิ่ม Samsung decoder/sender แบบ 32-bit LSB-first
- [ ] ทดสอบเรียนและยิง Samsung พร้อมวัด Flash/RAM
- [x] เพิ่ม LG decoder/sender แบบ 28-bit
- [x] เพิ่ม Sony/SIRC decoder/sender แบบ 12/15/20-bit
- [x] เพิ่ม JVC decoder/sender แบบ 16-bit
- [ ] ทดสอบจริง Samsung/LG/Sony/JVC และตรวจ protocol repeat ของแต่ละรุ่น

## Experiment Log

### 2026-09-07 — Sharp receive/send test

- Receiver จับเฟรมเต็มได้สำเร็จ: `31 pulses` หรือ 15 bit pairs + trailing mark
- ทดสอบกดรีโมท Sharp 3 ครั้ง ได้ 6 เฟรม หรือ 2 เฟรมต่อการกด
- Bit pattern ที่อ่านได้เหมือนกันทุกเฟรม: `100011101011010`
- สร้าง decoder candidate เป็น raw `0x473A`
- เพิ่ม `ir_send_sharp_raw(0x473A, 2)` แล้วทดสอบ: ทีวีไม่ตอบสนอง
- เพิ่ม `ir_send_raw_frame()` ใช้ timing 31 ค่าจากเฟรมจริง และยิงซ้ำ 2 ครั้ง gap 40 ms: ทีวีไม่ตอบสนอง
- ข้อสรุปปัจจุบัน: ฝั่งรับและการพิมพ์ raw ทำงาน แต่ยังยืนยันไม่ได้ว่าข้อมูลที่จับเริ่มตรงกับ carrier frame ที่อุปกรณ์ต้องการ หรือรูปแบบการส่ง/ขั้วสัญญาณถูกต้อง
- แก้ไขตาม source STM32/IRremote: Sharp ใช้ Denon decoder, 15-bit LSB-first, address 5-bit, command 8-bit, marker 2-bit; raw จากเฟรมทดสอบคำนวณเป็น `0x2D71` (address `0x11`, command `0x6B`, marker `01`)
- สมมติฐานที่ต้องตรวจต่อ:
  - ลำดับบิตหรือการจัดรูปแบบ Sharp 15-bit อาจกลับด้าน
  - repeat frame/gap ของรีโมทจริงอาจต้องใช้รูปแบบเฉพาะ
  - timing ที่วัดจาก VS1838B อาจต้องชดเชย mark/space ก่อนส่ง
  - carrier frequency, duty cycle, polarity, วงจร IR LED หรือทิศทางหลอดอาจผิด
  - ค่าที่เก็บใน Profile เดิมอาจเป็น decoded format คนละชนิดกับ raw frame
- [ ] ตรวจว่า storage เก็บเป็น decoded command, raw pulse หรือค่าที่ถูกตัดทอน
- [ ] แยก protocol/decoder ของ Sharp ออกจาก NEC/Samsung หากรูปแบบไม่เหมือนกัน
- [ ] เพิ่มโครงสร้างข้อมูลที่รองรับ protocol และจำนวนบิตตามที่ตรวจพบ
- [ ] ทดสอบบันทึกค่าแล้วปิด/เปิดเครื่องเพื่อยืนยันว่า Flash ไม่ทำให้ข้อมูลเพี้ยน

### Phase 5 — ทดสอบโหมดยิง

- [ ] ยิงรหัส Sharp ที่บันทึกจากรีโมทจริง
- [ ] ตรวจ carrier frequency, duty cycle, polarity และ timing
- [ ] ทดสอบปุ่ม Power/Mute/Volume อย่างน้อย 3 ปุ่ม
- [ ] เปรียบเทียบผลกับการยิงจากรีโมท Sharp จริง
- [ ] ตรวจ Flash/RAM หลังเพิ่ม decoder ใหม่

## ข้อควรระวัง

- ห้ามใช้ `PD1 (SWIO)` เป็น GPIO หรือเปลี่ยนทิศทางขา
- ต้องหยุด `minichlink -T` ด้วย `Ctrl+C` ก่อน upload firmware ใหม่
- ห้ามลบโมดูล raw serial จนกว่าจะเก็บ timing ของ Sharp ครบ
- ถ้าพื้นที่ยังไม่พอ ให้ถอดฟีเจอร์ที่ไม่เกี่ยวกับ IR ชั่วคราว เช่นเกมหรือเครื่องคิดเลข ก่อนเปลี่ยน MCU

## Definition of Done

- Flash เหลือตามเป้าหมายที่กำหนดและ build ผ่าน
- มี raw timing ของรีโมท Sharp ที่ตรวจซ้ำได้
- ระบุได้ว่าข้อมูลที่บันทึกเดิมผิดที่ขั้นตอนใด
- โหมด LEARN บันทึกข้อมูล Sharp ได้ถูกต้อง
- โหมด SEND ยิงคำสั่ง Sharp ให้เครื่องรับตอบสนองได้จริง
