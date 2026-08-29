#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "keypad.h"
#include "ir_recv.h"

void SetupDebugPrintf(void);

// 16 รายชื่อ Profile แบรนด์
static const char *profile_names[16] = {
    "SAMSUNG", // 01
    "LG",      // 02
    "HIKVI",   // 03
    "SHARP",   // 04
    "SONY",    // 05
    "06",      // 06
    "07",      // 07
    "08",      // 08
    "09",      // 09
    "10",      // 10
    "11",      // 11
    "12",      // 12
    "13",      // 13
    "14",      // 14
    "15",      // 15
    "16"       // 16
};

// 3 โหมดการทำงาน
typedef enum {
    MODE_SEND = 0,
    MODE_LEARN = 1,
    MODE_NEW = 2
} RemoteMode;

static const char *mode_names[3] = {
    "SEND",
    "LRN ",
    "NEW "
};

static const char *mode_select_labels[3] = {
    "[ SEND ]",
    "[ LEARN ]",
    "[  NEW  ]"
};

// ตารางจัดเก็บรหัส IR สำหรับ 16 Profiles x 12 ปุ่ม (Keys: 1..3, 5..7, 9..11, 13..15)
static uint32_t profile_codes[16][12];

// แปลงหมายเลขปุ่ม (1..15) เป็น Index ในตาราง (0..11)
static int key_to_index(uint8_t key) {
    switch (key) {
        case 1:  return 0;
        case 2:  return 1;
        case 3:  return 2;
        case 5:  return 3;
        case 6:  return 4;
        case 7:  return 5;
        case 9:  return 6;
        case 10: return 7;
        case 11: return 8;
        case 13: return 9;
        case 14: return 10;
        case 15: return 11;
        default: return -1;
    }
}

// สร้างข้อความ Footer ปกติ เช่น "SEND 05/16"
static void get_footer_str(char *buf, RemoteMode mode, uint8_t profile_idx) {
    uint8_t p_num = profile_idx + 1;
    const char *m_str = mode_names[mode];

    buf[0] = m_str[0];
    buf[1] = m_str[1];
    buf[2] = m_str[2];
    buf[3] = m_str[3];
    buf[4] = ' ';
    buf[5] = '0' + (p_num / 10);
    buf[6] = '0' + (p_num % 10);
    buf[7] = '/';
    buf[8] = '1';
    buf[9] = '6';
    buf[10] = '\0';
}

int main()
{
    // 1. กำหนดค่า Core Clock 48MHz
    SystemInit();

    // 2. เริ่มต้นระบบ SDI Debug Printf ผ่านขา SWDIO (PD1)
    SetupDebugPrintf();

    // Safety Delay
    Delay_Ms(200);

    printf("\r\n=========================================\r\n");
    printf("   CH32V003 Smart Remote + IR Receiver\r\n");
    printf("   IR RX Pin : PD0 (VS1838B)\r\n");
    printf("   IR TX Pin : PD4 (38kHz PWM)\r\n");
    printf("   16 Profiles x 12 Buttons\r\n");
    printf("=========================================\r\n");

    // ล้างค่าหน่วยความจำรหัสเริ่มต้น
    memset(profile_codes, 0, sizeof(profile_codes));

    // 3. เริ่มต้นฮาร์ดแวร์ OLED, Keypad และ IR Receiver
    ssd1306_init();
    keypad_init();
    ir_recv_init();

    // ตัวแปรสถานะระบบ
    RemoteMode current_mode = MODE_SEND;
    RemoteMode selected_mode = MODE_SEND;
    uint8_t current_profile = 0; // 0 = SAMSUNG (Profile 01)
    uint8_t mode_select_active = 0;

    // ตัวแปรสำหรับโหมด LEARN
    uint8_t ir_captured = 0;
    uint32_t captured_code = 0;

    char footer_buf[16];
    get_footer_str(footer_buf, current_mode, current_profile);
    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);

    uint8_t last_key = 0;
    uint8_t row = 0, col = 0;
    uint32_t blink_tick = 0;
    uint8_t mode_blink_state = 1;

    while (1)
    {
        // --- 1. ตรวจจับสัญญาณ IR (เฉพาะในโหมด LEARN เมื่อยังไม่ได้บันทึกค่าค้างไว้) ---
        if (current_mode == MODE_LEARN && !mode_select_active && !ir_captured)
        {
            uint32_t new_ir_code = 0;
            if (ir_recv_poll(&new_ir_code))
            {
                captured_code = new_ir_code;
                ir_captured = 1;
                printf("\r\n[IR RX] >>> Received Signal: 0x%08lX <<<\r\n", captured_code);

                // เปลี่ยนหน้าจอกลางเป็นหน้ารหัสที่อ่านได้ทันที
                oled_render_ir_captured_screen(profile_names[current_profile], captured_code);
            }
        }

        // --- 2. สแกนปุ่มกด Keypad 4x4 ---
        uint8_t key = keypad_scan(&row, &col);

        if (key != 0 && key != last_key)
        {
            last_key = key;
            printf("[Keypad] Pressed: Button %d (Row %d, Col %d)\r\n", key, row, col);

            // กรณีที่ 1: อยู่ในหน้าโชว์รหัส IR ที่เพิ่งอ่านได้ (โหมด LEARN)
            if (ir_captured)
            {
                if (key == 16)
                {
                    // ปุ่ม 16: CANCEL ยกเลิก ไม่เซฟรหัส
                    ir_captured = 0;
                    printf("[IR RX] Learn Cancelled by user\r\n");
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);
                }
                else
                {
                    // กดปุ่มในตาราง 1..15 เพื่อบันทึกรหัสลงปุ่มนั้น
                    int idx = key_to_index(key);
                    if (idx >= 0)
                    {
                        profile_codes[current_profile][idx] = captured_code;
                        ir_captured = 0;
                        printf("[IR RX] >> SAVED 0x%08lX to Button %d (Profile %02d: %s) <<\r\n",
                               captured_code, key, current_profile + 1, profile_names[current_profile]);

                        // กลับมาหน้าตาราง 3x4 พร้อมถมสีปุ่มที่เพิ่งเซฟทันที!
                        get_footer_str(footer_buf, current_mode, current_profile);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 1, profile_codes[current_profile]);
                        Delay_Ms(300);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);
                    }
                }
            }
            // กรณีที่ 2: อยู่ในหน้าเลือกโหมด (Footer กระพริบ)
            else if (mode_select_active)
            {
                if (key == 4)
                {
                    // ปุ่ม 4: UP
                    selected_mode = (RemoteMode)((selected_mode + 2) % 3);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, profile_codes[current_profile]);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN
                    selected_mode = (RemoteMode)((selected_mode + 1) % 3);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, profile_codes[current_profile]);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK ยืนยันโหมด
                    current_mode = selected_mode;
                    mode_select_active = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);
                    printf("[Mode] Switched to: %s\r\n", mode_names[current_mode]);
                }
                else if (key == 16)
                {
                    // ปุ่ม 16: BACK ยกเลิก
                    mode_select_active = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);
                }
            }
            // กรณีที่ 3: อยู่ในโหมดปกติ
            else
            {
                if (key == 16)
                {
                    // ปุ่ม 16: BACK เข้าสู่การเลือกโหมด
                    mode_select_active = 1;
                    selected_mode = current_mode;
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, profile_codes[current_profile]);
                }
                else if (key == 4)
                {
                    // ปุ่ม 4: UP เลื่อน Profile ถัดไป (01 ➡️ 02 ➡️ ... ➡️ 16)
                    current_profile = (current_profile + 1) % 16;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);
                    printf("[Profile] Selected: %02d (%s)\r\n", current_profile + 1, profile_names[current_profile]);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN เลื่อน Profile ก่อนหน้า (16 ⬅️ 15 ⬅️ ...)
                    current_profile = (current_profile + 15) % 16;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, profile_codes[current_profile]);
                    printf("[Profile] Selected: %02d (%s)\r\n", current_profile + 1, profile_names[current_profile]);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK
                    printf("[Action] OK Pressed in Mode %s\r\n", mode_names[current_mode]);
                }
                else
                {
                    // ปุ่มในตาราง 3x4 (1..15)
                    int idx = key_to_index(key);
                    if (idx >= 0)
                    {
                        uint32_t code = profile_codes[current_profile][idx];
                        if (code != 0) {
                            printf("[IR TX] Button %d -> Code: 0x%08lX\r\n", key, code);
                        } else {
                            printf("[IR TX] Button %d -> EMPTY (Not Learned)\r\n", key);
                        }
                    }

                    get_footer_str(footer_buf, current_mode, current_profile);

                    // สั่งกระพริบช่องปุ่ม 3 ครั้ง
                    for (int blink = 0; blink < 3; blink++)
                    {
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 1, profile_codes[current_profile]);
                        Delay_Ms(80);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 0, profile_codes[current_profile]);
                        Delay_Ms(80);
                    }
                }
            }
        }
        else if (key == 0)
        {
            last_key = 0;
        }

        // --- 3. การกระพริบ Footer เมื่ออยู่ในหน้า Mode Select ---
        if (mode_select_active)
        {
            blink_tick++;
            if (blink_tick >= 15)
            {
                blink_tick = 0;
                mode_blink_state = !mode_blink_state;

                if (mode_blink_state)
                {
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, profile_codes[current_profile]);
                }
                else
                {
                    oled_render_grid_screen(profile_names[current_profile], "", 0, 0, profile_codes[current_profile]);
                }
            }
        }

        Delay_Ms(15);
    }
}
