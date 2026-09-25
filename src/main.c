#include "ch32fun.h"
#include <string.h>
#include "ssd1306.h"
#include "keypad.h"
#include "ir_recv.h"
#include "ir_send.h"
#include "flash_storage.h"
#include "ir_database.h"
#include "tetris.h"
#include "calc.h"
#include "ina226.h"

// 6 โหมดการทำงานหลัก
typedef enum {
    MODE_SEND = 0,
    MODE_LEARN = 1,
    MODE_RENAME = 2,
    MODE_TETRIS = 3,
    MODE_CALC = 4,
    MODE_METER = 5
} RemoteMode;

static const char *mode_names[6] = {
    "SEND",
    "LRN ",
    "NAME",
    "TETR",
    "CALC",
    "METR"
};

static const char *mode_select_labels[6] = {
    "[ SEND ]",
    "[ LEARN ]",
    "[ RENAME ]",
    "[ TETRIS ]",
    "[  CALC  ]",
    "[ METER ]"
};

// บัฟเฟอร์ใน RAM สำหรับถือเฉพาะโปรไฟล์ปัจจุบันที่กำลังใช้งาน (12 ปุ่ม = 48 ไบต์เท่านั้น!)
static uint32_t active_codes[12];
static char current_profile_name[8];
static uint8_t active_protocol = 0;
static uint8_t active_bits = 0;

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

static void send_profile_button(uint8_t profile_idx, uint8_t idx, uint8_t repeat) {
    uint32_t code;
    uint8_t protocol, bits;
    uint8_t apple_command = 0;
    if (profile_idx == 7 && active_codes[idx] == 0) {
        // Apple aluminum remote: EE 87 command 59 on the wire (NEC LSB-first).
        static const uint8_t apple_commands[12] = {
            0x5E, 0x0B, 0, 0x08, 0x5D, 0x07,
            0x02, 0x0D, 0, 0, 0, 0
        };
        apple_command = apple_commands[idx];
        code = apple_command ? 0x590087EEUL | ((uint32_t)apple_command << 16) : 0;
        protocol = IR_PROTOCOL_NEC;
    } else {
        flash_decode_ir_button(idx, active_codes[idx], &code, &protocol, &bits);
    }
    if (code == 0) return;

    if (protocol == IR_PROTOCOL_SHARP) ir_send_sharp((uint16_t)code);
    else if (protocol == IR_PROTOCOL_NEC) {
        if (repeat) ir_send_nec_repeat(); else ir_send_nec(code);
    } else if (protocol == IR_PROTOCOL_SAMSUNG) ir_send_samsung(code);
    else if (protocol == IR_PROTOCOL_LG) ir_send_lg(code);
    else if (protocol == IR_PROTOCOL_SONY) ir_send_sony(code, bits);
    else if (protocol == IR_PROTOCOL_JVC) ir_send_jvc((uint16_t)code);
    if (!repeat && apple_command >= 0x5D) ir_send_nec(0x590487EEUL);
}

// ตารางตัวอักษรสำหรับปุ่ม 1..15 ในโหมด RENAME
static const char *key_letters[12] = {
    "AB",       // Key 1
    "CD",       // Key 2
    "EF",       // Key 3
    "GH",       // Key 5
    "IJ",       // Key 6
    "KL",       // Key 7
    "MN",       // Key 9
    "OP",       // Key 10
    "QR",       // Key 11
    "ST",       // Key 13
    "UV",       // Key 14
    "WXYZ "     // Key 15 (มี Space/ลบ)
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

    // Safety Delay
    Delay_Ms(200);

    // 2. เริ่มต้นฮาร์ดแวร์ OLED, Keypad, Flash Storage, IR RX & IR TX
    ssd1306_init();
    keypad_init();
    flash_storage_init();
    ir_recv_init();
    ir_send_init();

    // ตัวแปรสถานะระบบ
    RemoteMode current_mode = MODE_SEND;
    RemoteMode selected_mode = MODE_SEND;
    uint8_t current_profile = 0; // 0 = Profile 01
    uint8_t mode_select_active = 0;

    // โหลดข้อมูลปุ่มและชื่อของ Profile 01 จาก Flash ROM เข้าสู่ RAM
    flash_load_profile(current_profile, active_codes, current_profile_name, &active_protocol, &active_bits);

    // ตัวแปรสำหรับโหมด LEARN
    uint8_t ir_captured = 0;
    uint32_t captured_code = 0;

    // ตัวแปรสำหรับโหมด RENAME
    uint8_t rename_editing = 0;
    uint8_t rename_cursor = 0;
    char edit_name_buf[8] = {0};

    char footer_buf[16];
    get_footer_str(footer_buf, current_mode, current_profile);
    oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);

    uint8_t last_key = 0;
    uint8_t hold_ticks = 0;
    uint8_t row = 0, col = 0;
    uint32_t blink_tick = 0;
    uint8_t mode_blink_state = 1;

    while (1)
    {
        // --- 0.1 โหมดพิเศษ: TETRIS GAME ---
        if (current_mode == MODE_TETRIS && !mode_select_active)
        {
            uint8_t key = keypad_scan(&row, &col);
            uint8_t is_new = (key != 0 && key != last_key);
            if (key != 0) {
                last_key = key;
                tetris_handle_key(key, is_new);
            } else {
                last_key = 0;
                tetris_handle_key(0, 0);
            }

            if (tetris_should_exit()) {
                mode_select_active = 1;
                selected_mode = current_mode;
                mode_blink_state = 1;
                blink_tick = 0;
                oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
            } else {
                tetris_update();
                tetris_render();
            }

            Delay_Ms(20);
            continue;
        }

        // --- 0.1 โหมดพิเศษ: CALCULATOR ---
        if (current_mode == MODE_CALC && !mode_select_active)
        {
            uint8_t key = keypad_scan(&row, &col);
            if (key != 0 && key != last_key) {
                last_key = key;
                calc_handle_key(key);
                calc_render();
            } else if (key == 0) {
                last_key = 0;
            }

            if (calc_should_exit()) {
                mode_select_active = 1;
                selected_mode = current_mode;
                mode_blink_state = 1;
                blink_tick = 0;
                oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
            }

            Delay_Ms(20);
            continue;
        }

        // --- 0.2 โหมดพิเศษ: MULTIMETER (INA226) ---
        if (current_mode == MODE_METER && !mode_select_active)
        {
            uint8_t key = keypad_scan(&row, &col);
            if (key != 0 && key != last_key) {
                last_key = key;
                ina226_handle_key(key);
            } else if (key == 0) {
                last_key = 0;
            }

            if (ina226_should_exit()) {
                mode_select_active = 1;
                selected_mode = current_mode;
                mode_blink_state = 1;
                blink_tick = 0;
                oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
            } else {
                ina226_update();
                ina226_render();
            }

            Delay_Ms(50);
            continue;
        }

        // --- 1. ตรวจจับสัญญาณ IR ในโหมด LEARN ---
        if (current_mode == MODE_LEARN && !mode_select_active && !ir_captured)
        {
            uint32_t new_ir_code = 0;
            if (ir_recv_poll(&new_ir_code))
            {
                captured_code = new_ir_code;
                active_protocol = ir_recv_last_protocol();
                active_bits = ir_recv_last_bits();
                ir_captured = 1;
                oled_render_ir_captured_screen(current_profile_name, captured_code);
            }
        }

        // --- 3. สแกนปุ่มกด Keypad 4x4 ---
        uint8_t key = keypad_scan(&row, &col);

        if (key != 0 && key != last_key)
        {
            last_key = key;
            hold_ticks = 0;

            // ==========================================
            // กรณีที่ 1: หน้าแสดงรหัส IR ที่อ่านได้ (โหมด LEARN)
            // ==========================================
            if (ir_captured)
            {
                if (key == 16)
                {
                    // ปุ่ม 16: CANCEL ยกเลิก ไม่เซฟรหัส
                    ir_captured = 0;
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                }
                else
                {
                    // กดปุ่มในตาราง 1..15 เพื่อบันทึกรหัสลง Flash ROM ทันที
                    int idx = key_to_index(key);
                    if (idx >= 0)
                    {
                        active_codes[idx] = flash_encode_ir_button((uint8_t)idx, captured_code,
                                                                   active_protocol, active_bits);
                        flash_save_profile(current_profile, active_codes, current_profile_name, active_protocol, active_bits);
                        ir_captured = 0;

                        get_footer_str(footer_buf, current_mode, current_profile);
                        oled_render_grid_screen(current_profile_name, footer_buf, key, 1, active_codes);
                        Delay_Ms(300);
                        oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                    }
                }
            }
            // ==========================================
            // กรณีที่ 3: โหมด RENAME (แก้ไขชื่อโปรไฟล์)
            // ==========================================
            else if (current_mode == MODE_RENAME && !mode_select_active)
            {
                if (rename_editing == 0)
                {
                    // --- ขั้นตอนเลือก Profile ที่ต้องการแก้ชื่อ ---
                    if (key == 4)
                    {
                        // ปุ่ม 4: UP เลื่อน Profile ถัดไป
                        current_profile = (current_profile + 1) % TOTAL_PROFILES_COUNT;
                        flash_load_profile(current_profile, active_codes, current_profile_name, &active_protocol, &active_bits);
                        oled_render_rename_screen(current_profile_name, 0, 0, current_profile, 0);
                    }
                    else if (key == 8)
                    {
                        // ปุ่ม 8: DOWN เลื่อน Profile ก่อนหน้า
                        current_profile = (current_profile + TOTAL_PROFILES_COUNT - 1) % TOTAL_PROFILES_COUNT;
                        flash_load_profile(current_profile, active_codes, current_profile_name, &active_protocol, &active_bits);
                        oled_render_rename_screen(current_profile_name, 0, 0, current_profile, 0);
                    }
                    else if (key == 12)
                    {
                        // ปุ่ม 12: OK ยืนยันเข้าสู่การพิมพ์/แก้ไขชื่อ (Cursor Active)
                        rename_editing = 1;
                        rename_cursor = 0;
                        memset(edit_name_buf, ' ', 7);
                        edit_name_buf[7] = '\0';
                        int cur_len = strlen(current_profile_name);
                        for (int i = 0; i < cur_len && i < 7; i++) {
                            edit_name_buf[i] = current_profile_name[i];
                        }
                        oled_render_rename_screen(edit_name_buf, rename_cursor, 1, current_profile, 1);
                    }
                    else if (key == 16)
                    {
                        // ปุ่ม 16: BACK เข้าสู่การเลือกโหมด
                        mode_select_active = 1;
                        selected_mode = current_mode;
                        mode_blink_state = 1;
                        blink_tick = 0;
                        oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
                    }
                }
                else
                {
                    // --- ขั้นตอนกำลังพิมพ์ชื่อ (Cursor Active เลื่อนได้ครบทั้ง 7 ช่อง) ---
                    if (key == 4)
                    {
                        // ปุ่ม 4: เลื่อน Cursor ซ้าย (-1)
                        rename_cursor = (rename_cursor + 6) % 7;
                        oled_render_rename_screen(edit_name_buf, rename_cursor, 1, current_profile, 1);
                    }
                    else if (key == 8)
                    {
                        // ปุ่ม 8: เลื่อน Cursor ขวา (+1)
                        rename_cursor = (rename_cursor + 1) % 7;
                        oled_render_rename_screen(edit_name_buf, rename_cursor, 1, current_profile, 1);
                    }
                    else if (key == 12)
                    {
                        // ปุ่ม 12: OK บันทึกชื่อลง Flash ROM ถาวร!
                        for (int i = 6; i >= 0; i--) {
                            if (edit_name_buf[i] == ' ') edit_name_buf[i] = '\0';
                            else break;
                        }
                        if (strlen(edit_name_buf) == 0) {
                            strncpy(edit_name_buf, profile_names[current_profile], 7);
                        }
                        strncpy(current_profile_name, edit_name_buf, 7);
                        current_profile_name[7] = '\0';

                        flash_save_profile(current_profile, active_codes, current_profile_name, active_protocol, active_bits);
                        rename_editing = 0;

                        oled_render_rename_screen(current_profile_name, 0, 0, current_profile, 0);
                    }
                    else if (key == 16)
                    {
                        // ปุ่ม 16: CANCEL ยกเลิก ไม่เซฟชื่อ
                        rename_editing = 0;
                        oled_render_rename_screen(current_profile_name, 0, 0, current_profile, 0);
                    }
                    else
                    {
                        // ปุ่มในตาราง 1..15: วนตัวอักษรลงที่ตำแหน่ง Cursor
                        int idx = key_to_index(key);
                        if (idx >= 0)
                        {
                            const char *letters = key_letters[idx];
                            int let_len = strlen(letters);

                            char cur_c = edit_name_buf[rename_cursor];
                            int found_pos = -1;
                            for (int i = 0; i < let_len; i++) {
                                if (letters[i] == cur_c) {
                                    found_pos = i;
                                    break;
                                }
                            }

                            int next_pos = (found_pos + 1) % let_len;
                            edit_name_buf[rename_cursor] = letters[next_pos];

                            oled_render_rename_screen(edit_name_buf, rename_cursor, 1, current_profile, 1);
                        }
                    }
                }
            }
            // ==========================================
            // กรณีที่ 4: หน้าเลือกโหมด (Footer กระพริบ)
            // ==========================================
            else if (mode_select_active)
            {
                if (key == 4)
                {
                    // ปุ่ม 4: UP
                    selected_mode = (RemoteMode)((selected_mode + 5) % 6);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN
                    selected_mode = (RemoteMode)((selected_mode + 1) % 6);
                    mode_blink_state = 1;
                    blink_tick = 0;
                    oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK ยืนยันโหมด
                    current_mode = selected_mode;
                    mode_select_active = 0;
                    rename_editing = 0;

                    if (current_mode == MODE_TETRIS) {
                        tetris_init();
                        tetris_render();
                    } else if (current_mode == MODE_CALC) {
                        calc_init();
                        calc_render();
                    } else if (current_mode == MODE_METER) {
                        ina226_init();
                        ina226_render();
                    } else if (current_mode == MODE_RENAME) {
                        oled_render_rename_screen(current_profile_name, 0, 0, current_profile, 0);
                    } else {
                        get_footer_str(footer_buf, current_mode, current_profile);
                        oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                    }
                }
                else if (key == 16)
                {
                    // ปุ่ม 16: BACK ยกเลิก
                    mode_select_active = 0;
                    if (current_mode == MODE_RENAME) {
                        oled_render_rename_screen(current_profile_name, 0, 0, current_profile, 0);
                    } else if (current_mode == MODE_CALC) {
                        calc_render();
                    } else if (current_mode == MODE_TETRIS) {
                        tetris_render();
                    } else if (current_mode == MODE_METER) {
                        ina226_render();
                    } else {
                        get_footer_str(footer_buf, current_mode, current_profile);
                        oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                    }
                }
            }
            // ==========================================
            // กรณีที่ 5: อยู่ในหน้าตาราง 3x4 ปกติ (SEND / LEARN)
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
                    oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else if (key == 4)
                {
                    // ปุ่ม 4: UP เลื่อน Profile ถัดไป (01 ➡️ 02 ➡️ ... ➡️ 16)
                    current_profile = (current_profile + 1) % TOTAL_PROFILES_COUNT;
                    flash_load_profile(current_profile, active_codes, current_profile_name, &active_protocol, &active_bits);
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                }
                else if (key == 8)
                {
                    // ปุ่ม 8: DOWN เลื่อน Profile ก่อนหน้า (16 ⬅️ 15 ⬅️ ...)
                    current_profile = (current_profile + TOTAL_PROFILES_COUNT - 1) % TOTAL_PROFILES_COUNT;
                    flash_load_profile(current_profile, active_codes, current_profile_name, &active_protocol, &active_bits);
                    get_footer_str(footer_buf, current_mode, current_profile);
                    oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                }
                else if (key == 12)
                {
                    // ปุ่ม 12: OK (สงวนไว้สำหรับฟังก์ชัน Profile/IR ในอนาคต)
                }
                else
                {
                    // ปุ่มในตาราง 3x4 (1..15)
                    int idx = key_to_index(key);
                    if (idx >= 0)
                    {
                        send_profile_button(current_profile, (uint8_t)idx, 0);
                    }

                    get_footer_str(footer_buf, current_mode, current_profile);

                    oled_render_grid_screen(current_profile_name, footer_buf, 0, 0, active_codes);
                }
            }
        }
        else if (key != 0 && current_mode == MODE_SEND && !mode_select_active)
        {
            int idx = key_to_index(key);
            if (idx >= 0 && ++hold_ticks >= 3) {
                hold_ticks = 0;
                send_profile_button(current_profile, (uint8_t)idx, 1);
            }
        }
        else if (key == 0)
        {
            last_key = 0;
            hold_ticks = 0;
        }

        // --- 4. การกระพริบ Cursor ในโหมด RENAME และกระพริบ Footer ในหน้า Mode Select ---
        blink_tick++;
        if (blink_tick >= 15)
        {
            blink_tick = 0;
            mode_blink_state = !mode_blink_state;

            if (mode_select_active)
            {
                if (mode_blink_state)
                {
                    oled_render_grid_screen(current_profile_name, mode_select_labels[selected_mode], 0, 0, active_codes);
                }
                else
                {
                    oled_render_grid_screen(current_profile_name, "", 0, 0, active_codes);
                }
            }
            else if (current_mode == MODE_RENAME && rename_editing)
            {
                oled_render_rename_screen(edit_name_buf, rename_cursor, mode_blink_state, current_profile, 1);
            }
        }

        Delay_Ms(20);
    }
}
