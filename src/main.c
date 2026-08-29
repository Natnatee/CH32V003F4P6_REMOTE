#include "ch32fun.h"
#include <stdio.h>
#include "ssd1306.h"
#include "keypad.h"

void SetupDebugPrintf(void);

int main()
{
    // 1. กำหนดค่า Core Clock 48MHz
    SystemInit();

    // 2. เริ่มต้นระบบ SDI Debug Printf ผ่านขา SWDIO (PD1)
    SetupDebugPrintf();

    // Safety Delay
    Delay_Ms(200);

    printf("\r\n=========================================\r\n");
    printf("   CH32V003 3x4 Grid Remote with Settings\r\n");
    printf("   Grid Keys: 1..3, 5..7, 9..11, 13..15\r\n");
    printf("   Nav Keys : 4(UP), 8(DOWN), 12(OK), 16(BACK)\r\n");
    printf("=========================================\r\n");

    // 3. เริ่มต้นหน้าจอ OLED และ Keypad Matrix
    ssd1306_init();
    keypad_init();

    // 4. แสดงผลหน้าจอเริ่มต้น (ตาราง 3x4 ว่าง, Footer = READY)
    char footer_msg[12] = "READY";
    oled_render_grid_screen("SAMSUNG", footer_msg, 0, 0);

    uint8_t last_key = 0;
    uint8_t row = 0, col = 0;

    while (1)
    {
        uint8_t key = keypad_scan(&row, &col);

        if (key != 0 && key != last_key)
        {
            last_key = key;
            printf("[Keypad] Pressed: Button %d (Row %d, Col %d)\r\n", key, row, col);

            // แยกประเภทปุ่ม: ปุ่ม Setting 4 ปุ่ม (4, 8, 12, 16) vs ปุ่มในตาราง 3x4
            if (key == 4)
            {
                // ปุ่ม 4: UP
                oled_render_grid_screen("SAMSUNG", "NAV: UP", 0, 0);
            }
            else if (key == 8)
            {
                // ปุ่ม 8: DOWN
                oled_render_grid_screen("SAMSUNG", "NAV: DOWN", 0, 0);
            }
            else if (key == 12)
            {
                // ปุ่ม 12: OK
                oled_render_grid_screen("SAMSUNG", "NAV: OK", 0, 0);
            }
            else if (key == 16)
            {
                // ปุ่ม 16: BACK / CANCEL
                oled_render_grid_screen("SAMSUNG", "NAV: BACK", 0, 0);
            }
            else
            {
                // ปุ่มในตาราง 3x4 (เช่น ปุ่ม 1): ทำเอฟเฟกต์กระพริบช่องปุ่มนั้น
                char msg[12];
                if (key < 10) {
                    msg[0] = 'K'; msg[1] = 'E'; msg[2] = 'Y'; msg[3] = ':'; msg[4] = ' ';
                    msg[5] = '0' + key; msg[6] = '\0';
                } else {
                    msg[0] = 'K'; msg[1] = 'E'; msg[2] = 'Y'; msg[3] = ':'; msg[4] = ' ';
                    msg[5] = '0' + (key / 10); msg[6] = '0' + (key % 10); msg[7] = '\0';
                }

                // สั่งกระพริบช่องปุ่ม 3 ครั้ง
                for (int blink = 0; blink < 3; blink++)
                {
                    // ไฮไลต์ (พื้นขาว อักษรดำ)
                    oled_render_grid_screen("SAMSUNG", msg, key, 1);
                    Delay_Ms(120);

                    // ปกติ (พื้นดำ อักษรขาว)
                    oled_render_grid_screen("SAMSUNG", msg, key, 0);
                    Delay_Ms(120);
                }
            }
        }
        else if (key == 0)
        {
            last_key = 0;
        }

        Delay_Ms(10);
    }
}
