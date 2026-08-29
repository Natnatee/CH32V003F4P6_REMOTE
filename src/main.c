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
    printf("   CH32V003F4P6 Smart Remote (4x4 Keypad)\r\n");
    printf("   OLED I2C: PC6=SCL, PC7=SDA\r\n");
    printf("   Keypad Rows (1..4): PD6, PA1, PA2, PC0\r\n");
    printf("   Keypad Cols (1..4): PC1, PC2, PC3, PC4\r\n");
    printf("=========================================\r\n");

    // 3. เริ่มต้นหน้าจอ OLED และ Keypad Matrix
    ssd1306_init();
    keypad_init();

    // 4. แสดงผลหน้าจอเริ่มต้น (Header = SAMSUNG, ตัวเลขสแตนด์บาย = 0)
    oled_render_remote_screen("SAMSUNG", 0);

    uint8_t last_key = 0;
    uint8_t row = 0, col = 0;

    while (1)
    {
        uint8_t key = keypad_scan(&row, &col);

        if (key != 0 && key != last_key)
        {
            last_key = key;

            printf("[Keypad] Pressed: Button %d (Row %d, Col %d)\r\n", key, row, col);

            // อัปเดตหน้าจอแนวตั้ง: SAMSUNG ด้านบน + ตัวเลขปุ่มกด (1 - 16)
            oled_render_remote_screen("SAMSUNG", key);

            Delay_Ms(150); // Debounce
        }
        else if (key == 0)
        {
            last_key = 0;
        }

        Delay_Ms(10);
    }
}
