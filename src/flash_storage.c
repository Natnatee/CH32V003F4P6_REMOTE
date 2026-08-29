#include "flash_storage.h"
#include <string.h>
#include <stdio.h>

void flash_storage_init(void) {
    // ปลดล็อก Flash สำหรับการเขียนและลบหน้า (Unlock Flash)
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;
}

void flash_load_profile(uint8_t profile_idx, uint32_t *active_codes) {
    if (profile_idx >= 16 || !active_codes) return;

    uint32_t page_addr = FLASH_PROFILE_BASE_ADDR + (profile_idx * FLASH_PROFILE_PAGE_SIZE);
    const uint32_t *ptr = (const uint32_t *)page_addr;

    // ตรวจสอบ Magic Word ว่าเคยบันทึกโปรไฟล์นี้ไว้แล้วหรือไม่
    if (ptr[12] == FLASH_PROFILE_MAGIC) {
        for (int i = 0; i < 12; i++) {
            active_codes[i] = ptr[i];
        }
    } else {
        // ถ้ายังไม่เคยบันทึก ให้เคลียร์ค่าว่าง (0) ทั้งหมด
        for (int i = 0; i < 12; i++) {
            active_codes[i] = 0;
        }
    }
}

void flash_save_profile(uint8_t profile_idx, const uint32_t *active_codes) {
    if (profile_idx >= 16 || !active_codes) return;

    uint32_t page_addr = FLASH_PROFILE_BASE_ADDR + (profile_idx * FLASH_PROFILE_PAGE_SIZE);
    volatile uint32_t *ptr = (volatile uint32_t *)page_addr;

    // 1. ปลดล็อก Flash
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;

    // 2. ลบ Flash Page (64 ไบต์)
    FLASH->CTLR = CR_PAGE_ER;
    FLASH->ADDR = (intptr_t)ptr;
    FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
    while (FLASH->STATR & FLASH_STATR_BSY);

    // 3. เตรียม Buffer สำหรับเขียน
    FLASH->CTLR = CR_PAGE_PG;
    FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
    FLASH->ADDR = (intptr_t)ptr;
    while (FLASH->STATR & FLASH_STATR_BSY);

    // 4. โหลดข้อมูล 16 Words (12 ปุ่ม + Magic Word + Reserved) เข้าสู่ Buffer
    for (int i = 0; i < 16; i++) {
        if (i < 12) {
            ptr[i] = active_codes[i];
        } else if (i == 12) {
            ptr[i] = FLASH_PROFILE_MAGIC;
        } else {
            ptr[i] = 0;
        }
        FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
        while (FLASH->STATR & FLASH_STATR_BSY);
    }

    // 5. สั่งเขียนข้อมูลลง Flash ROM จริง
    FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
    while (FLASH->STATR & FLASH_STATR_BSY);

    // 6. ล็อก Flash เพื่อความปลอดภัย
    FLASH->CTLR = CR_LOCK_Set;

    printf("[Flash] Successfully saved Profile %02d to address 0x%08lX\r\n", profile_idx + 1, page_addr);
}
