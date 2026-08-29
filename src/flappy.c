#include "flappy.h"
#include "ssd1306.h"
#include <string.h>

#define BIRD_X          14
#define BIRD_W          5
#define BIRD_H          4
#define PIPE_W          8
#define PIPE_GAP        28
#define FLOOR_Y         117
#define CEIL_Y          15

static int16_t bird_y;   // Fixed-point (subpixel / 16)
static int16_t bird_vy;  // Fixed-point velocity
static int16_t pipe_x[2];
static int16_t pipe_gap_y[2];
static uint8_t pipe_passed[2];

static uint32_t score = 0;
static uint32_t high_score = 0;
static uint8_t game_over = 0;
static uint8_t exit_requested = 0;
static uint32_t rng_state = 0x1A2B3C4D;

static void u32_to_str_padded(uint32_t val, char *buf, int len) {
    buf[len] = '\0';
    for (int i = len - 1; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
}

static uint8_t random_val(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (uint8_t)(rng_state % 255);
}

void flappy_init(void) {
    bird_y = 60 * 16;
    bird_vy = 0;

    pipe_x[0] = 64;
    pipe_gap_y[0] = 35 + (random_val() % 40);
    pipe_passed[0] = 0;

    pipe_x[1] = 64 + 40;
    pipe_gap_y[1] = 35 + (random_val() % 40);
    pipe_passed[1] = 0;

    score = 0;
    game_over = 0;
    exit_requested = 0;
}

void flappy_update(void) {
    if (game_over) return;

    // แรงโน้มถ่วง (Gravity)
    bird_vy += 2;
    if (bird_vy > 24) bird_vy = 24;
    bird_y += bird_vy;

    int16_t py = bird_y / 16;

    // ชนเพดานหรือชนพื้นล่าง
    if (py <= CEIL_Y || py + BIRD_H >= FLOOR_Y) {
        game_over = 1;
        if (score > high_score) high_score = score;
        return;
    }

    // อัปเดตเสาท่อทั้ง 2 ต้น
    for (uint8_t i = 0; i < 2; i++) {
        pipe_x[i] -= 1;

        if (pipe_x[i] < -PIPE_W) {
            pipe_x[i] = 64 + 10;
            pipe_gap_y[i] = 25 + (random_val() % 45); // ช่องว่างอยู่ระหว่าง Y 25..70
            pipe_passed[i] = 0;
        }

        // นับคะแนนเมื่อนกบินผ่านเสา
        if (!pipe_passed[i] && (pipe_x[i] + PIPE_W < BIRD_X)) {
            pipe_passed[i] = 1;
            score++;
            if (score > high_score) high_score = score;
        }

        // ตรวจสอบการชนเสาท่อ
        if (BIRD_X + BIRD_W > pipe_x[i] && BIRD_X < pipe_x[i] + PIPE_W) {
            if (py < pipe_gap_y[i] || py + BIRD_H > pipe_gap_y[i] + PIPE_GAP) {
                game_over = 1;
                if (score > high_score) high_score = score;
                return;
            }
        }
    }
}

void flappy_handle_key(uint8_t key, uint8_t is_new) {
    if (!is_new) return;

    if (key == 16) {
        exit_requested = 1;
        return;
    }

    if (game_over) {
        if (key == 6 || key == 12 || key == 1 || key == 2) {
            flappy_init();
        }
        return;
    }

    // กดปุ่มกระโดด/บิน (ปุ่ม 6: OK, 2: UP, 12: OK, 1: ON)
    if (key == 6 || key == 2 || key == 12 || key == 1 || key == 5 || key == 7) {
        bird_vy = -20; // บินขึ้น
    }
}

void flappy_render(void) {
    oled_clear_buffer();

    // 1. [ส่วนบน] Header แสดงคะแนนปัจจุบัน 5 หลัก
    char score_buf[8];
    u32_to_str_padded(score, score_buf, 5);
    oled_draw_str(2, 4, score_buf, 1);
    oled_fill_rect(0, CEIL_Y - 1, 64, 1, 1);

    // 2. [ส่วนกลาง] วาดเสาท่อ (Pipes)
    for (uint8_t i = 0; i < 2; i++) {
        int16_t px = pipe_x[i];
        if (px > -PIPE_W && px < 64) {
            int16_t gy = pipe_gap_y[i];

            // ท่อบน
            if (gy > CEIL_Y) {
                oled_draw_rect(px, CEIL_Y, PIPE_W, gy - CEIL_Y, 1);
                oled_fill_rect(px - 1, gy - 3, PIPE_W + 2, 3, 1); // ขอบท่อ
            }

            // ท่อล่าง
            int16_t bot_y = gy + PIPE_GAP;
            if (bot_y < FLOOR_Y) {
                oled_draw_rect(px, bot_y, PIPE_W, FLOOR_Y - bot_y, 1);
                oled_fill_rect(px - 1, bot_y, PIPE_W + 2, 3, 1); // ขอบท่อ
            }
        }
    }

    // วาดตัวนก (Pixel Bird 5x4)
    int16_t py = bird_y / 16;
    if (py > CEIL_Y && py < FLOOR_Y) {
        oled_fill_rect(BIRD_X + 1, py, 3, 1, 1);
        oled_fill_rect(BIRD_X, py + 1, 5, 1, 1);
        oled_draw_pixel(BIRD_X + 3, py + 1, 0); // ตา
        oled_fill_rect(BIRD_X, py + 2, 5, 1, 1);
        oled_fill_rect(BIRD_X + 1, py + 3, 3, 1, 1);
    }

    // วาดพื้นล่าง (Ground)
    oled_fill_rect(0, FLOOR_Y, 64, 1, 1);

    // 3. [ส่วนล่าง] Footer แสดง High Score
    char hi_buf[10];
    hi_buf[0] = 'H'; hi_buf[1] = 'I'; hi_buf[2] = ':';
    u32_to_str_padded(high_score, &hi_buf[3], 5);
    oled_draw_str((64 - 8 * 6) / 2, 120, hi_buf, 1);

    // ป๊อปอัป Game Over
    if (game_over) {
        oled_fill_rect(6, 48, 52, 28, 0);
        oled_draw_rect(6, 48, 52, 28, 1);
        oled_draw_str(8, 53, "GAME OVER", 1);
        oled_draw_str(11, 65, "6:RETRY", 1);
    }

    oled_update();
}

uint8_t flappy_should_exit(void) {
    return exit_requested;
}
