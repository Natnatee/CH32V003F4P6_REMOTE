#include "ssd1306.h"
#include <string.h>

// 1KB RAM Framebuffer (128x64 = 1024 bytes) บน SRAM 2KB ของ CH32V003
static uint8_t oled_buffer[1024];

// 6x8 Font Table (ASCII 32 to 126)
static const uint8_t font6x8[][6] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ' '
    { 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00 }, // '!'
    { 0x00, 0x07, 0x00, 0x07, 0x00, 0x00 }, // '"'
    { 0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00 }, // '#'
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00 }, // '$'
    { 0x23, 0x13, 0x08, 0x64, 0x62, 0x00 }, // '%'
    { 0x36, 0x49, 0x55, 0x22, 0x50, 0x00 }, // '&'
    { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00 }, // '''
    { 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00 }, // '('
    { 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00 }, // ')'
    { 0x14, 0x08, 0x3E, 0x08, 0x14, 0x00 }, // '*'
    { 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 }, // '+'
    { 0x00, 0x50, 0x30, 0x00, 0x00, 0x00 }, // ','
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x00 }, // '-'
    { 0x00, 0x60, 0x60, 0x00, 0x00, 0x00 }, // '.'
    { 0x20, 0x10, 0x08, 0x04, 0x02, 0x00 }, // '/'
    { 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 }, // '0'
    { 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 }, // '1'
    { 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 }, // '2'
    { 0x21, 0x41, 0x45, 0x4B, 0x31, 0x00 }, // '3'
    { 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 }, // '4'
    { 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 }, // '5'
    { 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00 }, // '6'
    { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 }, // '7'
    { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 }, // '8'
    { 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00 }, // '9'
    { 0x00, 0x36, 0x36, 0x00, 0x00, 0x00 }, // ':'
    { 0x00, 0x56, 0x36, 0x00, 0x00, 0x00 }, // ';'
    { 0x08, 0x14, 0x22, 0x41, 0x00, 0x00 }, // '<'
    { 0x14, 0x14, 0x14, 0x14, 0x14, 0x00 }, // '='
    { 0x00, 0x41, 0x22, 0x14, 0x08, 0x00 }, // '>'
    { 0x02, 0x01, 0x51, 0x09, 0x06, 0x00 }, // '?'
    { 0x32, 0x49, 0x79, 0x41, 0x3E, 0x00 }, // '@'
    { 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 }, // 'A'
    { 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00 }, // 'B'
    { 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 }, // 'C'
    { 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00 }, // 'D'
    { 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00 }, // 'E'
    { 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00 }, // 'F'
    { 0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00 }, // 'G'
    { 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 }, // 'H'
    { 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00 }, // 'I'
    { 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00 }, // 'J'
    { 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 }, // 'K'
    { 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00 }, // 'L'
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00 }, // 'M'
    { 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00 }, // 'N'
    { 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 }, // 'O'
    { 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 }, // 'P'
    { 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00 }, // 'Q'
    { 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00 }, // 'R'
    { 0x46, 0x49, 0x49, 0x49, 0x31, 0x00 }, // 'S'
    { 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 }, // 'T'
    { 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00 }, // 'U'
    { 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00 }, // 'V'
    { 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00 }, // 'W'
    { 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 }, // 'X'
    { 0x07, 0x08, 0x70, 0x08, 0x07, 0x00 }, // 'Y'
    { 0x61, 0x51, 0x49, 0x45, 0x43, 0x00 }, // 'Z'
    { 0x00, 0x7F, 0x41, 0x41, 0x00, 0x00 }, // '['
    { 0x02, 0x04, 0x08, 0x10, 0x20, 0x00 }, // '\'
    { 0x00, 0x41, 0x41, 0x7F, 0x00, 0x00 }, // ']'
    { 0x04, 0x02, 0x01, 0x02, 0x04, 0x00 }, // '^'
    { 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 }, // '_'
    { 0x00, 0x01, 0x02, 0x04, 0x00, 0x00 }, // '`'
    { 0x20, 0x54, 0x54, 0x54, 0x78, 0x00 }, // 'a'
    { 0x7F, 0x48, 0x44, 0x44, 0x38, 0x00 }, // 'b'
    { 0x38, 0x44, 0x44, 0x44, 0x20, 0x00 }, // 'c'
    { 0x38, 0x44, 0x44, 0x48, 0x7F, 0x00 }, // 'd'
    { 0x38, 0x54, 0x54, 0x54, 0x18, 0x00 }, // 'e'
    { 0x08, 0x7E, 0x09, 0x01, 0x02, 0x00 }, // 'f'
    { 0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00 }, // 'g'
    { 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00 }, // 'h'
    { 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00 }, // 'i'
    { 0x20, 0x40, 0x44, 0x3D, 0x00, 0x00 }, // 'j'
    { 0x7F, 0x10, 0x28, 0x44, 0x00, 0x00 }, // 'k'
    { 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00 }, // 'l'
    { 0x7C, 0x04, 0x18, 0x04, 0x78, 0x00 }, // 'm'
    { 0x7C, 0x08, 0x04, 0x04, 0x78, 0x00 }, // 'n'
    { 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 }, // 'o'
    { 0x7C, 0x14, 0x14, 0x14, 0x08, 0x00 }, // 'p'
    { 0x08, 0x14, 0x14, 0x18, 0x7C, 0x00 }, // 'q'
    { 0x7C, 0x08, 0x04, 0x04, 0x08, 0x00 }, // 'r'
    { 0x48, 0x54, 0x54, 0x54, 0x20, 0x00 }, // 's'
    { 0x04, 0x3F, 0x44, 0x40, 0x20, 0x00 }, // 't'
    { 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00 }, // 'u'
    { 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00 }, // 'v'
    { 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00 }, // 'w'
    { 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 }, // 'x'
    { 0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00 }, // 'y'
    { 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00 }  // 'z'
};

// --- Fast Push-Pull I2C (High-Speed Bit-Bang บน CH32V003) ---

static inline void i2c_delay(void) {
    for (volatile int i = 0; i < 4; i++);
}

static inline void scl_high(void) { GPIOC->BSHR = (1 << SSD1306_SCL_PIN_POS); }
static inline void scl_low(void)  { GPIOC->BCR  = (1 << SSD1306_SCL_PIN_POS); }
static inline void sda_high(void) { GPIOC->BSHR = (1 << SSD1306_SDA_PIN_POS); }
static inline void sda_low(void)  { GPIOC->BCR  = (1 << SSD1306_SDA_PIN_POS); }
static inline uint8_t sda_read(void) { return (GPIOC->INDR & (1 << SSD1306_SDA_PIN_POS)) ? 1 : 0; }

static void i2c_gpio_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

    GPIOC->CFGLR &= ~((0xf << (4 * SSD1306_SCL_PIN_POS)) | (0xf << (4 * SSD1306_SDA_PIN_POS)));
    GPIOC->CFGLR |=  ((0x1 << (4 * SSD1306_SCL_PIN_POS)) | (0x1 << (4 * SSD1306_SDA_PIN_POS)));

    sda_high();
    scl_high();
}

static void i2c_start(void) {
    sda_high(); scl_high(); i2c_delay();
    sda_low();  i2c_delay();
    scl_low();  i2c_delay();
}

static void i2c_stop(void) {
    sda_low();  i2c_delay();
    scl_high(); i2c_delay();
    sda_high(); i2c_delay();
}

static uint8_t i2c_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80) { sda_high(); } 
        else { sda_low(); }
        byte <<= 1;
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
        i2c_delay();
    }
    sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    uint8_t ack = sda_read();
    scl_low();
    i2c_delay();
    return ack;
}

static void ssd1306_write_cmd(uint8_t cmd) {
    i2c_start();
    i2c_write_byte(SSD1306_I2C_ADDR);
    i2c_write_byte(0x00);
    i2c_write_byte(cmd);
    i2c_stop();
}

// --- High-level SSD1306 Functions ---

void ssd1306_init(void) {
    i2c_gpio_init();

    ssd1306_write_cmd(0xAE); // Display OFF
    ssd1306_write_cmd(0xD5); // Set Display Clock Divide Ratio
    ssd1306_write_cmd(0x80);
    ssd1306_write_cmd(0xA8); // Set Multiplex Ratio (64 lines)
    ssd1306_write_cmd(0x3F);
    ssd1306_write_cmd(0xD3); // Set Display Offset
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(0x40); // Set Start Line (0)
    ssd1306_write_cmd(0x8D); // Charge Pump
    ssd1306_write_cmd(0x14); // Enable Charge Pump
    ssd1306_write_cmd(0x20); // Memory Addressing Mode
    ssd1306_write_cmd(0x02); // Page Addressing Mode
    ssd1306_write_cmd(0xA1); // Segment Re-map
    ssd1306_write_cmd(0xC8); // COM Scan Direction
    ssd1306_write_cmd(0xDA); // COM Pins Hardware Config
    ssd1306_write_cmd(0x12);
    ssd1306_write_cmd(0x81); // Contrast Control
    ssd1306_write_cmd(0xCF);
    ssd1306_write_cmd(0xD9); // Pre-charge Period
    ssd1306_write_cmd(0xF1);
    ssd1306_write_cmd(0xDB); // VCOMH Deselect Level
    ssd1306_write_cmd(0x40);
    ssd1306_write_cmd(0xA4); // Output follows RAM
    ssd1306_write_cmd(0xA6); // Normal Display
    ssd1306_write_cmd(0xAF); // Display ON

    ssd1306_clear();
}

void ssd1306_clear(void) {
    oled_clear_buffer();
    oled_update();
}

// --- Portrait Framebuffer (64x128) Functions ---

void oled_clear_buffer(void) {
    for (int i = 0; i < 1024; i++) {
        oled_buffer[i] = 0;
    }
}

void oled_draw_pixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= 64 || y < 0 || y >= 128) return;

    int16_t xl = y;
    int16_t yl = 63 - x;

    uint16_t idx = xl + (yl / 8) * 128;
    if (color) {
        oled_buffer[idx] |= (1U << (yl % 8));
    } else {
        oled_buffer[idx] &= ~(1U << (yl % 8));
    }
}

void oled_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    for (int16_t i = 0; i < w; i++) {
        for (int16_t j = 0; j < h; j++) {
            oled_draw_pixel(x + i, y + j, color);
        }
    }
}

void oled_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    for (int16_t i = 0; i < w; i++) {
        oled_draw_pixel(x + i, y, color);
        oled_draw_pixel(x + i, y + h - 1, color);
    }
    for (int16_t j = 0; j < h; j++) {
        oled_draw_pixel(x, y + j, color);
        oled_draw_pixel(x + w - 1, y + j, color);
    }
}

void oled_draw_char(int16_t x, int16_t y, char c, uint8_t color) {
    if (c < 32 || c > 126) c = ' ';
    uint8_t idx = c - 32;
    for (uint8_t col = 0; col < 6; col++) {
        uint8_t line = font6x8[idx][col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1U << row)) {
                oled_draw_pixel(x + col, y + row, color);
            } else if (!color) {
                oled_draw_pixel(x + col, y + row, 1);
            }
        }
    }
}

void oled_draw_str(int16_t x, int16_t y, const char *str, uint8_t color) {
    while (*str) {
        oled_draw_char(x, y, *str++, color);
        x += 6;
    }
}

// วาดตัวอักษรขนาดใหญ่ สูง 11px เส้นโปร่ง (Single-pixel stroke)
void oled_draw_char_large(int16_t x, int16_t y, char c) {
    if (c < 32 || c > 126) c = ' ';
    uint8_t idx = c - 32;
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font6x8[idx][col];
        for (uint8_t row = 0; row < 11; row++) {
            uint8_t src_y = (row * 8) / 11;
            if (line & (1U << src_y)) {
                oled_draw_pixel(x + col, y + row, 1);
            }
        }
    }
}

// วาดข้อความ Header ด้านบน ตัวใหญ่ สูง 11px จัดกึ่งกลางและเว้นระยะอัตโนมัติ
void oled_draw_header_title(int16_t y, const char *str) {
    int len = strlen(str);
    if (len == 0) return;

    int char_w = 5;
    int gap = 3;
    if (len <= 2) gap = 6;
    else if (len <= 4) gap = 4;
    else if (len <= 6) gap = 3;
    else gap = 2;

    int total_w = (len * char_w) + ((len - 1) * gap);
    int16_t start_x = (64 - total_w) / 2;
    if (start_x < 2) start_x = 2;

    for (int i = 0; i < len; i++) {
        oled_draw_char_large(start_x + i * (char_w + gap), y, str[i]);
    }
}

// แปลงตัวเลข 32-bit เป็น Hex String เช่น "0x20DF10EF"
void hex_to_str(uint32_t val, char *out) {
    static const char hex_digits[] = "0123456789ABCDEF";
    out[0] = '0';
    out[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        out[2 + (7 - i)] = hex_digits[(val >> (i * 4)) & 0xF];
    }
    out[10] = '\0';
}

// ตาราง 3 คอลัมน์ x 4 แถว (12 ปุ่มควบคุม)
static const uint8_t grid_key_map[4][3] = {
    { 1,  2,  3 },
    { 5,  6,  7 },
    { 9, 10, 11 },
    { 13, 14, 15 }
};

// เรนเดอร์หน้าจอ 3 ส่วน: บน (Header) / กลาง (ตาราง 3x4 พร้อมสถานะช่องจำ/ว่าง) / ล่าง (Footer)
void oled_render_grid_screen(const char *header, const char *footer, uint8_t active_key, uint8_t blink_state, const uint32_t *profile_codes) {
    oled_clear_buffer();

    // 1. [ส่วนบน] วาด Header ตัวใหญ่ สูง 11px
    if (header) {
        oled_draw_header_title(4, header);
    }
    oled_fill_rect(2, 18, 60, 1, 1); // เส้นคั่นบน

    // 2. [ส่วนกลาง] วาดตาราง 3x4 (12 ช่องปุ่ม)
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 3; c++) {
            uint8_t key_num = grid_key_map[r][c];
            uint8_t cell_idx = r * 3 + c;

            int16_t bx = 3 + c * (17 + 3);  // X: 3, 23, 43
            int16_t by = 21 + r * (17 + 3); // Y: 21, 41, 61, 81
            int16_t bw = 17;
            int16_t bh = 17;

            // ตรวจสอบว่าช่องนี้มีโค้ดแล้วหรือยัง
            uint8_t has_code = (profile_codes != NULL && profile_codes[cell_idx] != 0);

            // ตรวจสอบสถานะการกด / กระพริบ
            uint8_t is_active = (key_num == active_key);
            uint8_t show_filled;

            if (is_active) {
                // ถ้ากำลังกดปุ่มนี้อยู่ ให้สลับสถานะไปมาเพื่อกระพริบ
                show_filled = blink_state ? (!has_code) : has_code;
            } else {
                // สถานะปกติ: ช่องที่จำแล้ว = พื้นขาวตัวเลขอักษรดำ (show_filled=1), ช่องว่าง = กรอบขาวตัวเลขขาว (show_filled=0)
                show_filled = has_code;
            }

            if (show_filled) {
                // ช่องมีโค้ด (หรือไฮไลต์): กล่องสีขาวทึบ ตัวเลขสีดำ
                oled_fill_rect(bx, by, bw, bh, 1);
            } else {
                // ช่องว่าง: ตีกรอบสีขาว ตัวเลขสีขาว
                oled_draw_rect(bx, by, bw, bh, 1);
            }

            // วาดตัวเลขข้างในกล่อง (จัดกึ่งกลาง)
            char num_str[4];
            if (key_num < 10) {
                num_str[0] = '0' + key_num;
                num_str[1] = '\0';
                oled_draw_char(bx + 6, by + 5, num_str[0], show_filled ? 0 : 1);
            } else {
                num_str[0] = '0' + (key_num / 10);
                num_str[1] = '0' + (key_num % 10);
                num_str[2] = '\0';
                oled_draw_char(bx + 3, by + 5, num_str[0], show_filled ? 0 : 1);
                oled_draw_char(bx + 9, by + 5, num_str[1], show_filled ? 0 : 1);
            }
        }
    }

    // 3. [ส่วนล่าง] เส้นคั่นล่าง และ Footer
    oled_fill_rect(2, 103, 60, 1, 1);

    if (footer) {
        int len = strlen(footer);
        int16_t fx = (64 - (len * 6)) / 2;
        if (fx < 2) fx = 2;
        oled_draw_str(fx, 111, footer, 1);
    }

    // ส่งภาพขึ้นจอ
    oled_update();
}

// เรนเดอร์หน้าจอเมื่อรับรหัส IR ได้ในโหมด LEARN
void oled_render_ir_captured_screen(const char *header, uint32_t ir_code) {
    oled_clear_buffer();

    // 1. [ส่วนบน] Header แบรนด์ตัวใหญ่
    if (header) {
        oled_draw_header_title(4, header);
    }
    oled_fill_rect(2, 18, 60, 1, 1); // เส้นคั่นบน

    // 2. [ส่วนกลาง] แสดงรหัส IR ที่อ่านได้ขนาดใหญ่ชัดเจน
    oled_draw_str(8, 25, "IR READ:", 1);

    // กรอบโชว์รหัส HEX (X: 1, Y: 38, W: 62, H: 18)
    oled_draw_rect(1, 38, 62, 18, 1);
    char hex_buf[12];
    hex_to_str(ir_code, hex_buf); // e.g. "0x20DF10EF"
    oled_draw_str(2, 43, hex_buf, 1);

    oled_draw_str(2, 68, "PRESS 1-15", 1);
    oled_draw_str(11, 82, "TO SAVE", 1);

    // 3. [ส่วนล่าง] Footer
    oled_fill_rect(2, 103, 60, 1, 1);
    oled_draw_str(5, 111, "16:CANCEL", 1);

    oled_update();
}

// เรนเดอร์หน้าจอเมื่อกวาดยิงรหัสในโหมด NEW (แสดงลำดับ รหัส และวิธีเซฟ)
void oled_render_new_code_screen(const char *header, uint32_t code, uint16_t cur_idx, uint16_t total_count) {
    oled_clear_buffer();

    // 1. [ส่วนบน] Header แบรนด์ตัวใหญ่
    if (header) {
        oled_draw_header_title(4, header);
    }
    oled_fill_rect(2, 18, 60, 1, 1); // เส้นคั่นบน

    // 2. [ส่วนกลาง] แสดงสถานะการกวาดยิง
    oled_draw_str(8, 23, "SENDING:", 1);

    // แสดงลำดับ [ 001 / 256 ]
    char idx_str[16];
    uint16_t c = cur_idx + 1;
    idx_str[0] = '[';
    idx_str[1] = '0' + (c / 100);
    idx_str[2] = '0' + ((c / 10) % 10);
    idx_str[3] = '0' + (c % 10);
    idx_str[4] = '/';
    idx_str[5] = '0' + (total_count / 100);
    idx_str[6] = '0' + ((total_count / 10) % 10);
    idx_str[7] = '0' + (total_count % 10);
    idx_str[8] = ']';
    idx_str[9] = '\0';
    oled_draw_str((64 - 9 * 6) / 2, 35, idx_str, 1);

    // กรอบโชว์รหัส HEX (X: 1, Y: 48, W: 62, H: 17)
    oled_draw_rect(1, 48, 62, 17, 1);
    char hex_buf[12];
    hex_to_str(code, hex_buf);
    oled_draw_str(2, 53, hex_buf, 1);

    oled_draw_str(2, 69, "PRESS 1-15", 1);
    oled_draw_str(11, 83, "TO SAVE", 1);

    // 3. [ส่วนล่าง] Footer
    oled_fill_rect(2, 103, 60, 1, 1);
    oled_draw_str(8, 111, "12:RETRY", 1);

    oled_update();
}

void oled_update(void) {
    for (uint8_t page = 0; page < 8; page++) {
        ssd1306_write_cmd(0xB0 + page);
        ssd1306_write_cmd(0x00);
        ssd1306_write_cmd(0x10);

        i2c_start();
        i2c_write_byte(SSD1306_I2C_ADDR);
        i2c_write_byte(0x40);
        for (uint8_t col = 0; col < 128; col++) {
            i2c_write_byte(oled_buffer[col + (page * 128)]);
        }
        i2c_stop();
    }
}
