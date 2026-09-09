#include "flash_storage.h"
#include "ir_database.h"
#include "ir_recv.h"
#include <string.h>

#define META_V4_FLAG       0x80U
#define CODE_TAG_SHIFT     28
#define CODE_VALUE_MASK    0x0FFFFFFFUL
#define CODE_TAG_SHARP     1U
#define CODE_TAG_JVC       2U
#define CODE_TAG_LG        3U
#define CODE_TAG_SONY12    4U
#define CODE_TAG_SONY15    5U
#define CODE_TAG_SONY20    6U

static uint16_t compact_mask;
static uint16_t samsung_mask;

static uint8_t compact_tag(uint8_t protocol, uint8_t bits) {
    if (protocol == IR_PROTOCOL_SHARP) return CODE_TAG_SHARP;
    if (protocol == IR_PROTOCOL_JVC) return CODE_TAG_JVC;
    if (protocol == IR_PROTOCOL_LG) return CODE_TAG_LG;
    if (protocol == IR_PROTOCOL_SONY) {
        if (bits == 12) return CODE_TAG_SONY12;
        if (bits == 15) return CODE_TAG_SONY15;
        if (bits == 20) return CODE_TAG_SONY20;
    }
    return 0;
}

uint32_t flash_encode_ir_button(uint8_t idx, uint32_t code, uint8_t protocol, uint8_t bits) {
    if (idx >= 12) return code;
    uint16_t bit = (uint16_t)(1U << idx);
    uint8_t tag = compact_tag(protocol, bits);
    compact_mask &= (uint16_t)~bit;
    samsung_mask &= (uint16_t)~bit;
    if (tag) {
        compact_mask |= bit;
        return ((uint32_t)tag << CODE_TAG_SHIFT) | (code & CODE_VALUE_MASK);
    }
    if (protocol == IR_PROTOCOL_SAMSUNG) samsung_mask |= bit;
    return code;
}

void flash_decode_ir_button(uint8_t idx, uint32_t stored, uint32_t *code,
                            uint8_t *protocol, uint8_t *bits) {
    uint8_t p = IR_PROTOCOL_NEC, b = 32;
    uint32_t value = stored;
    if (idx < 12 && (compact_mask & (1U << idx))) {
        uint8_t tag = stored >> CODE_TAG_SHIFT;
        value &= CODE_VALUE_MASK;
        if (tag == CODE_TAG_SHARP) { p = IR_PROTOCOL_SHARP; b = IR_SHARP_BITS; }
        else if (tag == CODE_TAG_JVC) { p = IR_PROTOCOL_JVC; b = 16; }
        else if (tag == CODE_TAG_LG) { p = IR_PROTOCOL_LG; b = 28; }
        else { p = IR_PROTOCOL_SONY; b = tag == CODE_TAG_SONY12 ? 12 :
                                               tag == CODE_TAG_SONY15 ? 15 : 20; }
    } else if (idx < 12 && (samsung_mask & (1U << idx))) {
        p = IR_PROTOCOL_SAMSUNG;
    }
    if (code) *code = value;
    if (protocol) *protocol = p;
    if (bits) *bits = b;
}

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
    uint32_t meta = ptr[15];
    uint8_t old_protocol = (uint8_t)meta;
    uint8_t old_bits = (uint8_t)(meta >> 8);
    uint8_t is_v4 = ptr[12] == FLASH_PROFILE_MAGIC &&
                    (old_protocol & META_V4_FLAG) != 0;
    compact_mask = is_v4 ? (uint16_t)((meta >> 8) & 0x0FFF) : 0;
    samsung_mask = is_v4 ? (uint16_t)((meta >> 20) & 0x0FFF) : 0;

    // 1. โหลดรหัสปุ่ม 12 ปุ่ม
    if (active_codes) {
        if (ptr[12] == FLASH_PROFILE_MAGIC) {
            for (int i = 0; i < 12; i++) {
                active_codes[i] = ptr[i];
                if (!is_v4 && ptr[i] != 0 && ptr[i] != 0xFFFFFFFF) {
                    active_codes[i] = flash_encode_ir_button((uint8_t)i, ptr[i],
                                                              old_protocol, old_bits);
                }
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

    if (active_protocol) *active_protocol = (ptr[12] == FLASH_PROFILE_MAGIC) ?
                                            (is_v4 ? IR_PROTOCOL_NEC : old_protocol) : 0;
    if (active_bits) *active_bits = (ptr[12] == FLASH_PROFILE_MAGIC) ?
                                    (is_v4 ? 32 : old_bits) : 0;
}

void flash_save_profile(uint8_t profile_idx, const uint32_t *active_codes, const char *active_name,
                        uint8_t active_protocol, uint8_t active_bits) {
    if (profile_idx >= TOTAL_PROFILES_COUNT || !active_codes) return;
    (void)active_protocol;
    (void)active_bits;

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
            ptr[i] = META_V4_FLAG | ((uint32_t)(compact_mask & 0x0FFF) << 8) |
                     ((uint32_t)(samsung_mask & 0x0FFF) << 20);
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
