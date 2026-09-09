#include "tetris.h"
#include "ch32fun.h"
#include "ssd1306.h"
#include <string.h>

#define BOARD_COLS      10
#define BOARD_ROWS      20
#define BLOCK_SIZE      5
#define BOARD_OFFSET_X  7
#define BOARD_OFFSET_Y  16
#define TETRIS_SCORE_ADDR  0x08003BC0UL
#define TETRIS_SCORE_MAGIC 0x54455452UL

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
static uint16_t drop_interval = 20;

static uint8_t hold_down_ticks = 0;
static uint8_t hold_lr_ticks = 0;
static uint8_t high_score_loaded = 0;

static uint32_t rng_state = 0x5A5A1234;

static void load_high_score(void) {
    const uint32_t *data = (const uint32_t *)TETRIS_SCORE_ADDR;
    high_score = data[0] == TETRIS_SCORE_MAGIC ? data[1] : 0;
    high_score_loaded = 1;
}

static void save_high_score(void) {
    const uint32_t *old = (const uint32_t *)TETRIS_SCORE_ADDR;
    if (old[0] == TETRIS_SCORE_MAGIC && old[1] >= high_score) return;

    volatile uint32_t *dst = (volatile uint32_t *)TETRIS_SCORE_ADDR;
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;
    FLASH->CTLR = CR_PAGE_ER;
    FLASH->ADDR = (intptr_t)dst;
    FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
    while (FLASH->STATR & FLASH_STATR_BSY);

    FLASH->CTLR = CR_PAGE_PG;
    FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
    FLASH->ADDR = (intptr_t)dst;
    while (FLASH->STATR & FLASH_STATR_BSY);
    for (uint8_t i = 0; i < 16; i++) {
        dst[i] = i == 0 ? TETRIS_SCORE_MAGIC : (i == 1 ? high_score : 0);
        FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
        while (FLASH->STATR & FLASH_STATR_BSY);
    }
    FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
    while (FLASH->STATR & FLASH_STATR_BSY);
    FLASH->CTLR = CR_LOCK_Set;
}

static void u32_to_str_padded(uint32_t val, char *buf, int len) {
    buf[len] = '\0';
    for (int i = len - 1; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
}

static uint8_t random_piece(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (uint8_t)(rng_state % 7);
}

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

static uint8_t check_collision(int8_t nx, int8_t ny, uint8_t nrot) {
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (get_piece_block(piece_type, nrot, c, r)) {
                int8_t gx = nx + c;
                int8_t gy = ny + r;

                if (gx < 0 || gx >= BOARD_COLS || gy >= BOARD_ROWS) {
                    return 1;
                }
                if (gy >= 0 && board[gy][gx]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

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
            for (int8_t kr = r; kr > 0; kr--) {
                for (uint8_t kc = 0; kc < BOARD_COLS; kc++) {
                    board[kr][kc] = board[kr - 1][kc];
                }
            }
            for (uint8_t kc = 0; kc < BOARD_COLS; kc++) {
                board[0][kc] = 0;
            }
            r++;
        }
    }

    if (lines_in_move > 0) {
        lines_cleared += lines_in_move;
        if (lines_in_move == 1) score += 100;
        else if (lines_in_move == 2) score += 300;
        else if (lines_in_move == 3) score += 500;
        else if (lines_in_move >= 4) score += 800;

        if (score > high_score) {
            high_score = score;
        }

        if (drop_interval > 6) {
            drop_interval = 20 - (lines_cleared / 5);
            if (drop_interval < 6) drop_interval = 6;
        }
    }

    piece_type = next_piece;
    next_piece = random_piece();
    piece_rot = 0;
    piece_x = 3;
    piece_y = -1;

    if (check_collision(piece_x, piece_y, piece_rot)) {
        game_over = 1;
        if (score > high_score) {
            high_score = score;
        }
        save_high_score();
    }
}

void tetris_init(void) {
    if (!high_score_loaded) load_high_score();
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
            save_high_score();
            exit_requested = 1;
            return;
        }

        if (game_over) {
            if (key == 12 || key == 6 || key == 1) {
                tetris_init();
            }
            return;
        }

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

        if (key == 12 || key == 1) {
            while (!check_collision(piece_x, piece_y + 1, piece_rot)) {
                piece_y++;
                score += 2;
            }
            lock_piece();
            return;
        }

        if (key == 5) {
            if (!check_collision(piece_x - 1, piece_y, piece_rot)) {
                piece_x--;
            }
            return;
        }

        if (key == 7) {
            if (!check_collision(piece_x + 1, piece_y, piece_rot)) {
                piece_x++;
            }
            return;
        }

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
        if (game_over) return;

        if (key == 10 || key == 14) {
            hold_down_ticks++;
            if (hold_down_ticks >= 2) {
                hold_down_ticks = 0;
                if (!check_collision(piece_x, piece_y + 1, piece_rot)) {
                    piece_y++;
                    score += 1;
                } else {
                    lock_piece();
                }
            }
        } else if (key == 5 || key == 7) {
            hold_lr_ticks++;
            if (hold_lr_ticks >= 7) {
                if (key == 5 && !check_collision(piece_x - 1, piece_y, piece_rot)) {
                    piece_x--;
                } else if (key == 7 && !check_collision(piece_x + 1, piece_y, piece_rot)) {
                    piece_x++;
                }
                hold_lr_ticks = 5;
            }
        }
    }
}

void tetris_render(void) {
    oled_clear_buffer();

    char score_buf[8];
    u32_to_str_padded(score, score_buf, 5);
    oled_draw_str(2, 4, score_buf, 1);

    for (uint8_t r = 0; r < 2; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (get_piece_block(next_piece, 0, c, r)) {
                oled_fill_rect(46 + c * 4, 3 + r * 4, 3, 3, 1);
            }
        }
    }
    oled_fill_rect(0, 14, 64, 1, 1);

    oled_draw_rect(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1, (BOARD_COLS * BLOCK_SIZE) + 2, (BOARD_ROWS * BLOCK_SIZE) + 2, 1);

    for (uint8_t r = 0; r < BOARD_ROWS; r++) {
        for (uint8_t c = 0; c < BOARD_COLS; c++) {
            if (board[r][c]) {
                int16_t bx = BOARD_OFFSET_X + c * BLOCK_SIZE;
                int16_t by = BOARD_OFFSET_Y + r * BLOCK_SIZE;
                oled_fill_rect(bx, by, BLOCK_SIZE - 1, BLOCK_SIZE - 1, 1);
            }
        }
    }

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

    oled_fill_rect(0, 118, 64, 1, 1);
    char hi_buf[10];
    hi_buf[0] = 'H'; hi_buf[1] = 'I'; hi_buf[2] = ':';
    u32_to_str_padded(high_score, &hi_buf[3], 5);
    oled_draw_str((64 - 8 * 6) / 2, 120, hi_buf, 1);

    if (game_over) {
        oled_fill_rect(6, 48, 52, 28, 0);
        oled_draw_rect(6, 48, 52, 28, 1);
        oled_draw_str(8, 53, "GAME OVER", 1);
        oled_draw_str(8, 65, "12:RETRY", 1);
    }

    oled_update();
}

uint8_t tetris_should_exit(void) {
    return exit_requested;
}
