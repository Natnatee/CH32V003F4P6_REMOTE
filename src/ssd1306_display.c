#include "ssd1306.h"

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

void oled_draw_char(int16_t x, int16_t y, char c) {
    if (c < 32 || c > 126) c = ' ';
    uint8_t idx = c - 32;
    for (uint8_t col = 0; col < 6; col++) {
        uint8_t line = font6x8[idx][col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1U << row)) {
                oled_draw_pixel(x + col, y + row, 1);
            }
        }
    }
}

void oled_draw_str(int16_t x, int16_t y, const char *str) {
    while (*str) {
        oled_draw_char(x, y, *str++);
        x += 6;
    }
}

// 7-Segment Big Digit
void oled_draw_digit(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, uint8_t digit) {
    static const uint8_t seg_masks[10] = {
        0x3F, // 0: A B C D E F
        0x06, // 1: B C
        0x5B, // 2: A B D E G
        0x4F, // 3: A B C D G
        0x66, // 4: B C F G
        0x6D, // 5: A C D F G
        0x7D, // 6: A C D E F G
        0x07, // 7: A B C
        0x7F, // 8: A B C D E F G
        0x6F  // 9: A B C D F G
    };
    if (digit > 9) return;
    uint8_t mask = seg_masks[digit];
    int16_t half_h = h / 2;

    if (mask & (1 << 0)) oled_fill_rect(x + t, y, w - (2 * t), t, 1);                         // Seg A
    if (mask & (1 << 1)) oled_fill_rect(x + w - t, y + t, t, half_h - t, 1);                  // Seg B
    if (mask & (1 << 2)) oled_fill_rect(x + w - t, y + half_h, t, half_h - t, 1);             // Seg C
    if (mask & (1 << 3)) oled_fill_rect(x + t, y + h - t, w - (2 * t), t, 1);                 // Seg D
    if (mask & (1 << 4)) oled_fill_rect(x, y + half_h, t, half_h - t, 1);                     // Seg E
    if (mask & (1 << 5)) oled_fill_rect(x, y + t, t, half_h - t, 1);                          // Seg F
    if (mask & (1 << 6)) oled_fill_rect(x + t, y + half_h - (t / 2), w - (2 * t), t, 1);     // Seg G
}

// วาดตัวอักษร SAMSUNG รวมกว้าง 56px พร้อมเว้นระยะห่างระหว่างตัวอักษร (Letter Spacing 3px)
void oled_draw_samsung_56px(int16_t x, int16_t y) {
    // 1. S (กว้าง 5, สูง 11) -> X: 4
    int16_t s1 = 4;
    oled_fill_rect(s1 + 1, y + 0, 4, 1, 1);
    oled_fill_rect(s1 + 0, y + 1, 1, 4, 1);
    oled_fill_rect(s1 + 1, y + 5, 3, 1, 1);
    oled_fill_rect(s1 + 4, y + 6, 1, 4, 1);
    oled_fill_rect(s1 + 0, y + 10, 4, 1, 1);

    // 2. A (กว้าง 5, สูง 11) -> X: 12 (Gap = 3px)
    int16_t ax = 12;
    oled_fill_rect(ax + 1, y + 0, 3, 1, 1);
    oled_fill_rect(ax + 0, y + 1, 1, 10, 1);
    oled_fill_rect(ax + 4, y + 1, 1, 10, 1);
    oled_fill_rect(ax + 1, y + 5, 3, 1, 1);

    // 3. M (กว้าง 7, สูง 11) -> X: 20 (Gap = 3px)
    int16_t mx = 20;
    oled_fill_rect(mx + 0, y + 0, 1, 11, 1);
    oled_fill_rect(mx + 6, y + 0, 1, 11, 1);
    oled_draw_pixel(mx + 1, y + 1, 1);
    oled_draw_pixel(mx + 2, y + 2, 1);
    oled_draw_pixel(mx + 3, y + 3, 1);
    oled_draw_pixel(mx + 3, y + 4, 1);
    oled_draw_pixel(mx + 4, y + 2, 1);
    oled_draw_pixel(mx + 5, y + 1, 1);

    // 4. S (กว้าง 5, สูง 11) -> X: 30 (Gap = 3px)
    int16_t s2 = 30;
    oled_fill_rect(s2 + 1, y + 0, 4, 1, 1);
    oled_fill_rect(s2 + 0, y + 1, 1, 4, 1);
    oled_fill_rect(s2 + 1, y + 5, 3, 1, 1);
    oled_fill_rect(s2 + 4, y + 6, 1, 4, 1);
    oled_fill_rect(s2 + 0, y + 10, 4, 1, 1);

    // 5. U (กว้าง 5, สูง 11) -> X: 38 (Gap = 3px)
    int16_t ux = 38;
    oled_fill_rect(ux + 0, y + 0, 1, 10, 1);
    oled_fill_rect(ux + 4, y + 0, 1, 10, 1);
    oled_fill_rect(ux + 1, y + 10, 3, 1, 1);

    // 6. N (กว้าง 6, สูง 11) -> X: 46 (Gap = 3px)
    int16_t nx = 46;
    oled_fill_rect(nx + 0, y + 0, 1, 11, 1);
    oled_fill_rect(nx + 5, y + 0, 1, 11, 1);
    oled_draw_pixel(nx + 1, y + 2, 1);
    oled_draw_pixel(nx + 2, y + 4, 1);
    oled_draw_pixel(nx + 3, y + 6, 1);
    oled_draw_pixel(nx + 4, y + 8, 1);

    // 7. G (กว้าง 6, สูง 11) -> X: 54 (Gap = 2px)
    int16_t gx = 54;
    oled_fill_rect(gx + 1, y + 0, 4, 1, 1);
    oled_fill_rect(gx + 0, y + 1, 1, 9, 1);
    oled_fill_rect(gx + 1, y + 10, 4, 1, 1);
    oled_fill_rect(gx + 5, y + 5, 1, 5, 1);
    oled_fill_rect(gx + 2, y + 5, 3, 1, 1);
}

// เรนเดอร์หน้าจอรีโมทแนวตั้ง: แสดง Header ด้านบน + ตัวเลขขนาดใหญ่ตรงกลาง
void oled_render_remote_screen(const char *header, uint8_t num) {
    oled_clear_buffer();

    // 1. วาด Header SAMSUNG ขนาด 56px พอดีจอ
    oled_draw_samsung_56px(5, 5);

    // เส้นคั่นใต้ Header
    oled_fill_rect(4, 19, 56, 1, 1);

    // 2. วาดตัวเลขขนาดใหญ่ตรงกลาง (Y: 28 - 90)
    if (num == 0) {
        // แถบขีดกลางตอนสแตนด์บาย
        oled_fill_rect(20, 56, 24, 4, 1);
    } else if (num < 10) {
        // ตัวเลขหลักเดียวขนาดใหญ่ (กว้าง 34, สูง 60)
        int16_t w = 34;
        int16_t h = 60;
        int16_t t = 5;
        int16_t x = (64 - w) / 2;
        int16_t y = 28;
        oled_draw_digit(x, y, w, h, t, num);
    } else {
        // ตัวเลข 2 หลัก (10 - 16) เคียงกัน
        int16_t w = 22;
        int16_t h = 52;
        int16_t t = 4;
        int16_t gap = 4;
        int16_t total_w = (w * 2) + gap;
        int16_t x1 = (64 - total_w) / 2;
        int16_t x2 = x1 + w + gap;
        int16_t y = 32;

        oled_draw_digit(x1, y, w, h, t, num / 10);
        oled_draw_digit(x2, y, w, h, t, num % 10);
    }

    // 3. เส้นคั่นและข้อความด้านล่าง
    oled_fill_rect(4, 98, 56, 1, 1);
    oled_draw_str(14, 108, "REMOTE");

    // ยิงขึ้นจอ
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
