#pragma once

#include <stdint.h>

#define TOTAL_PROFILES_COUNT    16
#define TOTAL_BRUTE_COMMANDS    256

// รายชื่อ Profile 16 ช่อง
static const char *profile_names[TOTAL_PROFILES_COUNT] = {
    "SAMSUNG", // 01: Samsung Smart TV
    "LG",      // 02: LG webOS Smart TV
    "HIKVI",   // 03: Hikvision CCTV DVR/NVR
    "SHARP",   // 04: Sharp Aquos TV
    "SONY",    // 05: Sony Bravia TV
    "TCL",     // 06: TCL / Hisense Android TV
    "MIBOX",   // 07: Xiaomi Mi Box / TrueID
    "APPLE",   // 08: Apple TV
    "PANAS",   // 09: Panasonic Viera TV
    "10",      // 10
    "11",      // 11
    "12",      // 12
    "13",      // 13
    "14",      // 14
    "15",      // 15
    "16"       // 16
};

// รหัส Address 16-bit ประจำแต่ละแบรนด์
static const uint16_t profile_addresses[TOTAL_PROFILES_COUNT] = {
    0xE0E0, // 01: SAMSUNG
    0x20DF, // 02: LG
    0x00FF, // 03: HIKVI
    0xAA55, // 04: SHARP
    0x0000, // 05: SONY
    0xF708, // 06: TCL
    0x807F, // 07: MIBOX
    0x77E1, // 08: APPLE
    0x4004, // 09: PANAS
    0x00FF, // 10: Generic NEC Universal
    0x00FF, // 11
    0x00FF, // 12
    0x00FF, // 13
    0x00FF, // 14
    0x00FF, // 15
    0x00FF  // 16
};

// ข้อมูล 12 ปุ่มมาตรฐาน:
// [0]: 1(ON/Power)   [1]: 2(UP)        [2]: 3(OFF/Mute)
// [3]: 5(LEFT)       [4]: 6(OK)        [5]: 7(RIGHT)
// [6]: 9(BACK)       [7]: 10(DOWN)     [8]: 11(HOME/Menu)
// [9]: 13(INPUT)     [10]: 14(VOL-)    [11]: 15(VOL+)

static const uint32_t default_presets[TOTAL_PROFILES_COUNT][12] = {
    // 01: SAMSUNG TV
    {
        0xE0E040BF, // 1: Power
        0xE0E006F9, // 2: UP
        0xE0E0F00F, // 3: MUTE
        0xE0E0A659, // 5: LEFT
        0xE0E016E9, // 6: OK
        0xE0E046B9, // 7: RIGHT
        0xE0E01AE5, // 9: BACK
        0xE0E08679, // 10: DOWN
        0xE0E058A7, // 11: HOME
        0xE0E0807F, // 13: SOURCE/INPUT
        0xE0E0D02F, // 14: VOL-
        0xE0E0E01F  // 15: VOL+
    },
    // 02: LG TV
    {
        0x20DF10EF, // 1: Power
        0x20DF02FD, // 2: UP
        0x20DF906F, // 3: MUTE
        0x20DFE01F, // 5: LEFT
        0x20DF22DD, // 6: OK
        0x20DF609F, // 7: RIGHT
        0x20DF14EB, // 9: BACK
        0x20DF827D, // 10: DOWN
        0x20DFC23D, // 11: HOME
        0x20DFD02F, // 13: INPUT
        0x20DFC03F, // 14: VOL-
        0x20DF40BF  // 15: VOL+
    },
    // 03: HIKVISION DVR
    {
        0x00FF00FF, // 1: Power
        0x00FF02FD, // 2: UP
        0x00FF906F, // 3: MUTE
        0x00FFE01F, // 5: LEFT
        0x00FF22DD, // 6: OK
        0x00FF609F, // 7: RIGHT
        0x00FF14EB, // 9: ESC/BACK
        0x00FF827D, // 10: DOWN
        0x00FFC23D, // 11: MENU
        0x00FFD02F, // 13: REC
        0x00FFC03F, // 14: PREV
        0x00FF40BF  // 15: NEXT
    },
    // 04: SHARP TV
    {
        0xAA5540BF, // 1: Power
        0xAA5502FD, // 2: UP
        0xAA55F00F, // 3: MUTE
        0xAA55E01F, // 5: LEFT
        0xAA5516E9, // 6: OK
        0xAA55609F, // 7: RIGHT
        0xAA551AE5, // 9: BACK
        0xAA55827D, // 10: DOWN
        0xAA5558A7, // 11: MENU
        0xAA55807F, // 13: INPUT
        0xAA55D02F, // 14: VOL-
        0xAA55E01F  // 15: VOL+
    },
    // 05: SONY TV
    {
        0x00000A90, // 1: Power
        0x00000090, // 2: UP
        0x00000290, // 3: MUTE
        0x00000C90, // 5: LEFT
        0x00000A70, // 6: OK
        0x00000490, // 7: RIGHT
        0x00000C70, // 9: RETURN
        0x00000890, // 10: DOWN
        0x00000070, // 11: HOME
        0x00000A50, // 13: INPUT
        0x00000C90, // 14: VOL-
        0x00000490  // 15: VOL+
    },
    // 06: TCL TV
    {
        0xF708FB04, // 1: Power
        0xF708D827, // 2: UP
        0xF708F00F, // 3: MUTE
        0xF70858A7, // 5: LEFT
        0xF708B847, // 6: OK
        0xF7087887, // 7: RIGHT
        0xF70818E7, // 9: BACK
        0xF708F807, // 10: DOWN
        0xF7086897, // 11: HOME
        0xF708E01F, // 13: SOURCE
        0xF708E718, // 14: VOL-
        0xF708D728  // 15: VOL+
    },
    // 07: MIBOX / TRUEID ANDROID TV BOX
    {
        0x807F807F, // 1: Power
        0x807F08F7, // 2: UP
        0x807F906F, // 3: MUTE
        0x807F48B7, // 5: LEFT
        0x807F00FF, // 6: OK
        0x807FC837, // 7: RIGHT
        0x807F20DF, // 9: BACK
        0x807F8877, // 10: DOWN
        0x807FA05F, // 11: HOME
        0x807F10EF, // 13: MENU
        0x807FE01F, // 14: VOL-
        0x807F609F  // 15: VOL+
    },
    // 08: APPLE TV
    {
        0x77E1BA01, // 1: Play/Pause
        0x77E1D001, // 2: UP
        0x77E1E001, // 3: Mute
        0x77E11001, // 5: LEFT
        0x77E1A001, // 6: SELECT
        0x77E1E001, // 7: RIGHT
        0x77E14001, // 9: MENU
        0x77E1B001, // 10: DOWN
        0x77E17C01, // 11: HOME
        0x77E10201, // 13: SOURCE
        0x77E13001, // 14: VOL-
        0x77E15001  // 15: VOL+
    },
    // 09: PANASONIC TV
    {
        0x40040100, // 1: Power
        0x40040152, // 2: UP
        0x4004014C, // 3: MUTE
        0x40040172, // 5: LEFT
        0x40040192, // 6: OK
        0x400401B2, // 7: RIGHT
        0x4004012B, // 9: RETURN
        0x400401D2, // 10: DOWN
        0x4004014B, // 11: MENU
        0x40040105, // 13: AV/INPUT
        0x40040121, // 14: VOL-
        0x40040120  // 15: VOL+
    },
    // 10..16: ว่าง (0)
    {0}, {0}, {0}, {0}, {0}, {0}, {0}
};

// ฟังก์ชันสร้างรหัสคำสั่ง NEC 32-bit อัตโนมัติ (Command 0x00 ถึง 0xFF รวม 256 คำสั่ง)
static inline uint32_t generate_brute_code(uint8_t profile_idx, uint8_t cmd) {
    if (profile_idx == 4) {
        // SONY 12-bit
        return (uint32_t)cmd;
    }
    if (profile_idx == 7) {
        // APPLE TV
        return (0x77E10000UL) | ((uint32_t)cmd << 8) | 0x01;
    }

    uint16_t addr = (profile_idx < TOTAL_PROFILES_COUNT) ? profile_addresses[profile_idx] : 0x00FF;
    uint8_t inv_cmd = ~cmd;
    return ((uint32_t)addr << 16) | ((uint32_t)cmd << 8) | inv_cmd;
}
