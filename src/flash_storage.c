#include "flash_storage.h"
#include "ir_database.h"
#include <string.h>

void flash_storage_init(void) {
    // ปลดล็อก Flash สำหรับการเขียนและลบหน้า (Unlock Flash)
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;
}

void flash_load_profile(uint8_t profile_idx, uint32_t *active_codes, char *active_name,
                        uint8_t *active_protocol, uint8_t *active_bits) {
    if (profile_idx >= TOTAL_PROFILES_COUNT) return;

    uint32_t page_addr = FLASH_PROFILE_BASE_ADDR + (profile_idx * FLASH_PROFILE_PAGE_SIZE);
    const uint32_t *ptr = (const uint32_t *)page_addr;

    // 1. โหลดรหัสปุ่ม 12 ปุ่ม
    if (active_codes) {
        if (ptr[12] == FLASH_PROFILE_MAGIC) {
            for (int i = 0; i < 12; i++) {
                active_codes[i] = ptr[i];
            }
        } else {
            // Legacy profile: ล้างรหัสเก่า แต่คงชื่อเดิมไว้ด้านล่าง
            for (int i = 0; i < 12; i++) {
                active_codes[i] = 0;
            }
        }
    }

    // 2. โหลดชื่อโปรไฟล์
    if (active_name) {
        if ((ptr[12] == FLASH_PROFILE_MAGIC || ptr[12] == FLASH_PROFILE_LEGACY_MAGIC) &&
            ptr[13] != 0 && ptr[13] != 0xFFFFFFFF) {
            memcpy(active_name, (const char *)&ptr[13], 7);
            active_name[7] = '\0';
        } else {
            strncpy(active_name, profile_names[profile_idx], 7);
            active_name[7] = '\0';
        }
    }

    if (active_protocol) *active_protocol = (ptr[12] == FLASH_PROFILE_MAGIC) ? (uint8_t)ptr[15] : 0;
    if (active_bits) *active_bits = (ptr[12] == FLASH_PROFILE_MAGIC) ? (uint8_t)(ptr[15] >> 8) : 0;
}

void flash_save_profile(uint8_t profile_idx, const uint32_t *active_codes, const char *active_name,
                        uint8_t active_protocol, uint8_t active_bits) {
    if (profile_idx >= TOTAL_PROFILES_COUNT || !active_codes) return;

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

    // 4. โหลดข้อมูล 16 Words (12 ปุ่ม + Magic Word + Name 8 bytes + Reserved) เข้าสู่ Buffer
    char name_buf[8] = {0};
    if (active_name) {
        strncpy(name_buf, active_name, 7);
    } else {
        strncpy(name_buf, profile_names[profile_idx], 7);
    }

    uint32_t name_w0 = 0, name_w1 = 0;
    memcpy(&name_w0, &name_buf[0], 4);
    memcpy(&name_w1, &name_buf[4], 4);

    for (int i = 0; i < 16; i++) {
        if (i < 12) {
            ptr[i] = active_codes[i];
        } else if (i == 12) {
            ptr[i] = FLASH_PROFILE_MAGIC;
        } else if (i == 13) {
            ptr[i] = name_w0;
        } else if (i == 14) {
            ptr[i] = name_w1;
        } else if (i == 15) {
            ptr[i] = (uint32_t)active_protocol | ((uint32_t)active_bits << 8);
        } else {
            ptr[i] = 0;
        }
        FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
        while (FLASH->STATR & FLASH_STATR_BSY);
    }

    // 5. สั่งเขียนข้อมูลลง Flash ROM จริง
    FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
    while (FLASH->STATR & FLASH_STATR_BSY);

    // 6. ล็อก Flash
    FLASH->CTLR = CR_LOCK_Set;
}
