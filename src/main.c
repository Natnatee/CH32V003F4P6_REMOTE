#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "keypad.h"
#include "ir_recv.h"
#include "ir_send.h"
#include "flash_storage.h"
#include "ir_database.h"

void SetupDebugPrintf(void);

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

// บัฟเฟอร์ใน RAM สำหรับถือเฉพาะโปรไฟล์ปัจจุบันที่กำลังใช้งาน (12 ปุ่ม = 48 ไบต์เท่านั้น!)
static uint32_t active_codes[12];

// แปลงหมายเลขปุ่ม (1..15) เป็น Index ในตาราง (0..11)
static int key_to_index(uint8_t key) {
    switch (key) {
        case 1:  return 0;  // 1: ON / Power
        case 2:  return 1;  // 2: UP
        case 3:  return 2;  // 3: OFF / MUTE
        case 5:  return 3;  // 5: LEFT
        case 6:  return 4;  // 6: OK
        case 7:  return 5;  // 7: RIGHT
        case 9:  return 6;  // 9: BACK
        case 10: return 7;  // 10: DOWN
        case 11: return 8;  // 11: HOME / MENU
        case 13: return 9;  // 13: INPUT / SOURCE
        case 14: return 10; // 14: VOL-
        case 15: return 11; // 15: VOL+
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
    printf("   CH32V003 Smart Remote + Mode NEW\r\n");
    printf("   IR RX Pin : PD0 (VS1838B)\r\n");
    printf("   IR TX Pin : PD4 (38kHz PWM)\r\n");
    printf("   Flash NVM : 16 Profiles @ 0x08003C00\r\n");
    printf("=========================================\r\n");

    // 3. เริ่มต้นฮาร์ดแวร์ OLED, Keypad, Flash Storage, IR RX & IR TX
    ssd1306_init();
    keypad_init();
    flash_storage_init();
    ir_recv_init();
    ir_send_init();

    // ตัวแปรสถานะระบบ
    RemoteMode current_mode = MODE_SEND;
    RemoteMode selected_mode = MODE_SEND;
    uint8_t current_profile = 0; // 0 = SAMSUNG (Profile 01)
    uint8_t mode_select_active = 0;

    // โหลดข้อมูลปุ่มของ Profile 01 จาก Flash ROM เข้าสู่ RAM
    flash_load_profile(current_profile, active_codes);

    // ตัวแปรสำหรับโหมด LEARN
    uint8_t ir_captured = 0;
    uint32_t captured_code = 0;

    // ตัวแปรสำหรับโหมด NEW (Brute-Force Generator)
    uint16_t new_brute_idx = 0;
    uint8_t new_preview_active = 0;
    uint32_t new_preview_ticks = 0;

    char footer_buf[16];
    get_footer_str(footer_buf, current_mode, current_profile);
    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);

    uint8_t last_key = 0;
    uint8_t row = 0, col = 0;
    uint32_t blink_tick = 0;
    uint8_t mode_blink_state = 1;

    while (1)
    {
        // --- 1. ตรวจจับสัญญาณ IR ในโหมด LEARN ---
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

        // --- 2. นับถอยหลัง 5 วินาทีในโหมด NEW Preview ---
        if (current_mode == MODE_NEW && new_preview_active && !mode_select_active)
        {
            new_preview_ticks++;
            if (new_preview_ticks >= 250) // 250 * 20ms = 5,000ms (5 วินาที)
            {
                new_preview_active = 0;
                new_preview_ticks = 0;
                get_footer_str(footer_buf, current_mode, current_profile);
                oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
            }
        }

        // --- 3. สแกนปุ่มกด Keypad 4x4 ---
        uint8_t key = keypad_scan(&row, &col);

        if (key != 0 && key != last_key)
        {
            last_key = key;
            printf("[Keypad] Pressed: Button %d (Row %d, Col %d)\r\n", key, row, col);

            // ==========================================
            // กรณีที่ 1: หน้าแสดงรหัส IR ที่อ่านได้ (โหมด LEARN)
            // ==========================================
            if (ir_captured)
            {
                if (key == 16)
                {
                    // ปุ่ม 16: CANCEL ยกเลิก ไม่เซฟรหัส
                    ir_captured = 0;
                    printf("[IR RX] Learn Cancelled by user\r\n");
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                }
                else
                {
                    // กดปุ่มในตาราง 1..15 เพื่อบันทึกรหัสลง Flash ROM ทันที
                    int idx = key_to_index(key);
                    if (idx >= 0)
                    {
                        active_codes[idx] = captured_code;
                        flash_save_profile(current_profile, active_codes); // บันทึกลง Flash ถาวร!
                        ir_captured = 0;

                        printf("[IR RX] >> SAVED 0x%08lX to Button %d (Profile %02d: %s) <<\r\n",
                               captured_code, key, current_profile + 1, profile_names[current_profile]);

                        // กลับมาหน้าตาราง 3x4 พร้อมถมสีปุ่มที่เพิ่งเซฟ
                        get_footer_str(footer_buf, current_mode, current_profile);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 1, active_codes);
                        Delay_Ms(300);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                    }
                }
            }
            // ==========================================
            // กรณีที่ 2: หน้ากวาดยิงรหัส (โหมด NEW Preview 5 วินาที)
            // ==========================================
            else if (current_mode == MODE_NEW && new_preview_active && !mode_select_active)
            {
                if (key == 4)
                {
                    // ปุ่ม 4: UP เลื่อนรหัสถัดไป (+1)
                    new_brute_idx = (new_brute_idx + 1) % TOTAL_BRUTE_COMMANDS;
                    uint32_t test_code = generate_brute_code(current_profile, (uint8_t)new_brute_idx);
                    ir_send_code(test_code);
                    oled_render_new_code_screen(profile_names[current_profile], test_code, new_brute_idx, TOTAL_BRUTE_COMMANDS);
                    new_preview_ticks = 0;
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN เลื่อนรหัสก่อนหน้า (-1)
                    new_brute_idx = (new_brute_idx + TOTAL_BRUTE_COMMANDS - 1) % TOTAL_BRUTE_COMMANDS;
                    uint32_t test_code = generate_brute_code(current_profile, (uint8_t)new_brute_idx);
                    ir_send_code(test_code);
                    oled_render_new_code_screen(profile_names[current_profile], test_code, new_brute_idx, TOTAL_BRUTE_COMMANDS);
                    new_preview_ticks = 0;
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK ยิงรหัสเดิมซ้ำอีกรอบ (RETRY)
                    uint32_t test_code = generate_brute_code(current_profile, (uint8_t)new_brute_idx);
                    ir_send_code(test_code);
                    oled_render_new_code_screen(profile_names[current_profile], test_code, new_brute_idx, TOTAL_BRUTE_COMMANDS);
                    new_preview_ticks = 0;
                }
                else if (key == 16)
                {
                    // ปุ่ม 16: BACK ยกเลิกหน้านี้ กลับสู่ตารางทันที
                    new_preview_active = 0;
                    new_preview_ticks = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                }
                else
                {
                    // กดปุ่มในตาราง 1..15 เพื่อบันทึกรหัสที่เพิ่งยิงลง Flash ROM!
                    int idx = key_to_index(key);
                    if (idx >= 0)
                    {
                        uint32_t test_code = generate_brute_code(current_profile, (uint8_t)new_brute_idx);
                        active_codes[idx] = test_code;
                        flash_save_profile(current_profile, active_codes); // เซฟลง Flash!
                        new_preview_active = 0;
                        new_preview_ticks = 0;

                        printf("[NEW Mode] >> SAVED Code 0x%08lX to Button %d (Profile %02d: %s) <<\r\n",
                               test_code, key, current_profile + 1, profile_names[current_profile]);

                        get_footer_str(footer_buf, current_mode, current_profile);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 1, active_codes);
                        Delay_Ms(300);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                    }
                }
            }
            // ==========================================
            // กรณีที่ 3: หน้าเลือกโหมด (Footer กระพริบ)
            // ==========================================
            else if (mode_select_active)
            {
                if (key == 4)
                {
                    // ปุ่ม 4: UP
                    selected_mode = (RemoteMode)((selected_mode + 2) % 3);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN
                    selected_mode = (RemoteMode)((selected_mode + 1) % 3);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK ยืนยันโหมด
                    current_mode = selected_mode;
                    mode_select_active = 0;
                    new_preview_active = 0;
                    new_brute_idx = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                    printf("[Mode] Switched to: %s\r\n", mode_names[current_mode]);
                }
                else if (key == 16)
                {
                    // ปุ่ม 16: BACK ยกเลิก
                    mode_select_active = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                }
            }
            // ==========================================
            // กรณีที่ 4: อยู่ในหน้าตาราง 3x4 ปกติ
            // ==========================================
            else
            {
                if (key == 16)
                {
                    // ปุ่ม 16: BACK เข้าสู่การเลือกโหมด
                    mode_select_active = 1;
                    selected_mode = current_mode;
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else if (current_mode == MODE_NEW && (key == 4 || key == 8))
                {
                    // ในโหมด NEW: กด 4 (UP) หรือ 8 (DOWN) เพื่อเริ่มกวาดยิงรหัส 256 คำสั่ง
                    if (key == 4) {
                        new_brute_idx = (new_brute_idx + 1) % TOTAL_BRUTE_COMMANDS;
                    } else {
                        new_brute_idx = (new_brute_idx + TOTAL_BRUTE_COMMANDS - 1) % TOTAL_BRUTE_COMMANDS;
                    }

                    uint32_t test_code = generate_brute_code(current_profile, (uint8_t)new_brute_idx);
                    ir_send_code(test_code);
                    oled_render_new_code_screen(profile_names[current_profile], test_code, new_brute_idx, TOTAL_BRUTE_COMMANDS);
                    new_preview_active = 1;
                    new_preview_ticks = 0;
                }
                else if (key == 4)
                {
                    // ปุ่ม 4: UP เลื่อน Profile ถัดไป (01 ➡️ 02 ➡️ ... ➡️ 16)
                    current_profile = (current_profile + 1) % TOTAL_PROFILES_COUNT;
                    flash_load_profile(current_profile, active_codes); // โหลดโปรไฟล์จาก Flash
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                    printf("[Profile] Loaded: %02d (%s)\r\n", current_profile + 1, profile_names[current_profile]);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN เลื่อน Profile ก่อนหน้า (16 ⬅️ 15 ⬅️ ...)
                    current_profile = (current_profile + TOTAL_PROFILES_COUNT - 1) % TOTAL_PROFILES_COUNT;
                    flash_load_profile(current_profile, active_codes); // โหลดโปรไฟล์จาก Flash
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(profile_names[current_profile], footer_buf, 0, 0, active_codes);
                    printf("[Profile] Loaded: %02d (%s)\r\n", current_profile + 1, profile_names[current_profile]);
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
                        uint32_t code = active_codes[idx];

                        // สั่งยิงสัญญาณ IR 38kHz ออกขา PD4 ทันทีถ้ามีโค้ด
                        if (code != 0) {
                            ir_send_code(code);
                        } else {
                            printf("[IR TX] Button %d is EMPTY (No Code)\r\n", key);
                        }
                    }

                    get_footer_str(footer_buf, current_mode, current_profile);

                    // สั่งกระพริบช่องปุ่ม 3 ครั้ง
                    for (int blink = 0; blink < 3; blink++)
                    {
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 1, active_codes);
                        Delay_Ms(70);
                        oled_render_grid_screen(profile_names[current_profile], footer_buf, key, 0, active_codes);
                        Delay_Ms(70);
                    }
                }
            }
        }
        else if (key == 0)
        {
            last_key = 0;
        }

        // --- 4. การกระพริบ Footer เมื่ออยู่ในหน้า Mode Select ---
        if (mode_select_active)
        {
            blink_tick++;
            if (blink_tick >= 15)
            {
                blink_tick = 0;
                mode_blink_state = !mode_blink_state;

                if (mode_blink_state)
                {
                    oled_render_grid_screen(profile_names[current_profile], mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else
                {
                    oled_render_grid_screen(profile_names[current_profile], "", 0, 0, active_codes);
                }
            }
        }

        Delay_Ms(20);
    }
}
