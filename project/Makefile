PROG = proj

# Adicionamos as novas subpastas ao .PATH
.PATH: controller model view assets assets/ships assets/flames assets/explosions serial

SRCS = main.c mouse.c rtc.c sprites.c game.c handlers.c font.c font_assets.c hand_cursor.c mouse_cursor.c game_menu.c board.c renderer.c uart.c protocol.c

SRCS += Carrier.c Carrier_dead.c Battleship.c Battleship_dead.c Cruiser.c Cruiser_dead.c Submarine.c Submarine_dead.c Destroyer.c Destroyer_dead.c
SRCS += flame_1.c flame_2.c flame_3.c flame_4.c flame_5.c flame_6.c
SRCS += Explosion_1.c Explosion_2.c Explosion_3.c Explosion_4.c Explosion_5.c Explosion_6.c Explosion_7.c Explosion_8.c

CFLAGS += -pedantic
CFLAGS += -I../pedro/lab3
CFLAGS += -I../pedro/lab4
CFLAGS += -I../pedro/lab5
CFLAGS += -I.
CFLAGS += -Iserial
CFLAGS += -O0

VIDEO_LIB = ../pedro/lab5/libvideo.a
MOUSE_LIB = ../pedro/lab4/libmouse.a
KBC_LIB   = ../pedro/lab3/libkbc.a
TIMER_LIB = ../pedro/lab2/libtimer.a

DPADD += ${LIBLCF} ${MOUSE_LIB} ${KBC_LIB} ${TIMER_LIB} ${VIDEO_LIB}
LDADD += -llcf  -L../pedro/lab4 -lmouse -L../pedro/lab3 -lkbc -L../pedro/lab2 -ltimer -L../pedro/lab5 -lvideo

IS_PROJ = yes

.include <minix.lcom.mk>
