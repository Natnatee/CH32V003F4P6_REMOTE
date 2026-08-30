#include "ssd1306.h"
#include <string.h>

// 1KB RAM Framebuffer (128x64 = 1024 bytes) บน SRAM 2KB ของ CH32V003
static uint8_t oled_buffer[1024];

// 5x8 Ultra-Compact Font Table (ASCII 32 ' ' to 90 'Z' = 59 ตัวอักษร x 5 ไบต์ = 295 ไบต์เท่านั้น!)
// ตัวพิมพ์เล็ก a-z ถูกแปลงเป็น A-Z อัตโนมัติ ประหยัด Flash มหาศาล
static const uint8_t font5x8[59][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, // 32: ' '
    { 0x00, 0x00, 0x5F, 0x00, 0x00 }, // 33: '!'
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, // 34: '"'
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, // 35: '#'
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, // 36: '$'
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, // 37: '%'
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, // 38: '&'
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, // 39: '''
    { 0x00, 0x1C, 0x22, 0x41, 0x00 }, // 40: '('
    { 0x00, 0x41, 0x22, 0x1C, 0x00 }, // 41: ')'
    { 0x14, 0x08, 0x3E, 0x08, 0x14 }, // 42: '*'
    { 0x08, 0x08, 0x3E, 0x08, 0x08 }, // 43: '+'
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, // 44: ','
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, // 45: '-'
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, // 46: '.'
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, // 47: '/'
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, // 48: '0'
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, // 49: '1'
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 50: '2'
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, // 51: '3'
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, // 52: '4'
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 53: '5'
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, // 54: '6'
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 55: '7'
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 56: '8'
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, // 57: '9'
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, // 58: ':'
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, // 59: ';'
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, // 60: '<'
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, // 61: '='
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, // 62: '>'
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, // 63: '?'
    { 0x32, 0x49, 0x79, 0x41, 0x3E }, // 64: '@'
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, // 65: 'A'
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // 66: 'B'
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // 67: 'C'
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, // 68: 'D'
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, // 69: 'E'
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, // 70: 'F'
    { 0x3E, 0x41, 0x49, 0x49, 0x7A }, // 71: 'G'
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // 72: 'H'
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // 73: 'I'
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // 74: 'J'
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // 75: 'K'
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // 76: 'L'
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, // 77: 'M'
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // 78: 'N'
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // 79: 'O'
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, // 80: 'P'
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, // 81: 'Q'
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, // 82: 'R'
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, // 83: 'S'
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, // 84: 'T'
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, // 85: 'U'
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, // 86: 'V'
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, // 87: 'W'
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, // 88: 'X'
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, // 89: 'Y'
    { 0x61, 0x51, 0x49, 0x45, 0x43 }  // 90: 'Z'
};

// --- Fast Push-Pull I2C (High-Speed Bit-Bang บน CH32V003) ---

static inline void i2c_delay(void) {
    for (volatile int i = 0; i < 4; i++);
}

static inline void scl_high(void) { GPIOC->BSHR = (1 << SSD1306_SCL_PIN_POS); }
static inline void scl_low(void)  { GPIOC->BCR  = (1 << SSD1306_SCL_PIN_POS); }
static inline void sda_high(void) { GPIOC->BSHR = (1 << SSD1306_SDA_PIN_POS); }
static inline void sda_low(void)  { GPIOC->BCR  = (1 << SSD1306_SDA_PIN_POS); }

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
    }
    sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    return 1;
}

static void ssd1306_write_cmd(uint8_t cmd) {
    i2c_start();
    i2c_write_byte(SSD1306_I2C_ADDR);
    i2c_write_byte(0x00);
    i2c_write_byte(cmd);
    i2c_stop();
}

void ssd1306_init(void) {
    i2c_gpio_init();
    Delay_Ms(20);

    ssd1306_write_cmd(0xAE); // Display OFF
    ssd1306_write_cmd(0xD5); ssd1306_write_cmd(0x80);
    ssd1306_write_cmd(0xA8); ssd1306_write_cmd(0x3F);
    ssd1306_write_cmd(0xD3); ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(0x40);
    ssd1306_write_cmd(0x8D); ssd1306_write_cmd(0x14); // Enable Charge Pump
    ssd1306_write_cmd(0x20); ssd1306_write_cmd(0x02); // Page Addressing Mode
    ssd1306_write_cmd(0xA0); // Segment Re-map (0xA0 = Normal, 0xA1 = Remapped)
    ssd1306_write_cmd(0xC0); // COM Output Scan Direction (0xC0 = Normal, 0xC8 = Remapped)
    ssd1306_write_cmd(0xDA); ssd1306_write_cmd(0x12);
    ssd1306_write_cmd(0x81); ssd1306_write_cmd(0xCF);
    ssd1306_write_cmd(0xD9); ssd1306_write_cmd(0xF1);
    ssd1306_write_cmd(0xDB); ssd1306_write_cmd(0x40);
    ssd1306_write_cmd(0xA4);
    ssd1306_write_cmd(0xA6); // Normal Display
    ssd1306_write_cmd(0xAF); // Display ON

    ssd1306_clear();
}

void ssd1306_clear(void) {
    oled_clear_buffer();
    oled_update();
}

void oled_clear_buffer(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_draw_pixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= 64 || y < 0 || y >= 128) return;

    int16_t xl = y;
    int16_t yl = 63 - x;

    uint16_t idx = (yl / 8) * 128 + xl;
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
    if (c >= 'a' && c <= 'z') c -= 32; // แปลงตัวพิมพ์เล็กเป็นพิมพ์ใหญ่
    if (c < 32 || c > 90) c = ' ';
    uint8_t idx = c - 32;
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x8[idx][col];
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
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 32 || c > 90) c = ' ';
    uint8_t idx = c - 32;
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x8[idx][col];
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

// แปลงค่า 32-bit Hex เป็น String
void hex_to_str(uint32_t val, char *out) {
    out[0] = '0';
    out[1] = 'X';
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0x0F;
        out[2 + (7 - i)] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }
    out[10] = '\0';
}

// ตารางตัวเลขปุ่ม 1..15
static const char *grid_labels[12] = {
    "1", "2", "3",
    "5", "6", "7",
    "9", "10", "11",
    "13", "14", "15"
};

// เรนเดอร์หน้าจอ 3 ส่วน
void oled_render_grid_screen(const char *header, const char *footer, uint8_t active_key, uint8_t blink_state, const uint32_t *profile_codes) {
    oled_clear_buffer();

    if (header && strlen(header) > 0) {
        oled_draw_header_title(4, header);
    }
    oled_fill_rect(2, 18, 60, 1, 1);

    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 3; c++) {
            uint8_t idx = r * 3 + c;
            int16_t bx = 3 + c * (17 + 3);
            int16_t by = 21 + r * (17 + 3);
            int16_t bw = 17;
            int16_t bh = 17;

            uint8_t key_num = 0;
            switch (idx) {
                case 0: key_num = 1; break;
                case 1: key_num = 2; break;
                case 2: key_num = 3; break;
                case 3: key_num = 5; break;
                case 4: key_num = 6; break;
                case 5: key_num = 7; break;
                case 6: key_num = 9; break;
                case 7: key_num = 10; break;
                case 8: key_num = 11; break;
                case 9: key_num = 13; break;
                case 10: key_num = 14; break;
                case 11: key_num = 15; break;
            }

            uint8_t is_learned = (profile_codes && profile_codes[idx] != 0);
            uint8_t is_solid = is_learned;
            if (active_key == key_num && blink_state) {
                is_solid = !is_solid;
            }

            if (is_solid) {
                oled_fill_rect(bx, by, bw, bh, 1);
                const char *lbl = grid_labels[idx];
                int lbl_len = strlen(lbl);
                int16_t tx = (lbl_len == 1) ? (bx + 6) : (bx + 3);
                oled_draw_str(tx, by + 5, lbl, 0);
            } else {
                oled_draw_rect(bx, by, bw, bh, 1);
                const char *lbl = grid_labels[idx];
                int lbl_len = strlen(lbl);
                int16_t tx = (lbl_len == 1) ? (bx + 6) : (bx + 3);
                oled_draw_str(tx, by + 5, lbl, 1);
            }
        }
    }

    oled_fill_rect(2, 103, 60, 1, 1);
    if (footer && strlen(footer) > 0) {
        int len = strlen(footer);
        int16_t fx = (64 - (len * 6)) / 2;
        if (fx < 2) fx = 2;
        oled_draw_str(fx, 111, footer, 1);
    }

    oled_update();
}

void oled_render_ir_captured_screen(const char *header, uint32_t ir_code) {
    oled_clear_buffer();

    if (header) {
        oled_draw_header_title(4, header);
    }
    oled_fill_rect(2, 18, 60, 1, 1);

    oled_draw_str(8, 26, "IR READ:", 1);

    oled_draw_rect(1, 40, 62, 17, 1);
    char hex_buf[12];
    hex_to_str(ir_code, hex_buf);
    oled_draw_str(2, 45, hex_buf, 1);

    oled_draw_str(2, 65, "PRESS 1-15", 1);
    oled_draw_str(11, 79, "TO SAVE", 1);

    oled_fill_rect(2, 103, 60, 1, 1);
    oled_draw_str(5, 111, "16:CANCEL", 1);

    oled_update();
}

void oled_render_new_code_screen(const char *header, uint32_t code, uint16_t cur_idx, uint16_t total_count) {
    oled_clear_buffer();

    if (header) {
        oled_draw_header_title(4, header);
    }
    oled_fill_rect(2, 18, 60, 1, 1);

    oled_draw_str(8, 23, "SENDING:", 1);

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

    oled_draw_rect(1, 48, 62, 17, 1);
    char hex_buf[12];
    hex_to_str(code, hex_buf);
    oled_draw_str(2, 53, hex_buf, 1);

    oled_draw_str(2, 69, "PRESS 1-15", 1);
    oled_draw_str(11, 83, "TO SAVE", 1);

    oled_fill_rect(2, 103, 60, 1, 1);
    oled_draw_str(8, 111, "12:RETRY", 1);

    oled_update();
}

static const char *rename_key_labels[4][3] = {
    { "AB", "CD", "EF" },
    { "GH", "IJ", "KL" },
    { "MN", "OP", "QR" },
    { "ST", "UV", "WZ" }
};

void oled_render_rename_screen(const char *name, uint8_t cursor_pos, uint8_t blink_state, uint8_t profile_idx, uint8_t is_editing) {
    oled_clear_buffer();

    int16_t start_x = 5;

    for (int i = 0; i < 7; i++) {
        char ch = ' ';
        if (name && i < (int)strlen(name)) {
            ch = name[i];
        }

        int16_t cx = start_x + (i * 8);

        if (ch != ' ') {
            oled_draw_char_large(cx, 4, ch);
        }

        if (is_editing && i == cursor_pos && blink_state) {
            oled_fill_rect(cx, 16, 5, 2, 1);
        }
    }
    oled_fill_rect(2, 18, 60, 1, 1);

    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 3; c++) {
            int16_t bx = 3 + c * (17 + 3);
            int16_t by = 21 + r * (17 + 3);
            int16_t bw = 17;
            int16_t bh = 17;

            oled_draw_rect(bx, by, bw, bh, 1);
            oled_draw_str(bx + 3, by + 5, rename_key_labels[r][c], 1);
        }
    }

    oled_fill_rect(2, 103, 60, 1, 1);

    char footer_str[16];
    uint8_t p = profile_idx + 1;
    footer_str[0] = 'N'; footer_str[1] = 'A'; footer_str[2] = 'M'; footer_str[3] = 'E';
    footer_str[4] = ' ';
    footer_str[5] = '0' + (p / 10);
    footer_str[6] = '0' + (p % 10);
    footer_str[7] = '/';
    footer_str[8] = '1';
    footer_str[9] = '6';
    footer_str[10] = '\0';
    oled_draw_str((64 - 10 * 6) / 2, 111, footer_str, 1);

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
        for (uint16_t col = 0; col < 128; col++) {
            i2c_write_byte(oled_buffer[page * 128 + col]);
        }
        i2c_stop();
    }
}
