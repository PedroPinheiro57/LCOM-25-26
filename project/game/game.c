#include "game.h"
#include "menu.h"
#include "renderer.h"
#include <lcom/lcf.h>
#include "../video/font.h"
#include "../video/sprites.h"
#include "../devices/keyboard.h"
#include "../devices/rtc.h"
#include "../../pedro/lab5/video.h"
#include "../../pedro/lab3/kbc.h"

#define CURSOR_SIZE  10
#define TICKS_PER_SEC 30   /* timer_set_frequency(0, 30) in main.c */

/* RTC clock position — top-right corner, scale 2 */
/* "HH:MM:SS" = 8 chars * 8px * 2 = 128px wide; x = 800-128-22 = 650 */
#define RTC_X  650
#define RTC_Y   20

static game_t  g;
static bool    over    = false;
static bool    dirty   = true;
static int16_t prev_cx = 0;
static int16_t prev_cy = 0;

/* ------------------------------------------------------------------
 * rtc_format  — build "HH:MM:SS" into buf[9] without sprintf
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------
 * draw_rtc  — render the clock at the top-right of screen.
 * Called from game_draw() for all in-game states.
 * ------------------------------------------------------------------ */
static void draw_rtc(void) {
  char buf[9];
  rtc_format(&g.rtc, buf);
  draw_string(buf, RTC_X, RTC_Y, 0xAAAAAA, 2);
}

void game_init(void) {
  g = (game_t) {
    .tag        = STATE_MAIN_MENU,
    .prev       = STATE_MAIN_MENU,
    .tick_count = 0,
    .data.menu  = { .selected = 0 }
  };
  board_init(&g.p1_board);
  board_init(&g.p2_board);

  /* read RTC once at startup so the clock shows immediately */
  rtc_read_time(&g.rtc);

  font_init();
  cursor_init();
  init_game_sprites();
}

static void transition(game_state_t next) {
  g.prev = g.tag;
  g.tag  = next;
  dirty  = true;
}

/* ------------------------------------------------------------------
 * game_handle_timer
 * Called once per timer tick (30 Hz).
 * Every TICKS_PER_SEC ticks we re-read the RTC.
 * ------------------------------------------------------------------ */
void game_handle_timer(void) {
  g.tick_count++;
  if (g.tick_count >= TICKS_PER_SEC) {
    g.tick_count = 0;
    rtc_read_time(&g.rtc);
  }

  if (renderer_explosion_finished()) {
    board_t *enemy = (g.tag == STATE_TURN_P1) ? &g.p2_board : &g.p1_board;

    if (board_all_sunk(enemy)) {
      g.data.game_over.winner = (g.tag == STATE_TURN_P1) ? 1 : 2;
      transition(STATE_GAME_OVER);
    } else {
      uint8_t row = renderer_get_expl_row();
      uint8_t col = renderer_get_expl_col();
      
      if (enemy->grid[row][col] == CELL_HIT || enemy->grid[row][col] == CELL_SUNK) {
      } else {
          if (g.tag == STATE_TURN_P1) {
            g.data.turn.player = 2;
            transition(STATE_HANDOVER_P2);
          } else {
            g.data.turn.player = 1;
            transition(STATE_HANDOVER_P1);
          }
      }
    }
  }
}

void game_handle_keyboard(uint8_t scancode) {
  uint8_t code = key_get_code(scancode);
  bool    make = key_is_make(scancode);

  switch (g.tag) {

    case STATE_MAIN_MENU:
      if (make && code == KEY_UP)
        g.data.menu.selected = (g.data.menu.selected + 2) % 3;
      if (make && code == KEY_DOWN)
        g.data.menu.selected = (g.data.menu.selected + 1) % 3;
      if (!make && code == KEY_ESC)
        over = true;
if (make && (code == KEY_ENTER || code == KEY_SPACE)) {
        if (!renderer_is_exploding()) { 
          board_t *enemy = (g.tag == STATE_TURN_P1) ? &g.p2_board : &g.p1_board;
          uint8_t  col   = g.data.turn.cursor_col;
          uint8_t  row   = g.data.turn.cursor_row;

          if (!board_already_attacked(enemy, col, row)) {
            board_attack(enemy, col, row); 
            
            bool is_hit = (enemy->grid[row][col] == CELL_HIT || enemy->grid[row][col] == CELL_SUNK);
            
            start_explosion(col, row, is_hit); 
          }
        }
      }
      break;

    case STATE_INSTRUCTIONS:
      /* any key returns to main menu */
      if (make && (code == KEY_ESC || code == KEY_ENTER))
        transition(STATE_MAIN_MENU);
      break;

    case STATE_PLACE_SHIPS_P1:
    case STATE_PLACE_SHIPS_P2: {
      if (make && code == KEY_UP)
        g.data.place.cursor_row = (g.data.place.cursor_row > 0)
                                    ? g.data.place.cursor_row - 1 : 0;
      if (make && code == KEY_DOWN)
        g.data.place.cursor_row = (g.data.place.cursor_row < BOARD_ROWS - 1)
                                    ? g.data.place.cursor_row + 1 : BOARD_ROWS - 1;
      if (make && code == KEY_LEFT)
        g.data.place.cursor_col = (g.data.place.cursor_col > 0)
                                    ? g.data.place.cursor_col - 1 : 0;
      if (make && code == KEY_RIGHT)
        g.data.place.cursor_col = (g.data.place.cursor_col < BOARD_COLS - 1)
                                    ? g.data.place.cursor_col + 1 : BOARD_COLS - 1;

      if (make && code == KEY_R)
        g.data.place.orient ^= 1;

      if (make && code == KEY_ENTER) {
        board_t *b    = (g.tag == STATE_PLACE_SHIPS_P1) ? &g.p1_board : &g.p2_board;
        uint8_t  col  = g.data.place.cursor_col;
        uint8_t  row  = g.data.place.cursor_row;
        uint8_t  size = SHIP_SIZES[g.data.place.ship_idx];
        orientation_t orient = g.data.place.orient;

        if (board_can_place(b, col, row, size, orient)) {
          board_place_ship(b, col, row, size, orient);
          g.data.place.ship_idx++;
          if (g.data.place.ship_idx >= NUM_SHIPS) {
            if (g.tag == STATE_PLACE_SHIPS_P1)
              transition(STATE_HANDOVER_P2);
            else {
              g.data.turn.player     = 1;
              g.data.turn.cursor_col = 0;
              g.data.turn.cursor_row = 0;
              transition(STATE_HANDOVER_P1);
            }
          }
        }
      }

      if (!make && code == KEY_ESC)
        transition(STATE_PAUSED);
      break;
    }

    case STATE_HANDOVER_P1:
      if (make && code == KEY_ENTER) {
        if (g.data.turn.player == 1)
          transition(STATE_TURN_P1);
        else
          transition(STATE_TURN_P2);
      }
      break;

    case STATE_HANDOVER_P2:
      if (make && code == KEY_ENTER) {
        if (g.prev == STATE_PLACE_SHIPS_P1) {
          g.data.place.player     = 2;
          g.data.place.ship_idx   = 0;
          g.data.place.orient     = HORIZONTAL;
          g.data.place.cursor_col = 0;
          g.data.place.cursor_row = 0;
          transition(STATE_PLACE_SHIPS_P2);
        } else {
          transition(g.data.turn.player == 1 ? STATE_TURN_P1 : STATE_TURN_P2);
        }
      }
      break;

    case STATE_TURN_P1:
    case STATE_TURN_P2:
      if (make && code == KEY_UP)
        g.data.turn.cursor_row = (g.data.turn.cursor_row > 0)
                                   ? g.data.turn.cursor_row - 1 : 0;
      if (make && code == KEY_DOWN)
        g.data.turn.cursor_row = (g.data.turn.cursor_row < BOARD_ROWS - 1)
                                   ? g.data.turn.cursor_row + 1 : BOARD_ROWS - 1;
      if (make && code == KEY_LEFT)
        g.data.turn.cursor_col = (g.data.turn.cursor_col > 0)
                                   ? g.data.turn.cursor_col - 1 : 0;
      if (make && code == KEY_RIGHT)
        g.data.turn.cursor_col = (g.data.turn.cursor_col < BOARD_COLS - 1)
                                   ? g.data.turn.cursor_col + 1 : BOARD_COLS - 1;

      if (make && (code == KEY_ENTER || code == KEY_SPACE)) {
        board_t *enemy = (g.tag == STATE_TURN_P1) ? &g.p2_board : &g.p1_board;
        uint8_t  col   = g.data.turn.cursor_col;
        uint8_t  row   = g.data.turn.cursor_row;

        if (!board_already_attacked(enemy, col, row)) {
          board_attack(enemy, col, row);
          if (board_all_sunk(enemy)) {
            g.data.game_over.winner = (g.tag == STATE_TURN_P1) ? 1 : 2;
            transition(STATE_GAME_OVER);
          } else {
            if (g.tag == STATE_TURN_P1) {
              g.data.turn.player = 2;
              transition(STATE_HANDOVER_P1);
            } else {
              g.data.turn.player = 1;
              transition(STATE_HANDOVER_P2);
            }
          }
        }
      }

      if (!make && code == KEY_ESC)
        transition(STATE_PAUSED);
      break;

    case STATE_PAUSED:
      if (make && code == KEY_UP)
        g.data.pause.selected = (g.data.pause.selected + 1) % 2;
      if (make && code == KEY_DOWN)
        g.data.pause.selected = (g.data.pause.selected + 1) % 2;
      if (make && code == KEY_ENTER) {
        if (g.data.pause.selected == 0) transition(g.prev);
        if (g.data.pause.selected == 1) over = true;
      }
      if (!make && code == KEY_ESC)
        transition(g.prev);
      break;

    case STATE_GAME_OVER:
      if (make && code == KEY_ENTER) {
        board_init(&g.p1_board);
        board_init(&g.p2_board);
        g.data.menu.selected = 0;
        transition(STATE_MAIN_MENU);
      }
      if (!make && code == KEY_ESC)
        over = true;
      break;

    default:
      break;
  }
}

void game_handle_mouse(mouse_state_t *ms) {
  int col, row;
  board_pixel_to_cell(ms->x, ms->y, &col, &row);

  switch (g.tag) {
    case STATE_MAIN_MENU: {
      int hover = menu_mouse_hover(ms->x, ms->y);
      cursor_set_mode(hover >= 0 ? CURSOR_HOVER : CURSOR_NORMAL);
      /* only update selection when mouse actually moves — prevents hover from
         overriding keyboard arrows when the cursor is sitting still */
      if (ms->moved && hover >= 0 && hover != g.data.menu.selected)
        g.data.menu.selected = hover;
      if (ms->clicked && hover >= 0) {
        switch (hover) {
          case 0:
            board_init(&g.p1_board);
            board_init(&g.p2_board);
            g.data.place.player     = 1;
            g.data.place.ship_idx   = 0;
            g.data.place.orient     = HORIZONTAL;
            g.data.place.cursor_col = 0;
            g.data.place.cursor_row = 0;
            transition(STATE_PLACE_SHIPS_P1);
            break;
          case 1:
            transition(STATE_INSTRUCTIONS);
            break;
          case 2:
            over = true;
            break;
        }
      }
      break;
    }

    case STATE_PLACE_SHIPS_P1:
    case STATE_PLACE_SHIPS_P2: {
      board_t *b = (g.tag == STATE_PLACE_SHIPS_P1) ? &g.p1_board : &g.p2_board;
      if (col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS) {
        g.data.place.cursor_col = col;
        g.data.place.cursor_row = row;
      }
      if (ms->clicked && col >= 0 && col < BOARD_COLS &&
                         row >= 0 && row < BOARD_ROWS) {
        uint8_t       size   = SHIP_SIZES[g.data.place.ship_idx];
        orientation_t orient = g.data.place.orient;
        if (board_can_place(b, col, row, size, orient)) {
          board_place_ship(b, col, row, size, orient);
          g.data.place.ship_idx++;
          if (g.data.place.ship_idx >= NUM_SHIPS) {
            if (g.tag == STATE_PLACE_SHIPS_P1)
              transition(STATE_HANDOVER_P2);
            else {
              g.data.turn.player     = 1;
              g.data.turn.cursor_col = 0;
              g.data.turn.cursor_row = 0;
              transition(STATE_HANDOVER_P1);
            }
          }
        }
      }
      break;
    }

    case STATE_TURN_P1:
    case STATE_TURN_P2: {
      board_t *enemy = (g.tag == STATE_TURN_P1) ? &g.p2_board : &g.p1_board;
      if (col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS) {
        g.data.turn.cursor_col = col;
        g.data.turn.cursor_row = row;
      }
  
  if (!renderer_is_exploding() && ms->clicked && col >= 0 && col < BOARD_COLS &&
                         row >= 0 && row < BOARD_ROWS &&
                         !board_already_attacked(enemy, col, row)) {
                         
        board_attack(enemy, col, row); 
        
        bool is_hit = (enemy->grid[row][col] == CELL_HIT || enemy->grid[row][col] == CELL_SUNK);
        
        start_explosion(col, row, is_hit);
      }
      break;
    }

    default:
      break;
  }
}

void game_erase_cursor(void) {
  vg_draw_rectangle_project(prev_cx, prev_cy,
                             CURSOR_SIZE + 1, CURSOR_SIZE + 1, 0x000000);
}

void game_save_cursor(int16_t x, int16_t y) {
  prev_cx = x;
  prev_cy = y;
}

void game_draw(void) {
  video_clear_screen(0x000000);

  switch (g.tag) {
    case STATE_MAIN_MENU:
      menu_draw_main(g.data.menu.selected);
      break;

    case STATE_INSTRUCTIONS:
      menu_draw_instructions();
      break;

    case STATE_PLACE_SHIPS_P1:
      board_draw(&g.p1_board, false);
      board_draw_preview(&g.p1_board,
        g.data.place.cursor_col, g.data.place.cursor_row,
        SHIP_SIZES[g.data.place.ship_idx], g.data.place.orient);
      draw_hud_place(1, g.data.place.ship_idx);
      draw_rtc();
      break;

    case STATE_PLACE_SHIPS_P2:
      board_draw(&g.p2_board, false);
      board_draw_preview(&g.p2_board,
        g.data.place.cursor_col, g.data.place.cursor_row,
        SHIP_SIZES[g.data.place.ship_idx], g.data.place.orient);
      draw_hud_place(2, g.data.place.ship_idx);
      draw_rtc();
      break;

    case STATE_HANDOVER_P1:
      menu_draw_handover(1);
      draw_rtc();
      break;

    case STATE_HANDOVER_P2:
      menu_draw_handover(2);
      draw_rtc();
      break;

case STATE_TURN_P1:
      board_draw(&g.p2_board, true);
      if (!board_already_attacked(&g.p2_board, g.data.turn.cursor_col, g.data.turn.cursor_row)) {
          board_highlight_cell(g.data.turn.cursor_col, g.data.turn.cursor_row);
      }
      draw_hud_attack(1, &g.p2_board);
      draw_rtc();
      break;

    case STATE_TURN_P2:
      board_draw(&g.p1_board, true);
      if (!board_already_attacked(&g.p1_board, g.data.turn.cursor_col, g.data.turn.cursor_row)) {
          board_highlight_cell(g.data.turn.cursor_col, g.data.turn.cursor_row);
      }
      draw_hud_attack(2, &g.p1_board);
      draw_rtc();
      break;

    case STATE_PAUSED:
      menu_draw_pause(g.data.pause.selected);
      draw_rtc();
      break;

    case STATE_GAME_OVER:
      menu_draw_game_over(g.data.game_over.winner);
      break;

    default:
      break;
  }
}

bool game_is_over(void) { return over; }
