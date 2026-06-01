#include "renderer.h"
#include "font.h"
#include "../../pedro/lab5/video.h"
#include "sprites.h"
#include "../controller/game.h"
#include "game_menu.h"

#include "../assets/explosions/Explosion_1.xpm"
#include "../assets/explosions/Explosion_2.xpm"
#include "../assets/explosions/Explosion_3.xpm"
#include "../assets/explosions/Explosion_4.xpm"
#include "../assets/explosions/Explosion_5.xpm"
#include "../assets/explosions/Explosion_6.xpm"
#include "../assets/explosions/Explosion_7.xpm"
#include "../assets/explosions/Explosion_8.xpm"

#include "../assets/flames/flame_1.xpm"
#include "../assets/flames/flame_2.xpm"
#include "../assets/flames/flame_3.xpm"
#include "../assets/flames/flame_4.xpm"
#include "../assets/flames/flame_5.xpm"
#include "../assets/flames/flame_6.xpm"

#include "../assets/ships/Battleship.xpm"
#include "../assets/ships/Battleship_dead.xpm"
#include "../assets/ships/Carrier.xpm"
#include "../assets/ships/Carrier_dead.xpm"
#include "../assets/ships/Cruiser.xpm"
#include "../assets/ships/Cruiser_dead.xpm"
#include "../assets/ships/Destroyer_dead.xpm"
#include "../assets/ships/Destroyer.xpm"
#include "../assets/ships/Submarine.xpm"
#include "../assets/ships/Submarine_dead.xpm"


#define RTC_X 650
#define RTC_Y 20

#define C_EMPTY    0x1a3a5c
#define C_SHIP     0x808080
#define C_HIT      0xFF4500
#define C_MISS     0x00BFFF
#define C_SUNK     0x8B0000
#define C_VALID    0x00FF00
#define C_INVALID  0xFF0000
#define C_HOVER    0xFFFF00
#define C_REMOTE   0xFF8C00

sprite_t *spr_ships[NUM_SHIPS];
sprite_t *spr_ships_dead[NUM_SHIPS];

animated_sprite_t *anim_flame;
animated_sprite_t *anim_explosion;

static bool is_exploding        = false;
static int  expl_col            = 0;
static int  expl_row            = 0;
static int  expl_timer_ticks    = 0;
static bool explosion_finished  = false;
static bool is_waiting_for_turn = false;
static int  wait_ticks          = 0;
static bool expl_is_hit         = false;

static uint8_t num_to_str(uint8_t n, char buf[3]) {
    if (n >= 10) {
        buf[0] = '0' + n / 10;
        buf[1] = '0' + n % 10;
        buf[2] = '\0';
        return 2;
    }
    buf[0] = '0' + n;
    buf[1] = '\0';
    return 1;
}

static void draw_cell(uint8_t col, uint8_t row, uint32_t color) {
    uint16_t px = BOARD_X + col * CELL_SIZE + 1;
    uint16_t py = BOARD_Y + row * CELL_SIZE + 1;
    vg_draw_rectangle_project(px, py, CELL_SIZE - 2, CELL_SIZE - 2, color);
}

void board_draw(board_t *b, bool hide_ships) {
    vg_draw_rectangle_project(BOARD_X - 2, BOARD_Y - 2,
        BOARD_COLS * CELL_SIZE + 4,
        BOARD_ROWS * CELL_SIZE + 4, 0xFFFFFF);

    for (uint8_t row = 0; row < BOARD_ROWS; row++) {
        for (uint8_t col = 0; col < BOARD_COLS; col++) {
            uint32_t color;
            switch (b->grid[row][col]) {
                case CELL_SHIP:  color = hide_ships ? C_EMPTY : C_SHIP; break;
                case CELL_HIT:   color = C_HIT;   break;
                case CELL_MISS:  color = C_MISS;  break;
                case CELL_SUNK:  color = C_SUNK;  break;
                default:         color = C_EMPTY; break;
            }
            draw_cell(col, row, color);
        }
    }

    /* Column labels A-J */
    for (uint8_t col = 0; col < BOARD_COLS; col++) {
        char label[2] = { 'A' + col, '\0' };
        draw_string(label,
            BOARD_X + col * CELL_SIZE + 15,
            BOARD_Y - 30, 0xFFFFFF, 2);
    }

    /* Row labels 1-10 */
    for (uint8_t row = 0; row < BOARD_ROWS; row++) {
        char label[3];
        if (row < 9) {
            label[0] = '1' + row;
            label[1] = '\0';
        } else {
            label[0] = '1';
            label[1] = '0';
            label[2] = '\0';
        }
        draw_string(label,
            BOARD_X - 35,
            BOARD_Y + row * CELL_SIZE + 13, 0xFFFFFF, 2);
    }

    /* Draw ship sprites using type_idx for correct sprite */
    for (int i = 0; i < b->ships_placed; i++) {
        ship_t s = b->ships[i];
        uint16_t px = BOARD_X + s.col * CELL_SIZE + 1;
        uint16_t py = BOARD_Y + s.row * CELL_SIZE + 1;
        bool need_rotation = (s.orient == HORIZONTAL);
        if (s.sunk) {
            sprite_draw_rotated(spr_ships_dead[s.type_idx], px, py, need_rotation);
        } else if (!hide_ships) {
            sprite_draw_rotated(spr_ships[s.type_idx], px, py, need_rotation);
        }
    }

    /* Draw flame animations on hit/sunk cells */
    for (uint8_t row = 0; row < BOARD_ROWS; row++) {
        for (uint8_t col = 0; col < BOARD_COLS; col++) {
            if (b->grid[row][col] == CELL_HIT || b->grid[row][col] == CELL_SUNK) {
                if (is_exploding && col == expl_col && row == expl_row) continue;
                uint16_t px = BOARD_X + col * CELL_SIZE + 1;
                uint16_t py = BOARD_Y + row * CELL_SIZE + 1;
                if (anim_flame != NULL) {
                    anim_flame->x = px + 2;
                    anim_flame->y = py;
                    anim_sprite_draw(anim_flame);
                } else {
                    draw_cell(col, row, C_HIT);
                }
            }
        }
    }

    /* Draw explosion animation */
    if (is_exploding && anim_explosion != NULL) {
        uint16_t px = BOARD_X + expl_col * CELL_SIZE + 1;
        uint16_t py = BOARD_Y + expl_row * CELL_SIZE + 1;
        anim_explosion->x = px;
        anim_explosion->y = py;
        anim_sprite_draw(anim_explosion);
    }
}

void board_draw_preview(board_t *b, int col, int row,
                         uint8_t size, orientation_t orient) {
    if (col < 0 || row < 0) return;
    bool valid = board_can_place(b, col, row, size, orient);
    uint32_t color = valid ? C_VALID : C_INVALID;

    for (uint8_t i = 0; i < size; i++) {
        int c = (orient == HORIZONTAL) ? col + i : col;
        int r = (orient == HORIZONTAL) ? row     : row + i;
        if (c >= BOARD_COLS || r >= BOARD_ROWS) break;
        draw_cell((uint8_t)c, (uint8_t)r, color);
    }

    int ship_idx = b->ships_placed;
    if (ship_idx < NUM_SHIPS) {
        uint16_t px = BOARD_X + col * CELL_SIZE + 1;
        uint16_t py = BOARD_Y + row * CELL_SIZE + 1;
        bool need_rotation = (orient == HORIZONTAL);
        sprite_draw_rotated(spr_ships[ship_idx], px, py, need_rotation);
    }
}

void board_highlight_cell(int col, int row) {
    if (col < 0 || col >= BOARD_COLS) return;
    if (row < 0 || row >= BOARD_ROWS) return;
    draw_cell((uint8_t)col, (uint8_t)row, C_HOVER);
}

void board_highlight_remote_cursor(int col, int row) {
    if (col < 0 || col >= BOARD_COLS) return;
    if (row < 0 || row >= BOARD_ROWS) return;
    draw_cell((uint8_t)col, (uint8_t)row, C_REMOTE);
}

void draw_hud_place(int player, int ship_idx) {
    if (player == 1)
        draw_string("PLAYER 1 - PLACE SHIPS", 224, 20, 0x00BFFF, 2);
    else
        draw_string("PLAYER 2 - PLACE SHIPS", 224, 20, 0xFFD700, 2);

    if (ship_idx < NUM_SHIPS) {
        draw_string("PLACING:", 100, 575, 0xFFFFFF, 2);
        draw_string(SHIP_NAMES[ship_idx], 240, 575, 0x00FF00, 2);
        uint8_t size = SHIP_SIZES[ship_idx];
        for (uint8_t i = 0; i < size; i++)
            vg_draw_rectangle_project(560 + i * 20, 575, 15, 15, 0x808080);
    }

    draw_string("R=ROTATE  CLICK=PLACE", 316, 550, 0x888888, 1);
}

void draw_hud_attack(int player, board_t *enemy) {
    if (player == 1)
        draw_string("PLAYER 1 - YOUR TURN", 240, 20, 0x00BFFF, 2);
    else
        draw_string("PLAYER 2 - YOUR TURN", 240, 20, 0xFFD700, 2);

    draw_string("CLICK TO ATTACK", 280, 558, 0x888888, 2);

    uint8_t hits   = board_count_hits(enemy);
    uint8_t misses = board_count_misses(enemy);
    uint8_t sunk   = enemy->ships_sunk;

    char num[3];

    draw_string("HITS:", 30, 548, 0xFF4500, 2);
    num_to_str(hits, num);
    draw_string(num, 126, 548, 0xFF4500, 2);

    draw_string("MISS:", 30, 568, 0x00BFFF, 2);
    num_to_str(misses, num);
    draw_string(num, 126, 568, 0x00BFFF, 2);

    draw_string("SUNK:", 30, 588, 0xFF6666, 2);
    num_to_str(sunk, num);
    draw_string(num,  126, 588, 0xFF6666, 2);
    draw_string("/5",  142, 588, 0xFF6666, 2);
}

void init_game_sprites(void) {
    spr_ships[0]      = sprite_load((xpm_map_t) Carrier_xpm);
    spr_ships_dead[0] = sprite_load((xpm_map_t) Carrier_dead_xpm);
    spr_ships[1]      = sprite_load((xpm_map_t) Battleship_xpm);
    spr_ships_dead[1] = sprite_load((xpm_map_t) Battleship_dead_xpm);
    spr_ships[2]      = sprite_load((xpm_map_t) Cruiser_xpm);
    spr_ships_dead[2] = sprite_load((xpm_map_t) Cruiser_dead_xpm);
    spr_ships[3]      = sprite_load((xpm_map_t) Submarine_xpm);
    spr_ships_dead[3] = sprite_load((xpm_map_t) Submarine_dead_xpm);
    spr_ships[4]      = sprite_load((xpm_map_t) Destroyer_xpm);
    spr_ships_dead[4] = sprite_load((xpm_map_t) Destroyer_dead_xpm);

    anim_flame = anim_sprite_create(0, 0, 6);
    if (anim_flame != NULL) {
        anim_flame->pixmaps[0] = sprite_load((xpm_map_t) flame_1_xpm);
        anim_flame->pixmaps[1] = sprite_load((xpm_map_t) flame_2_xpm);
        anim_flame->pixmaps[2] = sprite_load((xpm_map_t) flame_3_xpm);
        anim_flame->pixmaps[3] = sprite_load((xpm_map_t) flame_4_xpm);
        anim_flame->pixmaps[4] = sprite_load((xpm_map_t) flame_5_xpm);
        anim_flame->pixmaps[5] = sprite_load((xpm_map_t) flame_6_xpm);
    }

    anim_explosion = anim_sprite_create(0, 0, 8);
    if (anim_explosion != NULL) {
        anim_explosion->pixmaps[0] = sprite_load((xpm_map_t) Explosion_1_xpm);
        anim_explosion->pixmaps[1] = sprite_load((xpm_map_t) Explosion_2_xpm);
        anim_explosion->pixmaps[2] = sprite_load((xpm_map_t) Explosion_3_xpm);
        anim_explosion->pixmaps[3] = sprite_load((xpm_map_t) Explosion_4_xpm);
        anim_explosion->pixmaps[4] = sprite_load((xpm_map_t) Explosion_5_xpm);
        anim_explosion->pixmaps[5] = sprite_load((xpm_map_t) Explosion_6_xpm);
        anim_explosion->pixmaps[6] = sprite_load((xpm_map_t) Explosion_7_xpm);
        anim_explosion->pixmaps[7] = sprite_load((xpm_map_t) Explosion_8_xpm);
    }
}

void destroy_game_sprites(void) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        sprite_destroy(spr_ships[i]);
        sprite_destroy(spr_ships_dead[i]);
    }
    anim_sprite_destroy(anim_flame);
    anim_sprite_destroy(anim_explosion);
}

void start_explosion(int col, int row, bool is_hit) {
    is_exploding       = true;
    explosion_finished = false;
    expl_col           = col;
    expl_row           = row;
    expl_timer_ticks   = 0;
    expl_is_hit        = is_hit;

    if (anim_explosion != NULL)
        anim_explosion->cur_pixmap = 0;
    if (anim_flame != NULL)
        anim_flame->cur_pixmap = 0;
}

void update_animations(void) {
    
    static int flame_ticks = 0;
    flame_ticks++;
    if (flame_ticks % 5 == 0)
        anim_sprite_update(anim_flame);

    if (is_exploding && anim_explosion != NULL) {
        expl_timer_ticks++;
        if (expl_timer_ticks % 5 == 0) {
            anim_sprite_update(anim_explosion);
            if (anim_explosion->cur_pixmap == 0) {
                is_exploding = false;
                if (expl_is_hit) {
                    is_waiting_for_turn = false;
                    explosion_finished  = true;
                } else {
                    is_waiting_for_turn = true;
                    wait_ticks          = 0;
                }
            }
        }
    } else if (is_waiting_for_turn) {
        wait_ticks++;
        if (wait_ticks >= 45) {
            is_waiting_for_turn = false;
            explosion_finished  = true;
        }
    }
}

bool renderer_explosion_finished(void) {
    if (explosion_finished) {
        explosion_finished = false;
        return true;
    }
    return false;
}

bool renderer_is_exploding(void) {
    return is_exploding || is_waiting_for_turn;
}

int renderer_get_expl_col(void) { return expl_col; }
int renderer_get_expl_row(void) { return expl_row; }

void renderer_reset(void) {
    is_exploding        = false;
    explosion_finished  = false;
    is_waiting_for_turn = false;
    expl_col            = 0;
    expl_row            = 0;
    expl_timer_ticks    = 0;
    wait_ticks          = 0;
    expl_is_hit         = false;

    if (anim_explosion != NULL)
        anim_explosion->cur_pixmap = 0;
    if (anim_flame != NULL)
        anim_flame->cur_pixmap = 0;
}

static void rtc_format(const rtc_time_t *t, char buf[9]) {
    buf[0] = '0' + t->hours   / 10;
    buf[1] = '0' + t->hours   % 10;
    buf[2] = ':';
    buf[3] = '0' + t->minutes / 10;
    buf[4] = '0' + t->minutes % 10;
    buf[5] = ':';
    buf[6] = '0' + t->seconds / 10;
    buf[7] = '0' + t->seconds % 10;
    buf[8] = '\0';
}

static void draw_rtc(const rtc_time_t *t) {
    char buf[9];
    rtc_format(t, buf);
    draw_string(buf, RTC_X, RTC_Y, 0xAAAAAA, 2);
}

void game_draw(const game_t *g) {
    video_clear_screen();

    switch (g->tag) {

        case STATE_WAITING_CONNECT:
            if (g->role == ROLE_HOST) {
                draw_string("WAITING FOR", 248, 220, 0x00BFFF, 3);
                draw_string("PLAYER 2...", 248, 270, 0x00BFFF, 3);
                draw_string("(START CLIENT VM NOW)", 132, 380, 0x888888, 2);
            } else {
                draw_string("CONNECTING TO", 208, 220, 0xFFD700, 3);
                draw_string("HOST...", 304, 270, 0xFFD700, 3);
            }
            break;

        case STATE_MAIN_MENU:
            menu_draw_main(g->data.menu.selected);
            draw_rtc(&g->rtc);
            break;

        case STATE_INSTRUCTIONS:
            menu_draw_instructions();
            break;

        case STATE_PLACE_SHIPS_P1:
            if (g->role == ROLE_HOST) {
                board_draw((board_t *)&g->p1_board, false);
                board_draw_preview((board_t *)&g->p1_board,
                    g->data.place.cursor_col, g->data.place.cursor_row,
                    SHIP_SIZES[g->data.place.ship_idx],
                    (orientation_t)g->data.place.orient);
                draw_hud_place(1, g->data.place.ship_idx);
            } else {
                draw_string("PLAYER 1 IS", 248, 220, 0x00BFFF, 3);
                draw_string("PLACING SHIPS...", 192, 270, 0x00BFFF, 3);
                draw_string("PLEASE WAIT", 248, 340, 0x888888, 2);
            }
            draw_rtc(&g->rtc);
            break;

        case STATE_PLACE_SHIPS_P2:
            if (g->role == ROLE_CLIENT) {
                board_draw((board_t *)&g->p2_board, false);
                board_draw_preview((board_t *)&g->p2_board,
                    g->data.place.cursor_col, g->data.place.cursor_row,
                    SHIP_SIZES[g->data.place.ship_idx],
                    (orientation_t)g->data.place.orient);
                draw_hud_place(2, g->data.place.ship_idx);
            } else {
                draw_string("PLAYER 2 IS", 248, 220, 0x00BFFF, 3);
                draw_string("PLACING SHIPS...", 192, 270, 0x00BFFF, 3);
                draw_string("PLEASE WAIT", 248, 340, 0x888888, 2);
            }
            draw_rtc(&g->rtc);
            break;

        case STATE_PLACE_SHIPS_WAITING:
            if (g->role == ROLE_HOST) {
                board_draw((board_t *)&g->p1_board, false);
            } else {
                board_draw((board_t *)&g->p2_board, false);
            }
            draw_string("WAITING FOR", 248, 540, 0x888888, 2);
            draw_string("OPPONENT...", 248, 560, 0x888888, 2);
            draw_rtc(&g->rtc);
            break;

        case STATE_COUNTDOWN: {
            draw_string("GAME STARTS IN", 192, 200, 0x00BFFF, 3);
            char digit[2] = { '0' + g->countdown_seconds, '\0' };
            draw_string(digit, 376, 280, 0xFFD700, 8);
            draw_rtc(&g->rtc);
            break;
        }

        case STATE_TURN_P1:
            if (g->role == ROLE_HOST) {
                board_draw((board_t *)&g->p2_board, true);
                board_highlight_cell(
                    g->data.turn.cursor_col,
                    g->data.turn.cursor_row);
                draw_hud_attack(1, (board_t *)&g->p2_board);
            } else {
                board_draw((board_t *)&g->p2_board, false);
                if (g->remote_cursor_col >= 0)
                    board_highlight_remote_cursor(
                        g->remote_cursor_col,
                        g->remote_cursor_row);
                draw_hud_attack(1, (board_t *)&g->p2_board);
            }
            draw_rtc(&g->rtc);
            break;

        case STATE_TURN_P2:
            if (g->role == ROLE_CLIENT) {
                board_draw((board_t *)&g->p1_board, true);
                board_highlight_cell(
                    g->data.turn.cursor_col,
                    g->data.turn.cursor_row);
                draw_hud_attack(2, (board_t *)&g->p1_board);
            } else {
                board_draw((board_t *)&g->p1_board, false);
                if (g->remote_cursor_col >= 0)
                    board_highlight_remote_cursor(
                        g->remote_cursor_col,
                        g->remote_cursor_row);
                draw_hud_attack(2, (board_t *)&g->p1_board);
            }
            draw_rtc(&g->rtc);
            break;

        case STATE_HANDOVER_P1:
            menu_draw_handover(1);
            draw_rtc(&g->rtc);
            break;

        case STATE_HANDOVER_P2:
            menu_draw_handover(2);
            draw_rtc(&g->rtc);
            break;

        case STATE_PAUSED:
            menu_draw_pause(g->data.pause.selected);
            draw_rtc(&g->rtc);
            break;

        case STATE_GAME_OVER:
            menu_draw_game_over(g->data.game_over.winner);
            break;

        default:
            break;
    }
}
