PROG = lab5

SRCS = lab5.c

CFLAGS += -pedantic
CFLAGS += -I../lab4

# --- libvideo (built here) ---
VIDEO_LIB      = libvideo.a
VIDEO_LIB_SRCS = video.c
VIDEO_LIB_OBJS = ${VIDEO_LIB_SRCS:.c=.o}

# --- libs reused from previous labs ---
MOUSE_LIB = ../lab4/libmouse.a
KBC_LIB   = ../lab3/libkbc.a
TIMER_LIB = ../lab2/libtimer.a

DPADD += ${LIBLCF} ${VIDEO_LIB} ${MOUSE_LIB} ${KBC_LIB} ${TIMER_LIB}
LDADD += -llcf -L. -lvideo -L../lab4 -lmouse -L../lab3 -lkbc -L../lab2 -ltimer

CLEANFILES += ${VIDEO_LIB} ${VIDEO_LIB_OBJS}

all: lib

lib: ${VIDEO_LIB_OBJS}
	ar rcs ${VIDEO_LIB} ${VIDEO_LIB_OBJS}

.include <minix.lcom.mk>
