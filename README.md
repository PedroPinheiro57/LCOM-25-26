# Template


## AI Generated TODOS

---

## Phase 1 — Foundation (both work in parallel)

### Task 1 — Project skeleton and Makefile
- Create folder structure (`devices/`, `video/`, `game/`, `assets/`)
- Write blank `main.c` with empty `proj_main_loop`
- Write Makefile linking all previous labs
- Verify `make` compiles clean and `lcom_run proj` runs without crashing

### Task 2 — Copy and extend video from lab5
- Copy `video.c/h` from lab5 into `video/`
- Add `video_clear_screen(uint32_t color)`
- Add `video_get_hres()` and `video_get_vres()` accessors
- Test: switch to `0x115` mode, fill screen with solid color, return to terminal

---

## Phase 2 — Device Layer

### Task 3 — Timer
- Write `devices/timer.h` with extern declarations for `libtimer.a`
- Verify timer ticks increment correctly inside a test loop in `main.c`
- Establish convention: 60 ticks = 1 second

### Task 4 — Keyboard
- Write `devices/keyboard.c/h`
- Define `kbc_status_t` union with named bitfields for clean flag access
- Define all scancode constants needed (ESC, ENTER, arrows, R, SPACE)
- Implement `key_is_make` and `key_get_code` helpers
- Test: print key name on terminal for every keypress (before graphics)

### Task 5 — Mouse
- Write `devices/mouse.c/h`
- Implement `mouse_state_t` struct tracking x, y, lb, rb, clicked, moved
- Implement `mouse_state_update` with PS/2 packet parsing and Y-axis inversion
- Clamp cursor to screen boundaries using `video_get_hres/vres`
- Test: print cursor x/y to terminal on every packet

### Task 6 — RTC
- Extend `rtc.c/h` from lab with time registers (hours, minutes, seconds)
- Add `rtc_time` struct and `rtc_read_time` function
- Handle BCD to binary conversion same as existing `rtc_read_date`
- Test: print time to terminal every second using timer ticks

---

## Phase 3 — Rendering Layer

### Task 7 — Cursor rendering
- Write `video/sprites.c/h`
- Implement `cursor_draw(x, y)` — simple cross or arrow shape
- Implement `cursor_erase(x, y)` — redraw background color at old position
- Track `prev_x`, `prev_y` in main loop to erase before redrawing
- Test: cursor visibly follows mouse movement on screen

### Task 8 — Sprite drawing
- Implement `sprite_draw(xpm, x, y)` in `sprites.c`
- Create `assets/pixmaps.h` with placeholder XPM arrays (can reuse lab5 pic1)
- Test: draw a sprite at a fixed position on screen

### Task 9 — Text rendering
- Implement `draw_char(char c, uint16_t x, uint16_t y, uint32_t color)` using bitmap font XPM
- Implement `draw_string(char *s, uint16_t x, uint16_t y, uint32_t color)`
- Used for: player labels, hit/miss counts, RTC clock, menu options
- Test: draw "BATTLESHIP" title on screen

---

## Phase 4 — Game Logic Layer

### Task 10 — Board data structure
- Write `game/board.c/h`
- Define `cell_state_t` enum:
  - CELL_EMPTY, CELL_SHIP, CELL_HIT, CELL_MISS, CELL_SUNK
- Define `ship_t` struct:
  - anchor col/row, size, orientation, hits, sunk flag
- Define `board_t` struct:
  - 10x10 grid, 5 ships, ships placed count, ships sunk count
- Implement:
  - `board_init`
  - `board_can_place`
  - `board_place_ship`
  - `board_attack`
  - `board_all_sunk`
- Test: unit test with printf before graphics

### Task 11 — Board rendering
- Implement `board_draw(board_t *b, uint16_t sx, uint16_t sy, bool hide_ships)`
  - hide ships during enemy attack phase
- Draw each cell as colored rectangle based on state
- Draw grid lines

#### Board preview
- Implement:
  `board_draw_preview(board_t *b, uint16_t sx, uint16_t sy, uint8_t col, uint8_t row, uint8_t size, orientation_t orient)`
- Draw ghost ship:
  - green = valid placement
  - red = invalid placement

#### Highlight
- Implement:
  `board_highlight_cell(uint16_t sx, uint16_t sy, uint8_t col, uint8_t row)`
- Yellow highlight for hovered cell

- Test: draw both boards side by side

---

## Phase 5 — Menu and Screens

### Task 12 — Main menu
- Write `game/menu.c/h`
- Implement `menu_draw_main(int selected)`
  - title + 3 options (Play, Instructions, Exit)
- Keyboard:
  - UP/DOWN = navigate
  - ENTER = select
- Mouse:
  - hover highlights
  - click selects
- Test: menu fully interactive

### Task 13 — Secondary screens
- `menu_draw_handover(int player)`
  - "Pass screen to Player X"
- `menu_draw_pause(int selected)`
  - Resume / Quit
- `menu_draw_game_over(int winner)`
  - winner screen + replay/exit
- `menu_draw_instructions()`
  - rules display

---

## Phase 6 — State Machine and Integration

### Task 14 — Game state machine
- Write `game/game.c/h`
- Define `game_state_t`
- Define `game_t`:
  - boards, state, cursor, ship placement, orientation, timer ticks
- Implement:
  - `game_init`
  - `game_handle_key`
  - `game_handle_mouse`
  - `game_handle_timer`
  - `game_draw`

### Task 15 — Main event loop
- Implement full `proj_main_loop` in `main.c`
- Subscribe:
  - timer, keyboard, mouse
- Initialize:
  - video mode, RTC, game state
- Run `driver_receive` loop
- Call `game_draw` every frame
- Cleanup devices on exit

### Task 16 — Full flow integration
- Wire full flow:
  - Main menu → P1 place → handover → P2 place → handover → P1 attack → ...
  - → Game over → Main menu
- Verify:
  - ESC pauses anywhere
  - resume/quit works
  - replay resets cleanly

---

## Phase 7 — Polish

### Task 17 — RTC clock on screen
- Display HH:MM:SS
- Update every second
- Use `draw_string`

### Task 18 — Hit and miss feedback
- Flash hit cells temporarily
- Different visuals for sunk ships
- Show hit/miss counters

### Task 19 — Turn timer (optional)
- Countdown bar per turn
- Auto-skip on timeout

### Task 20 — Ship sprites
- Replace rectangles with XPM sprites
- Draw ships during placement confirmation

### Task 21 — Final testing and cleanup
- Full gameplay testing
- Remove debug prints
- Ensure no crashes/leaks
- Verify clean exit to terminal
- Compile with zero warnings

---


## Getting started

Welcome to your LCOM code repository. This is where your team should deliver all the required artifacts, including code.
Please take your time to get acquainted with GitLab and its functionalities. The way your team uses Git and GitLab to collaborate will be evaluated. 

## Boilerplate

In this repository, you will find some pre-loaded files and an initial setup of your team's project board. 
Along the semester, you will be adding files and folders to this repository.
Make sure you expand on the issues and milestones for your project, helping your team to coordinate and meet all the deadlines. Major deadlines are already setup but you should add your own sub-issues, additional issues, and deadlines. 

## GitLab Setup 

### Add your files

* [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
* [Add files using the command line](https://docs.gitlab.com/topics/git/add_files/#add-files-to-a-git-repository) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://gitlab.up.pt/lcom-26/template.git
git branch -M main
git push -uf origin main
```

### Integrate with your tools

* [Set up project integrations](https://gitlab.up.pt/lcom-26/template/-/settings/integrations)

### Collaborate with your team

* [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
* [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
* [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
* [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
* [Set auto-merge](https://docs.gitlab.com/user/project/merge_requests/auto_merge/)

### Test and Deploy

Use the built-in continuous integration in GitLab.

* [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/)
* [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
* [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
* [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
* [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

## License
For open source projects, say how it is licensed.

## Declaration of Responsible AI Use

We declare that:

1. We are responsible for all code and documentation in this repository, and we understand that we must be able to explain and justify any part of it on request.  
2. We have used AI-based tools (e.g., code assistants, chatbots, or generators) only to support my learning, not to bypass the intended learning outcomes or any assessment rules.  
3. Wherever AI tools contributed to this work, we have:  
   - Used them within the limits set by the course policies and institutional regulations.  
   - Reviewed, tested, and, where necessary, edited the outputs, taking full responsibility for their correctness, originality, and legality.  
   - Ensured that no confidential, personal, or sensitive data were shared with AI tools.  
4. We have not used AI tools to generate complete solutions that we present as entirely our own unaided work, and we have avoided plagiarism, whether from AI outputs or other sources.  
5. If asked, we will provide details of which tools we used, for which files or parts of the project, and how we verified and adapted their outputs.

Signed: `<student name>`, `<student name>`, `<student name>`, `<student name>`  
Date: `<date>`

## Authors and acknowledgment

LCOM Project for group GRUPO_2LEIC<m><n>_<p>.
Group members:

<first name> <family name> (<email address>)
<first name> <family name> (<email address>)
<first name> <family name> (<email address>)
<first name> <family name> (<email address>)

