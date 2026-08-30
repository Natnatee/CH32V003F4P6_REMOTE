#include "ina226.h"
#include "ssd1306.h"
#include <string.h>

#define SCL_PIN_POS 6
#define SDA_PIN_POS 7
#define INA_ADDR    0x40

#define INA226_REG_CONFIG       0x00
#define INA226_REG_SHUNTVOLTAGE 0x01
#define INA226_REG_BUSVOLTAGE   0x02

static uint8_t exit_req = 0;
static uint8_t cur_page = 0; // 0: REAL TIME, 1: PEAK, 2: AVG (5s)

static int32_t live_v_mv = 0;
static int32_t live_i_ua = 0;
static int32_t peak_v_mv = 0;
static int32_t peak_i_ua = 0;

#define AVG_SLOTS 25 // 25 slots x 200ms = 5.0 seconds window
static int16_t v_hist[AVG_SLOTS];
static int16_t i_hist[AVG_SLOTS];
static uint8_t avg_idx = 0;
static uint8_t avg_count = 0;
static uint8_t avg_tick = 0;
static int32_t avg_v_mv = 0;
static int32_t avg_i_ua = 0;

static const char *page_titles[3] = {
    "REAL TIME",
    "PEAK",
    "AVG (5s)"
};

// --- Low-Level Bit-Bang I2C Functions (100kHz Standard Timing) ---

static inline void i2c_dly(void) {
    for (volatile int i = 0; i < 45; i++);
}

static inline void scl_hi(void) { GPIOC->BSHR = (1 << SCL_PIN_POS); }
static inline void scl_lo(void) { GPIOC->BCR  = (1 << SCL_PIN_POS); }
static inline void sda_hi(void) { GPIOC->BSHR = (1 << SDA_PIN_POS); }
static inline void sda_lo(void) { GPIOC->BCR  = (1 << SDA_PIN_POS); }

static inline void sda_mode_out(void) {
    GPIOC->CFGLR &= ~(0xf << (4 * SDA_PIN_POS));
    GPIOC->CFGLR |=  (0x1 << (4 * SDA_PIN_POS)); // 10MHz Push-pull
}

static inline void sda_mode_in(void) {
    GPIOC->CFGLR &= ~(0xf << (4 * SDA_PIN_POS));
    GPIOC->CFGLR |=  (0x8 << (4 * SDA_PIN_POS)); // Input with pull-up
    GPIOC->BSHR = (1 << SDA_PIN_POS);
}

static inline uint8_t sda_get(void) {
    return (GPIOC->INDR & (1 << SDA_PIN_POS)) ? 1 : 0;
}

static void i2c_start_cond(void) {
    sda_mode_out();
    sda_hi();
    scl_hi();
    i2c_dly();
    sda_lo();
    i2c_dly();
    scl_lo();
    i2c_dly();
}

static void i2c_stop_cond(void) {
    sda_mode_out();
    sda_lo();
    scl_lo();
    i2c_dly();
    scl_hi();
    i2c_dly();
    sda_hi();
    i2c_dly();
}

static uint8_t i2c_tx_byte(uint8_t byte) {
    sda_mode_out();
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80) { sda_hi(); }
        else { sda_lo(); }
        byte <<= 1;
        i2c_dly();
        scl_hi();
        i2c_dly();
        scl_lo();
        i2c_dly();
    }

    sda_mode_in();
    i2c_dly();
    scl_hi();
    i2c_dly();
    uint8_t ack = sda_get();
    scl_lo();
    i2c_dly();
    sda_mode_out();
    return (ack == 0);
}

static uint8_t i2c_rx_byte(uint8_t send_ack) {
    uint8_t byte = 0;
    sda_mode_in();
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        i2c_dly();
        scl_hi();
        i2c_dly();
        if (sda_get()) {
            byte |= 1;
        }
        scl_lo();
        i2c_dly();
    }

    sda_mode_out();
    if (send_ack) { sda_lo(); }
    else { sda_hi(); }
    i2c_dly();
    scl_hi();
    i2c_dly();
    scl_lo();
    i2c_dly();
    sda_hi();
    return byte;
}

static uint8_t ina226_read_reg16(uint8_t addr, uint8_t reg, uint16_t *val) {
    i2c_start_cond();
    if (!i2c_tx_byte((addr << 1) | 0)) { i2c_stop_cond(); return 0; }
    if (!i2c_tx_byte(reg)) { i2c_stop_cond(); return 0; }
    i2c_stop_cond();
    i2c_dly();

    i2c_start_cond();
    if (!i2c_tx_byte((addr << 1) | 1)) { i2c_stop_cond(); return 0; }
    uint8_t msb = i2c_rx_byte(1);
    uint8_t lsb = i2c_rx_byte(0);
    i2c_stop_cond();

    *val = ((uint16_t)msb << 8) | lsb;
    return 1;
}

static uint8_t ina226_write_reg16(uint8_t addr, uint8_t reg, uint16_t val) {
    i2c_start_cond();
    if (!i2c_tx_byte((addr << 1) | 0)) { i2c_stop_cond(); return 0; }
    if (!i2c_tx_byte(reg)) { i2c_stop_cond(); return 0; }
    if (!i2c_tx_byte((uint8_t)(val >> 8))) { i2c_stop_cond(); return 0; }
    if (!i2c_tx_byte((uint8_t)(val & 0xFF))) { i2c_stop_cond(); return 0; }
    i2c_stop_cond();
    return 1;
}

// --- Module Functions ---

uint8_t ina226_init(void) {
    exit_req = 0;
    peak_v_mv = 0;
    peak_i_ua = 0;
    avg_count = 0;
    avg_idx = 0;
    avg_tick = 0;

    // Config: Continuous Shunt & Bus, 1.1ms conversion
    ina226_write_reg16(INA_ADDR, INA226_REG_CONFIG, 0x4127);
    return 1;
}

void ina226_update(void) {
    uint16_t raw_vbus = 0, raw_vshunt = 0;
    if (ina226_read_reg16(INA_ADDR, INA226_REG_BUSVOLTAGE, &raw_vbus) &&
        ina226_read_reg16(INA_ADDR, INA226_REG_SHUNTVOLTAGE, &raw_vshunt)) {

        live_v_mv = ((int32_t)raw_vbus * 125) / 100;
        live_i_ua = (((int32_t)(int16_t)raw_vshunt) * 25) / 100; // Shunt 10Ω: 0.25uA per LSB

        // 1. อัปเดต Peak (Max Voltage & Max Current)
        if (live_v_mv > peak_v_mv) peak_v_mv = live_v_mv;
        int32_t abs_i = (live_i_ua < 0) ? -live_i_ua : live_i_ua;
        if (abs_i > peak_i_ua) peak_i_ua = abs_i;

        // 2. อัปเดต AVG (5s Window: 25 slots x 200ms)
        avg_tick++;
        if (avg_tick >= 4) { // 4 x 50ms = 200ms
            avg_tick = 0;
            v_hist[avg_idx] = (int16_t)live_v_mv;
            i_hist[avg_idx] = (int16_t)live_i_ua;
            avg_idx = (avg_idx + 1) % AVG_SLOTS;
            if (avg_count < AVG_SLOTS) avg_count++;
        }

        if (avg_count > 0) {
            int32_t sv = 0, si = 0;
            for (uint8_t k = 0; k < avg_count; k++) {
                sv += v_hist[k];
                si += i_hist[k];
            }
            avg_v_mv = sv / avg_count;
            avg_i_ua = si / avg_count;
        } else {
            avg_v_mv = live_v_mv;
            avg_i_ua = live_i_ua;
        }
    }
}

void ina226_handle_key(uint8_t key) {
    if (key == 16) {
        exit_req = 1;
    } else if (key == 12 || key == 4 || key == 8) {
        // สลับหน้าระหว่าง REAL TIME -> PEAK -> AVG (5s)
        cur_page = (cur_page + 1) % 3;
    } else if (key == 6 || key == 1) {
        // รีเซ็ต Peak และ AVG
        peak_v_mv = live_v_mv;
        peak_i_ua = (live_i_ua < 0) ? -live_i_ua : live_i_ua;
        avg_count = 0;
        avg_idx = 0;
    }
}

uint8_t ina226_should_exit(void) {
    if (exit_req) {
        exit_req = 0;
        return 1;
    }
    return 0;
}

// Helper ฟอร์แมตตัวเลขแบบประหยัด Flash ROM
static void format_num(char *buf, int32_t val, uint8_t is_curr) {
    if (val < 0) {
        *buf++ = '-';
        val = -val;
    }
    if (!is_curr) {
        // Voltage: 5.02 V
        int32_t ip = val / 1000;
        int32_t fp = (val % 1000) / 10;
        char temp[6];
        int t = 0;
        if (ip == 0) temp[t++] = '0';
        else {
            while (ip > 0) { temp[t++] = '0' + (ip % 10); ip /= 10; }
        }
        for (int i = t - 1; i >= 0; i--) *buf++ = temp[i];
        *buf++ = '.';
        *buf++ = '0' + (fp / 10);
        *buf++ = '0' + (fp % 10);
        *buf++ = ' ';
        *buf++ = 'V';
    } else {
        // Current (Shunt 10Ω): ถ้า >= 1000uA แสดง mA (เช่น 1.45 mA) / ถ้าน้อยกว่าแสดง uA (เช่น 350 uA)
        if (val >= 1000) {
            int32_t ip = val / 1000;
            int32_t fp = (val % 1000) / 10;
            char temp[6];
            int t = 0;
            if (ip == 0) temp[t++] = '0';
            else {
                while (ip > 0) { temp[t++] = '0' + (ip % 10); ip /= 10; }
            }
            for (int i = t - 1; i >= 0; i--) *buf++ = temp[i];
            *buf++ = '.';
            *buf++ = '0' + (fp / 10);
            *buf++ = '0' + (fp % 10);
            *buf++ = ' ';
            *buf++ = 'm';
            *buf++ = 'A';
        } else {
            char temp[6];
            int t = 0;
            if (val == 0) temp[t++] = '0';
            else {
                while (val > 0) { temp[t++] = '0' + (val % 10); val /= 10; }
            }
            for (int i = t - 1; i >= 0; i--) *buf++ = temp[i];
            *buf++ = ' ';
            *buf++ = 'u';
            *buf++ = 'A';
        }
    }
    *buf = '\0';
}

static void draw_large_centered(int16_t y, const char *str) {
    int len = strlen(str);
    int16_t x = (64 - (len * 7)) / 2;
    if (x < 2) x = 2;
    while (*str) {
        char c = *str++;
        oled_draw_char_large(x, y, c);
        oled_draw_char_large(x + 1, y, c); // bold stroke
        x += 7;
    }
}

void ina226_render(void) {
    oled_clear_buffer();

    // 1. Header (REAL TIME / PEAK / AVG (5s))
    oled_draw_header_title(4, page_titles[cur_page]);
    oled_fill_rect(2, 18, 60, 1, 1);

    // เลือกข้อมูลตามโหมดที่กำลังแสดงผล
    int32_t disp_v = live_v_mv;
    int32_t disp_i = live_i_ua;
    if (cur_page == 1) {
        disp_v = peak_v_mv;
        disp_i = peak_i_ua;
    } else if (cur_page == 2) {
        disp_v = avg_v_mv;
        disp_i = avg_i_ua;
    }

    // 2. แสดงค่า Voltage ตัวใหญ่ตรงกลาง (เช่น 5.02 V)
    char v_str[16];
    format_num(v_str, disp_v, 0);
    draw_large_centered(38, v_str);

    // 3. แสดงค่า Current ตัวใหญ่ตรงกลาง (เช่น 350 uA / 1.45 mA)
    char i_str[16];
    format_num(i_str, disp_i, 1);
    draw_large_centered(70, i_str);

    // 4. Footer (MULTI)
    oled_fill_rect(2, 104, 60, 1, 1);
    oled_draw_header_title(111, "MULTI");

    oled_update();
}
