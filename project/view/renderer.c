#include "renderer.h"
#include "font.h"
#include "../../pedro/lab5/video.h"
#include "sprites.h"
#include "../controller/game.h"
#include "game_menu.h"

#include "../assets/gameBackground.xpm"
#include "../assets/logo.xpm"
#include "../assets/miss.xpm"

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

sprite_t *spr_ships[NUM_SHIPS];
sprite_t *spr_ships_dead[NUM_SHIPS];
sprite_t *spr_background;

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
static sprite_t *spr_logo       = NULL;
static sprite_t *spr_miss       = NULL;

sprite_t* get_logo_sprite(void) {
    return spr_logo;
}

sprite_t* get_miss_sprite(void) {
    return spr_miss;
}

sprite_t* get_ship_sprite(int index) {
    if (index < 0 || index >= NUM_SHIPS) return NULL;
    return spr_ships[index];
}

sprite_t* get_ship_dead_sprite(int index) {
    if (index < 0 || index >= NUM_SHIPS) return NULL;
    return spr_ships_dead[index];
}

animated_sprite_t* get_anim_flame(void) {
    return anim_flame;
}

animated_sprite_t* get_anim_explosion(void) {
    return anim_explosion;
}

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
    for (uint8_t col = 0; col <= BOARD_COLS; col++)
        vg_draw_rectangle_project(BOARD_X + col * CELL_SIZE - 1, BOARD_Y - 2, 2, BOARD_ROWS * CELL_SIZE + 4, 0xFFFFFF);
    for (uint8_t row = 0; row <= BOARD_ROWS; row++)
        vg_draw_rectangle_project(BOARD_X - 2, BOARD_Y + row * CELL_SIZE - 1, BOARD_COLS * CELL_SIZE + 4, 2, 0xFFFFFF);

    for (uint8_t row = 0; row < BOARD_ROWS; row++) {
        for (uint8_t col = 0; col < BOARD_COLS; col++) {
            uint32_t color = 0;
            bool draw = false;

            switch (b->grid[row][col]) {
                case CELL_SHIP:
                    draw = false; 
                    break;
                case CELL_HIT:
                    draw = false;
                    break;
                case CELL_SUNK:
                    draw = false;
                    break;
                case CELL_MISS:
                    draw = false;
                    if (spr_miss != NULL) {
                        if (is_exploding && col == expl_col && row == expl_row) {
                            break; 
                        }
                        uint16_t px = BOARD_X + col * CELL_SIZE + 1;
                        uint16_t py = BOARD_Y + row * CELL_SIZE + 1;
                        sprite_draw(spr_miss, px, py);
                    }
                    break;
                default:
                    break;
            }
            if (draw) draw_cell(col, row, color);
        }
    }

    for (uint8_t col = 0; col < BOARD_COLS; col++) {
        char label[2] = { 'A' + col, '\0' };
        draw_string(label, BOARD_X + col * CELL_SIZE + 15, BOARD_Y - 30, 0xFFFFFF, 2);
    }

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
        draw_string(label, BOARD_X - 35, BOARD_Y + row * CELL_SIZE + 13, 0xFFFFFF, 2);
    }

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
                }
            }
        }
    }

    if (is_exploding && anim_explosion != NULL) {
        uint16_t px = BOARD_X + expl_col * CELL_SIZE + 1;
        uint16_t py = BOARD_Y + expl_row * CELL_SIZE + 1;
        anim_explosion->x = px;
        anim_explosion->y = py;
        anim_sprite_draw(anim_explosion);
    }
}

void board_draw_preview(board_t *b, int col, int row, uint8_t size, orientation_t orient) {
    if ((uint8_t)col == 255 || col < 0 || row < 0) return;
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
    if (col < 0 || row < 0) return;
    uint16_t px = BOARD_X + col * CELL_SIZE;
    uint16_t py = BOARD_Y + row * CELL_SIZE;
    uint32_t color = 0xFFD700;
    int t = 3; 

    vg_draw_rectangle_project(px, py, CELL_SIZE, t, color); 
    vg_draw_rectangle_project(px, py + CELL_SIZE - t, CELL_SIZE, t, color); 
    vg_draw_rectangle_project(px, py, t, CELL_SIZE, color);
    vg_draw_rectangle_project(px + CELL_SIZE - t, py, t, CELL_SIZE, color); 
}

void board_highlight_remote_cursor(int col, int row) {
    if (col < 0 || row < 0) return;
    uint16_t px = BOARD_X + col * CELL_SIZE;
    uint16_t py = BOARD_Y + row * CELL_SIZE;
    uint32_t color = 0xFF8C00; 
    int t = 3; 

    vg_draw_rectangle_project(px, py, CELL_SIZE, t, color); 
    vg_draw_rectangle_project(px, py + CELL_SIZE - t, CELL_SIZE, t, color); 
    vg_draw_rectangle_project(px, py, t, CELL_SIZE, color);
    vg_draw_rectangle_project(px + CELL_SIZE - t, py, t, CELL_SIZE, color); 
}

void draw_hud_place(int player, int ship_idx, uint32_t timer_seconds) {
    if (player == 1) draw_string("PLAYER 1 - PLACE SHIPS", 200, 30, 0x00BFFF, 2);
    else             draw_string("PLAYER 2 - PLACE SHIPS", 200, 30, 0xFFD700, 2);

    if (ship_idx < NUM_SHIPS) {
        vg_draw_rectangle_project(15, 200, 220, 150, 0x0A1025);

        draw_string("TIME:", 25, 215, 0xAAAAAA, 2);
        draw_stopwatch(timer_seconds, 120, 215);

        draw_string("PLACING:", 25, 250, 0xFFFFFF, 2);
        draw_string(SHIP_NAMES[ship_idx], 25, 280, 0x00FF00, 2);
        
        sprite_draw_mini(spr_ships[ship_idx], 25, 320, true);
    }

    draw_string("R=ROTATE", 200, 560, 0x888888, 2);
    draw_string("CLICK=PLACE", 400, 560, 0x888888, 2);
}

void draw_hud_attack(int player, board_t *enemy, uint32_t timer_seconds, bool is_my_turn) {
    if (player == 1) draw_string("PLAYER 1 - YOUR TURN", 220, 30, 0x00BFFF, 2);
    else             draw_string("PLAYER 2 - YOUR TURN", 220, 30, 0xFFD700, 2);

    vg_draw_rectangle_project(15, 200, 220, 150, 0x0A1025);

    draw_string("TIME:", 25, 215, 0xAAAAAA, 2);
    draw_stopwatch(timer_seconds, 120, 215);

    uint8_t hits = board_count_hits(enemy), misses = board_count_misses(enemy), sunk = enemy->ships_sunk;
    char num[3];

    draw_string("HITS:", 25, 250, 0xFF4500, 2);
    num_to_str(hits, num); draw_string(num, 120, 250, 0xFF4500, 2);

    draw_string("MISS:", 25, 280, 0x00BFFF, 2);
    num_to_str(misses, num); draw_string(num, 120, 280, 0x00BFFF, 2);

    draw_string("SUNK:", 25, 310, 0xFF6666, 2);
    num_to_str(sunk, num); draw_string(num, 120, 310, 0xFF6666, 2); draw_string("/5", 136, 310, 0xFF6666, 2);

    if (is_my_turn) {
        draw_string("CLICK TO ATTACK", 280, 560, 0x888888, 2);
    }
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

    spr_background = sprite_load((xpm_map_t) gameBackground_xpm);
    spr_logo = sprite_load((xpm_map_t) logo_xpm);
    spr_miss = sprite_load((xpm_map_t) miss_xpm);
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

void draw_stopwatch(uint32_t total_seconds, uint16_t x, uint16_t y) {
    uint8_t m = total_seconds / 60;
    uint8_t s = total_seconds % 60;
    char buf[6];
    buf[0] = '0' + (m / 10);
    buf[1] = '0' + (m % 10);
    buf[2] = ':';
    buf[3] = '0' + (s / 10);
    buf[4] = '0' + (s % 10);
    buf[5] = '\0';
    draw_string(buf, x, y, 0xFFFFFF, 2);
}

void game_draw(const game_t *g) {
video_clear_screen();

    bool usar_background = (g->tag != STATE_MAIN_MENU && 
                            g->tag != STATE_INSTRUCTIONS && 
                            g->tag != STATE_WAITING_CONNECT &&
                            g->tag != STATE_COUNTDOWN &&
                            g->tag != STATE_GAME_OVER &&
                            g->tag != STATE_PAUSED);

    if (usar_background && spr_background != NULL) {
        sprite_draw(spr_background, 0, 0); 
    }

    switch (g->tag) {

        case STATE_WAITING_CONNECT:
            if (g->role == ROLE_HOST) {
                draw_string("WAITING FOR", 268, 220, 0x00BFFF, 3);
                draw_string("PLAYER 2...", 268, 270, 0x00BFFF, 3);
                draw_string("(START CLIENT VM NOW)", 232, 380, 0x888888, 2);
            } else {
                draw_string("CONNECTING TO", 244, 220, 0xFFD700, 3);
                draw_string("HOST...", 316, 270, 0xFFD700, 3);
            }
            break;

        case STATE_MAIN_MENU:
            menu_draw_main(g->data.menu.selected);            
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
                draw_hud_place(1, g->data.place.ship_idx, g->timer_seconds);
            } else {
                draw_string("PLAYER 1 IS", 268, 220, 0x00BFFF, 3);
                draw_string("PLACING SHIPS...", 208, 270, 0x00BFFF, 3);
                draw_string("PLEASE WAIT", 312, 340, 0x888888, 2);
            }
            break;

        case STATE_PLACE_SHIPS_P2:
            if (g->role == ROLE_CLIENT) {
                board_draw((board_t *)&g->p2_board, false);
                board_draw_preview((board_t *)&g->p2_board,
                    g->data.place.cursor_col, g->data.place.cursor_row,
                    SHIP_SIZES[g->data.place.ship_idx],
                    (orientation_t)g->data.place.orient);
                draw_hud_place(2, g->data.place.ship_idx, g->timer_seconds);
            } else {
                draw_string("PLAYER 2 IS", 268, 220, 0xFFD700, 3);
                draw_string("PLACING SHIPS...", 208, 270, 0xFFD700, 3);
                draw_string("PLEASE WAIT", 312, 340, 0x888888, 2);
            }
            break;

        case STATE_PLACE_SHIPS_WAITING:
            if (g->role == ROLE_HOST) {
                draw_string("PLAYER 2 IS", 268, 220, 0xFFD700, 3);
                draw_string("PLACING SHIPS...", 208, 270, 0xFFD700, 3);
                draw_string("PLEASE WAIT", 312, 340, 0x888888, 2);
            } else {
                draw_string("WAITING FOR", 268, 220, 0x00BFFF, 3);
                draw_string("HOST...", 316, 270, 0x00BFFF, 3);
            }
            break;

        case STATE_COUNTDOWN: {
            draw_string("GAME STARTS IN", 232, 200, 0x00BFFF, 3);
            char digit[2] = { '0' + g->countdown_seconds, '\0' };
            draw_string(digit, 376, 280, 0xFFD700, 8);
            break;
        }

    case STATE_TURN_P1:
            if (g->role == ROLE_HOST) {
                board_draw((board_t *)&g->p2_board, true);
                if (!renderer_is_exploding()) {
                    board_highlight_cell(g->data.turn.cursor_col, g->data.turn.cursor_row);
                }
                draw_hud_attack(1, (board_t *)&g->p2_board, g->timer_seconds, true); 
            } else {
                board_draw((board_t *)&g->p2_board, false);
                if (g->remote_cursor_col >= 0 && !renderer_is_exploding()) {
                    board_highlight_remote_cursor(g->remote_cursor_col, g->remote_cursor_row);
                }
                draw_hud_attack(1, (board_t *)&g->p2_board, g->timer_seconds, false); 
            }
            break;

        case STATE_TURN_P2:
            if (g->role == ROLE_CLIENT) {
                board_draw((board_t *)&g->p1_board, true);
                if (!renderer_is_exploding()) {
                    board_highlight_cell(g->data.turn.cursor_col, g->data.turn.cursor_row);
                }
                draw_hud_attack(2, (board_t *)&g->p1_board, g->timer_seconds, true); 
            } else {
                board_draw((board_t *)&g->p1_board, false);
                if (g->remote_cursor_col >= 0 && !renderer_is_exploding()) {
                    board_highlight_remote_cursor(g->remote_cursor_col, g->remote_cursor_row);
                }
                draw_hud_attack(2, (board_t *)&g->p1_board, g->timer_seconds, false); 
            }
            break;

        case STATE_HANDOVER_P1:
            menu_draw_handover(1);
            break;

        case STATE_HANDOVER_P2:
            menu_draw_handover(2);
            break;

        case STATE_PAUSED:
            menu_draw_pause(g->data.pause.selected);
            break;

        case STATE_GAME_OVER:
            menu_draw_game_over(g->data.game_over.winner);
            break;

        default:
            break;
    }
}
