#include "ch32fun.h"
#include <stdio.h>

int main()
{
    // กำหนดค่าระบบสัญญาณนาฬิกา 48MHz
    SystemInit();

    // ⚠️ ต้องเรียกฟังก์ชันนี้เสมอเพื่อเปิดระบบ SDI Debug Printf ผ่านขา SWDIO (PD1)
    SetupDebugPrintf();

    // เปิดสัญญาณนาฬิกาให้พอร์ต GPIOC และ GPIOD
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    // ตั้งค่า PC1 เป็น Output Push-Pull (10MHz) สำหรับ Blink LED
    GPIOC->CFGLR &= ~(0xf << (4 * 1));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 1);

    // ตั้งค่า PD4 เป็น Output Push-Pull (10MHz) สำหรับ Blink LED สำรอง
    GPIOD->CFGLR &= ~(0xf << (4 * 4));
    GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4);

    printf("\r\n=========================================\r\n");
    printf("  CH32V003F4P6 Remote Test Project Booted!\r\n");
    printf("  Framework: ch32v003fun | Clock: 48MHz\r\n");
    printf("  Debug Port: SWDIO (PD1) 1-Wire Printf\r\n");
    printf("=========================================\r\n");

    uint32_t count = 0;
    while(1)
    {
        // สั่งเปิด LED (PC1 และ PD4 HIGH)
        GPIOC->BSHR = (1 << 1);
        GPIOD->BSHR = (1 << 4);
        printf("[%lu] LED State: ON\r\n", count);
        Delay_Ms(500);

        // สั่งปิด LED (PC1 และ PD4 LOW)
        GPIOC->BSHR = (1 << (1 + 16));
        GPIOD->BSHR = (1 << (4 + 16));
        printf("[%lu] LED State: OFF\r\n", count);
        Delay_Ms(500);

        count++;
    }
}
