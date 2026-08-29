#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "keypad.h"

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
    printf("   CH32V003 Smart Remote with 16 Profiles\r\n");
    printf("   Modes: SEND / LEARN / NEW\r\n");
    printf("=========================================\r\n");

    // 3. เริ่มต้นหน้าจอ OLED และ Keypad Matrix
    ssd1306_init();
    keypad_init();

    // ตัวแปรสถานะระบบ
    RemoteMode current_mode = MODE_SEND;
    RemoteMode selected_mode = MODE_SEND;
    uint8_t current_profile = 0; // 0 = SAMSUNG (Profile 01)
    uint8_t mode_select_active = 0;

    char footer_buf[16];
    get_footer_str(footer_buf, current_mode, current_profile);
    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0);

    uint8_t last_key = 0;
    uint8_t row = 0, col = 0;
    uint32_t blink_tick = 0;
    uint8_t mode_blink_state = 1;

    while (1)
    {
        uint8_t key = keypad_scan(&row, &col);

        // --- 1. จัดการปุ่มกด (Key Pressed) ---
        if (key != 0 && key != last_key)
        {
            last_key = key;
            printf("[Keypad] Pressed: Button %d (Row %d, Col %d)\r\n", key, row, col);

            if (mode_select_active)
            {
                // === อยู่ในหน้าเลือกโหมด (Footer กระพริบ) ===
                if (key == 4)
                {
                    // ปุ่ม 4: UP (เลื่อนโหมดก่อนหน้า)
                    selected_mode = (RemoteMode)((selected_mode + 2) % 3);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN (เลื่อนโหมดถัดไป)
                    selected_mode = (RemoteMode)((selected_mode + 1) % 3);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK (ยืนยันเข้าโหมดที่เลือก)
                    current_mode = selected_mode;
                    mode_select_active = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0);
                    printf("[Mode] Switched to: %s\r\n", mode_names[current_mode]);
                }
                else if (key == 16)
                {
                    // ปุ่ม 16: BACK (ยกเลิก กลับสู่โหมดเดิม)
                    mode_select_active = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0);
                }
            }
            else
            {
                // === อยู่ในโหมดปกติ (SEND / LEARN / NEW) ===
                if (key == 16)
                {
                    // ปุ่ม 16: BACK (เข้าสู่การเลือกโหมด)
                    mode_select_active = 1;
                    selected_mode = current_mode;
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0);
                }
                else if (key == 4)
                {
                    // ปุ่ม 4: UP (เลื่อน Profile ถัดไป: 01 ➡️ 02 ➡️ ... ➡️ 16)
                    current_profile = (current_profile + 1) % 16;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0);
                    printf("[Profile] Selected: %02d (%s)\r\n", current_profile + 1, profile_names[current_profile]);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN (เลื่อน Profile ก่อนหน้า: 16 ⬅️ 15 ⬅️ ...)
                    current_profile = (current_profile + 15) % 16;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0);
                    printf("[Profile] Selected: %02d (%s)\r\n", current_profile + 1, profile_names[current_profile]);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK
                    printf("[Action] OK Pressed in Mode %s\r\n", mode_names[current_mode]);
                }
                else
                {
                    // ปุ่มในตาราง 3x4 (1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15)
                    get_footer_str(footer_buf, current_mode, current_profile);

                    // สั่งกระพริบช่องปุ่ม 3 ครั้ง
                    for (int blink = 0; blink < 3; blink++)
                    {
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 1);
                        Delay_Ms(100);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 0);
                        Delay_Ms(100);
                    }
                }
            }
        }
        else if (key == 0)
        {
            last_key = 0;
        }

        // --- 2. การกระพริบ Footer เมื่ออยู่ในหน้า Mode Select ---
        if (mode_select_active)
        {
            blink_tick++;
            if (blink_tick >= 15) // สลับทุก ~300ms
            {
                blink_tick = 0;
                mode_blink_state = !mode_blink_state;

                if (mode_blink_state)
                {
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0);
                }
                else
                {
                    // ดับตัวหนังสือ Footer เพื่อให้กระพริบ
                    oled_render_grid_screen(profile_names[current_profile], "", 0, 0);
                }
            }
        }

        Delay_Ms(20);
    }
}
