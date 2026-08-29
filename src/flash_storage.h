#pragma once

#include "ch32fun.h"
#include <stdint.h>

// 16 Profiles เก็บไว้ที่ Flash 1KB ท้ายสุด (0x08003C00 - 0x08003FFF)
#define FLASH_PROFILE_BASE_ADDR   0x08003C00
#define FLASH_PROFILE_PAGE_SIZE   64
#define FLASH_PROFILE_MAGIC       0xA55A0001

// เริ่มต้น Flash Storage
void flash_storage_init(void);

// โหลดข้อมูลปุ่ม 12 ปุ่มของ Profile ที่ระบุ (profile_idx: 0..15) เข้าสู่ active_codes (12 uint32_t)
void flash_load_profile(uint8_t profile_idx, uint32_t *active_codes);

// บันทึกข้อมูลปุ่ม 12 ปุ่มของ Profile ที่ระบุลง Flash ROM ถาวร
void flash_save_profile(uint8_t profile_idx, const uint32_t *active_codes);
