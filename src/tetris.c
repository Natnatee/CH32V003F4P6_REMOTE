#include "tetris.h"
#include "ssd1306.h"
#include <string.h>
#include <stdio.h>

#define BOARD_COLS      10
#define BOARD_ROWS      20
#define BLOCK_SIZE      5
#define BOARD_OFFSET_X  7
#define BOARD_OFFSET_Y  16

// รูปทรง Tetromino 7 แบบ (I, J, L, O, S, T, Z) ในเมทริกซ์ 4x4
static const uint16_t TETROMINOES[7] = {
    0x0F00, // 0: I (.... / #### / .... / ....)
    0x8E00, // 1: J (#... / ###. / .... / ....)
    0x2E00, // 2: L (..#. / ###. / .... / ....)
    0x6600, // 3: O (##.. / ##.. / .... / ....)
    0x6C00, // 4: S (.##. / ##.. / .... / ....)
    0x4E00, // 5: T (.#.. / ###. / .... / ....)
    0xC600  // 6: Z (##.. / .##. / .... / ....)
};

static uint8_t board[BOARD_ROWS][BOARD_COLS];
static int8_t piece_x, piece_y;
static uint8_t piece_type, piece_rot;
static uint8_t next_piece;
static uint32_t score = 0;
static uint32_t high_score = 0;
static uint16_t lines_cleared = 0;
static uint8_t game_over = 0;
static uint8_t exit_requested = 0;
static uint16_t drop_ticks = 0;
static uint16_t drop_interval = 20; // 20 * 20ms = 400ms ต่อก้าว

static uint8_t hold_down_ticks = 0;
static uint8_t hold_lr_ticks = 0;

static uint32_t rng_state = 0x5A5A1234;

static uint8_t random_piece(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (uint8_t)(rng_state % 7);
}

// ตรวจสอบว่ามีบล็อกที่พิกัด (bx, by) ในชิ้นส่วนที่หมุนมุม rot หรือไม่
static uint8_t get_piece_block(uint8_t type, uint8_t rot, uint8_t bx, uint8_t by) {
    uint8_t ox = bx, oy = by;
    if (rot == 1) {
        ox = by;
        oy = 3 - bx;
    } else if (rot == 2) {
        ox = 3 - bx;
        oy = 3 - by;
    } else if (rot == 3) {
        ox = 3 - by;
        oy = bx;
    }

    uint16_t shape = TETROMINOES[type];
    uint8_t bit_pos = 15 - (oy * 4 + ox);
    return (shape >> bit_pos) & 1;
}

// ตรวจสอบการชนขอบกระดานหรือชนบล็อกเดิม
static uint8_t check_collision(int8_t nx, int8_t ny, uint8_t nrot) {
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (get_piece_block(piece_type, nrot, c, r)) {
                int8_t gx = nx + c;
                int8_t gy = ny + r;

                // ชนขอบซ้าย/ขวา หรือชนพื้นล่าง
                if (gx < 0 || gx >= BOARD_COLS || gy >= BOARD_ROWS) {
                    return 1;
                }
                // ชนบล็อกที่อยู่บนกระดานแล้ว
                if (gy >= 0 && board[gy][gx]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// วางชิ้นบล็อกลงกระดานถาวร
static void lock_piece(void) {
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (get_piece_block(piece_type, piece_rot, c, r)) {
                int8_t gx = piece_x + c;
                int8_t gy = piece_y + r;
                if (gy >= 0 && gy < BOARD_ROWS && gx >= 0 && gx < BOARD_COLS) {
                    board[gy][gx] = 1;
                }
            }
        }
    }

    // ตรวจสอบและลบแถวที่เต็ม
    uint8_t lines_in_move = 0;
    for (int8_t r = BOARD_ROWS - 1; r >= 0; r--) {
        uint8_t full = 1;
        for (uint8_t c = 0; c < BOARD_COLS; c++) {
            if (!board[r][c]) {
                full = 0;
                break;
            }
        }

        if (full) {
            lines_in_move++;
            // ดึงแถวข้างบนลงมาทับ
            for (int8_t kr = r; kr > 0; kr--) {
                for (uint8_t kc = 0; kc < BOARD_COLS; kc++) {
                    board[kr][kc] = board[kr - 1][kc];
                }
            }
            // แถวบนสุดให้เป็นว่าง
            for (uint8_t kc = 0; kc < BOARD_COLS; kc++) {
                board[0][kc] = 0;
            }
            r++; // ตรวจสอบแถวเดิมอีกรอบ
        }
    }

    if (lines_in_move > 0) {
        lines_cleared += lines_in_move;
        if (lines_in_move == 1) score += 100;
        else if (lines_in_move == 2) score += 300;
        else if (lines_in_move == 3) score += 500;
        else if (lines_in_move >= 4) score += 800; // TETRIS!

        if (score > high_score) {
            high_score = score;
        }

        // เร่งความเร็วตามแถวที่ลบได้
        if (drop_interval > 6) {
            drop_interval = 20 - (lines_cleared / 5);
            if (drop_interval < 6) drop_interval = 6;
        }
    }

    // สปอว์นชิ้นบล็อกถัดไป
    piece_type = next_piece;
    next_piece = random_piece();
    piece_rot = 0;
    piece_x = 3;
    piece_y = -1;

    // ถ้าเกิดมาแล้วชนทันที = Game Over
    if (check_collision(piece_x, piece_y, piece_rot)) {
        game_over = 1;
        if (score > high_score) {
            high_score = score;
        }
    }
}

void tetris_init(void) {
    memset(board, 0, sizeof(board));
    score = 0;
    lines_cleared = 0;
    game_over = 0;
    exit_requested = 0;
    drop_ticks = 0;
    drop_interval = 20;
    hold_down_ticks = 0;
    hold_lr_ticks = 0;

    next_piece = random_piece();
    piece_type = random_piece();
    piece_rot = 0;
    piece_x = 3;
    piece_y = -1;
}

void tetris_update(void) {
    if (game_over) return;

    drop_ticks++;
    if (drop_ticks >= drop_interval) {
        drop_ticks = 0;
        if (!check_collision(piece_x, piece_y + 1, piece_rot)) {
            piece_y++;
        } else {
            lock_piece();
        }
    }

    if (score > high_score) {
        high_score = score;
    }
}

void tetris_handle_key(uint8_t key, uint8_t is_new) {
    if (key == 0) {
        hold_down_ticks = 0;
        hold_lr_ticks = 0;
        return;
    }

    if (is_new) {
        hold_down_ticks = 0;
        hold_lr_ticks = 0;

        if (key == 16) {
            exit_requested = 1;
            return;
        }

        if (game_over) {
            if (key == 12 || key == 6 || key == 1) {
                tetris_init();
            }
            return;
        }

        // ปุ่ม 2 หรือ 6: หมุนบล็อก (ROTATE)
        if (key == 2 || key == 6) {
            uint8_t next_rot = (piece_rot + 1) % 4;
            if (!check_collision(piece_x, piece_y, next_rot)) {
                piece_rot = next_rot;
            } else if (!check_collision(piece_x - 1, piece_y, next_rot)) {
                piece_x--;
                piece_rot = next_rot;
            } else if (!check_collision(piece_x + 1, piece_y, next_rot)) {
                piece_x++;
                piece_rot = next_rot;
            }
            return;
        }

        // ปุ่ม 12 หรือ 1: Hard Drop (ทิ้งลงพื้นทันที)
        if (key == 12 || key == 1) {
            while (!check_collision(piece_x, piece_y + 1, piece_rot)) {
                piece_y++;
                score += 2;
            }
            lock_piece();
            return;
        }

        // ปุ่ม 5: เลื่อนซ้าย (LEFT)
        if (key == 5) {
            if (!check_collision(piece_x - 1, piece_y, piece_rot)) {
                piece_x--;
            }
            return;
        }

        // ปุ่ม 7: เลื่อนขวา (RIGHT)
        if (key == 7) {
            if (!check_collision(piece_x + 1, piece_y, piece_rot)) {
                piece_x++;
            }
            return;
        }

        // ปุ่ม 10 หรือ 14: Soft Drop (ทิ้งบล็อกลงเร็ว)
        if (key == 10 || key == 14) {
            if (!check_collision(piece_x, piece_y + 1, piece_rot)) {
                piece_y++;
                score += 1;
            } else {
                lock_piece();
            }
            return;
        }
    } else {
        // --- การกดแช่ (Continuous Hold) ---
        if (game_over) return;

        // กดปุ่มลง 10 หรือ 14 แช่ไว้ -> ทิ้งบล็อกลงอย่างรวดเร็วต่อเนื่อง
        if (key == 10 || key == 14) {
            hold_down_ticks++;
            if (hold_down_ticks >= 2) { // ทุก 40ms
                hold_down_ticks = 0;
                if (!check_collision(piece_x, piece_y + 1, piece_rot)) {
                    piece_y++;
                    score += 1;
                } else {
                    lock_piece();
                }
            }
        }
        // กดปุ่ม 5 หรือ 7 แช่ไว้ -> เลื่อนซ้าย/ขวาต่อเนื่อง
        else if (key == 5 || key == 7) {
            hold_lr_ticks++;
            if (hold_lr_ticks >= 7) { // Initial DAS delay (~140ms)
                if (key == 5 && !check_collision(piece_x - 1, piece_y, piece_rot)) {
                    piece_x--;
                } else if (key == 7 && !check_collision(piece_x + 1, piece_y, piece_rot)) {
                    piece_x++;
                }
                hold_lr_ticks = 5; // Repeat rate (~40ms)
            }
        }
    }
}

void tetris_render(void) {
    oled_clear_buffer();

    // 1. [ส่วนบน] Header แสดงตัวเลขคะแนนปัจจุบัน 5 หลัก
    char score_buf[16];
    snprintf(score_buf, sizeof(score_buf), "%05lu", score);
    oled_draw_str(2, 4, score_buf, 1);

    // แสดงชิ้นบล็อกถัดไปตัวจิ๋ว (NEXT) มุมขวาบน (X: 46, Y: 4)
    for (uint8_t r = 0; r < 2; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (get_piece_block(next_piece, 0, c, r)) {
                oled_fill_rect(46 + c * 4, 3 + r * 4, 3, 3, 1);
            }
        }
    }
    oled_fill_rect(0, 14, 64, 1, 1); // เส้นคั่นบน

    // 2. [ส่วนกลาง] กรอบและกระดานเกม (50x100 px)
    oled_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1, (BOARD_COLS * BLOCK_SIZE) + 2, (BOARD_ROWS * BLOCK_SIZE) + 2, 1);

    // วาดบล็อกที่ติดอยู่บนกระดานแล้ว
    for (uint8_t r = 0; r < BOARD_ROWS; r++) {
        for (uint8_t c = 0; c < BOARD_COLS; c++) {
            if (board[r][c]) {
                int16_t bx = BOARD_OFFSET_X + c * BLOCK_SIZE;
                int16_t by = BOARD_OFFSET_Y + r * BLOCK_SIZE;
                oled_fill_rect(bx, by, BLOCK_SIZE - 1, BLOCK_SIZE - 1, 1);
            }
        }
    }

    // วาดชิ้นบล็อกที่กำลังตกลงมา
    if (!game_over) {
        for (uint8_t r = 0; r < 4; r++) {
            for (uint8_t c = 0; c < 4; c++) {
                if (get_piece_block(piece_type, piece_rot, c, r)) {
                    int8_t gx = piece_x + c;
                    int8_t gy = piece_y + r;
                    if (gy >= 0 && gy < BOARD_ROWS && gx >= 0 && gx < BOARD_COLS) {
                        int16_t bx = BOARD_OFFSET_X + gx * BLOCK_SIZE;
                        int16_t by = BOARD_OFFSET_Y + gy * BLOCK_SIZE;
                        oled_fill_rect(bx, by, BLOCK_SIZE - 1, BLOCK_SIZE - 1, 1);
                    }
                }
            }
        }
    }

    // 3. [ส่วนล่าง] Footer แสดง High Score (HI: 00000)
    oled_fill_rect(0, 118, 64, 1, 1);
    char hi_buf[16];
    snprintf(hi_buf, sizeof(hi_buf), "HI:%05lu", high_score);
    oled_draw_str((64 - 8 * 6) / 2, 120, hi_buf, 1);

    // ป๊อปอัป Game Over
    if (game_over) {
        oled_fill_rect(6, 48, 52, 28, 0); // กล่องดำลบพื้นหลัง
        oled_draw_rect(6, 48, 52, 28, 1); // กรอบขาว
        oled_draw_str(8, 53, "GAME OVER", 1);
        oled_draw_str(8, 65, "12:RETRY", 1);
    }

    oled_update();
}

uint8_t tetris_should_exit(void) {
    return exit_requested;
}
