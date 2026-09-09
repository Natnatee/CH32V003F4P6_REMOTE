#pragma once

#include "ch32fun.h"
#include <stdint.h>

#define FLASH_PROFILE_BASE_ADDR   0x08003C00
#define FLASH_PROFILE_PAGE_SIZE   64
#define FLASH_PROFILE_MAGIC       0xA55A0003
#define FLASH_PROFILE_LEGACY_MAGIC 0xA55A0002

// เริ่มต้น Flash Storage
void flash_storage_init(void);

// โหลดข้อมูลปุ่ม 12 ปุ่ม และชื่อโปรไฟล์ (name: char[8]) ของ Profile ที่ระบุ (profile_idx: 0..15)
void flash_load_profile(uint8_t profile_idx, uint32_t *active_codes, char *active_name,
                        uint8_t *active_protocol, uint8_t *active_bits);

// บันทึกข้อมูลปุ่ม 12 ปุ่ม และชื่อโปรไฟล์ลง Flash ROM ถาวร
void flash_save_profile(uint8_t profile_idx, const uint32_t *active_codes, const char *active_name,
                        uint8_t active_protocol, uint8_t active_bits);
