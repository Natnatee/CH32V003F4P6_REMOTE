#include "calc.h"
#include "ssd1306.h"
#include <string.h>

static const char *calc_key_labels_4x4[4][4] = {
    { "1", "2", "3", "+-" },
    { "4", "5", "6", "*/" },
    { "7", "8", "9", "="  },
    { "C", "0", ".", "EX" }
};

static const char op_chars[5] = { ' ', '+', '-', '*', '/' };

static int32_t prev_val = 0;
static uint8_t cur_op = 1; // 1: +, 2: -, 3: *, 4: /
static uint8_t op_active = 0;
static char input_buf[16] = "0";
static char history_buf[24] = "";
static uint8_t is_result_displayed = 0;
static uint8_t exit_requested = 0;

static void string_append(char *dst, const char *src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = '\0';
}

static int32_t parse_fixed(const char *s) {
    int32_t integer_part = 0;
    int32_t frac_part = 0;
    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        integer_part = integer_part * 10 + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        int count = 0;
        while (*s >= '0' && *s <= '9' && count < 2) {
            frac_part = frac_part * 10 + (*s - '0');
            count++;
            s++;
        }
        if (count == 1) frac_part *= 10;
    }
    return sign * (integer_part * 100 + frac_part);
}

static void format_fixed(int32_t val, char *buf) {
    if (val < 0) {
        *buf++ = '-';
        val = -val;
    }
    int32_t ip = val / 100;
    int32_t fp = val % 100;

    char temp[12];
    int t_idx = 0;
    if (ip == 0) {
        temp[t_idx++] = '0';
    } else {
        while (ip > 0) {
            temp[t_idx++] = '0' + (ip % 10);
            ip /= 10;
        }
    }
    for (int i = t_idx - 1; i >= 0; i--) {
        *buf++ = temp[i];
    }

    if (fp > 0) {
        *buf++ = '.';
        if (fp % 10 == 0) {
            *buf++ = '0' + (fp / 10);
        } else {
            *buf++ = '0' + (fp / 10);
            *buf++ = '0' + (fp % 10);
        }
    }
    *buf = '\0';
}

void calc_init(void) {
    prev_val = 0;
    cur_op = 1;
    op_active = 0;
    strcpy(input_buf, "0");
    history_buf[0] = '\0';
    is_result_displayed = 0;
    exit_requested = 0;
}

static void compute_result(void) {
    int32_t cur_val = parse_fixed(input_buf);
    int32_t res = cur_val;

    if (op_active) {
        if (cur_op == 1) res = prev_val + cur_val;
        else if (cur_op == 2) res = prev_val - cur_val;
        else if (cur_op == 3) res = (prev_val * cur_val) / 100;
        else if (cur_op == 4) {
            if (cur_val == 0) {
                strcpy(input_buf, "ERR");
                op_active = 0;
                is_result_displayed = 1;
                history_buf[0] = '\0';
                return;
            }
            res = (prev_val * 100) / cur_val;
        }

        char p_str[12], c_str[12];
        format_fixed(prev_val, p_str);
        format_fixed(cur_val, c_str);

        history_buf[0] = '\0';
        string_append(history_buf, p_str);
        char op_s[4] = { ' ', op_chars[cur_op], ' ', '\0' };
        string_append(history_buf, op_s);
        string_append(history_buf, c_str);

        prev_val = res;
        format_fixed(res, input_buf);
        op_active = 0;
        is_result_displayed = 1;
    }
}

void calc_handle_key(uint8_t key) {
    if (key == 16) {
        exit_requested = 1;
        return;
    }

    if (key == 4) {
        if (!op_active) {
            prev_val = parse_fixed(input_buf);
            cur_op = 1;
            op_active = 1;
            is_result_displayed = 1;
        } else {
            cur_op = (cur_op == 1) ? 2 : 1;
        }
        char p_str[12];
        format_fixed(prev_val, p_str);
        history_buf[0] = '\0';
        string_append(history_buf, p_str);
        char op_s[3] = { ' ', op_chars[cur_op], '\0' };
        string_append(history_buf, op_s);
        return;
    }

    if (key == 8) {
        if (!op_active) {
            prev_val = parse_fixed(input_buf);
            cur_op = 3;
            op_active = 1;
            is_result_displayed = 1;
        } else {
            cur_op = (cur_op == 3) ? 4 : 3;
        }
        char p_str[12];
        format_fixed(prev_val, p_str);
        history_buf[0] = '\0';
        string_append(history_buf, p_str);
        char op_s[3] = { ' ', op_chars[cur_op], '\0' };
        string_append(history_buf, op_s);
        return;
    }

    if (key == 12) {
        compute_result();
        return;
    }

    if (key == 13) {
        if (strcmp(input_buf, "0") != 0 && !is_result_displayed) {
            strcpy(input_buf, "0");
        } else {
            calc_init();
        }
        return;
    }

    if (key == 15) {
        if (is_result_displayed || strcmp(input_buf, "ERR") == 0) {
            strcpy(input_buf, "0.");
            is_result_displayed = 0;
        } else if (!strchr(input_buf, '.')) {
            int len = strlen(input_buf);
            if (len < 7) {
                input_buf[len] = '.';
                input_buf[len + 1] = '\0';
            }
        }
        return;
    }

    char digit = 0;
    switch (key) {
        case 1:  digit = '1'; break;
        case 2:  digit = '2'; break;
        case 3:  digit = '3'; break;
        case 5:  digit = '4'; break;
        case 6:  digit = '5'; break;
        case 7:  digit = '6'; break;
        case 9:  digit = '7'; break;
        case 10: digit = '8'; break;
        case 11: digit = '9'; break;
        case 14: digit = '0'; break;
        default: return;
    }

    if (digit) {
        if (is_result_displayed || strcmp(input_buf, "0") == 0 || strcmp(input_buf, "ERR") == 0) {
            input_buf[0] = digit;
            input_buf[1] = '\0';
            is_result_displayed = 0;
        } else {
            int len = strlen(input_buf);
            if (len < 7) {
                input_buf[len] = digit;
                input_buf[len + 1] = '\0';
            }
        }
    }
}

void calc_render(void) {
    oled_clear_buffer();

    if (strlen(history_buf) > 0) {
        oled_draw_str(3, 3, history_buf, 1);
    }

    int len = strlen(input_buf);
    int char_w = 5;
    int gap = 2;
    int total_w = (len * char_w) + ((len - 1) * gap);
    int16_t start_x = 61 - total_w;
    if (start_x < 3) start_x = 3;

    for (int i = 0; i < len; i++) {
        char ch = input_buf[i];
        int16_t cx = start_x + (i * (char_w + gap));
        if (ch == '.') {
            oled_fill_rect(cx + 1, 24, 2, 2, 1);
        } else {
            oled_draw_char_large(cx, 16, ch);
        }
    }

    oled_fill_rect(2, 32, 60, 1, 1);

    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            int16_t bx = 3 + c * (13 + 2);
            int16_t by = 37 + r * (13 + 2);
            int16_t bw = 13;
            int16_t bh = 13;

            oled_draw_rect(bx, by, bw, bh, 1);

            const char *lbl = calc_key_labels_4x4[r][c];
            int lbl_len = strlen(lbl);
            int16_t tx = (lbl_len == 1) ? (bx + 4) : (bx + 1);
            oled_draw_str(tx, by + 3, lbl, 1);
        }
    }

    oled_fill_rect(2, 102, 60, 1, 1);
    oled_draw_str(20, 110, "CALC", 1);

    oled_update();
}

uint8_t calc_should_exit(void) {
    return exit_requested;
}
