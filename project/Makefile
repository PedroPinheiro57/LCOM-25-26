PROG = proj

.PATH: devices video game handlers assets

SRCS = main.c keyboard.c mouse.c rtc.c sprites.c game.c handlers.c font.c font_assets.c menu.c

CFLAGS += -pedantic
CFLAGS += -I../pedro/lab3
CFLAGS += -I../pedro/lab4
CFLAGS += -I../pedro/lab5
CFLAGS += -I.

CFLAGS += -O0


MOUSE_LIB = ../pedro/lab4/libmouse.a
KBC_LIB   = ../pedro/lab3/libkbc.a
TIMER_LIB = ../pedro/lab2/libtimer.a
VIDEO_LIB = ../pedro/lab5/libvideo.a

DPADD += ${LIBLCF} ${VIDEO_LIB} ${MOUSE_LIB} ${KBC_LIB} ${TIMER_LIB}
LDADD += -llcf -L../pedro/lab5 -lvideo -L../pedro/lab4 -lmouse -L../pedro/lab3 -lkbc -L../pedro/lab2 -ltimer

IS_PROJ = yes

.include <minix.lcom.mk>
