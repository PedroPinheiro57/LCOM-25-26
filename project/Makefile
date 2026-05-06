PROG = proj

SRCS = main.c

CFLAGS += -pedantic
CFLAGS += -I../pedro/lab3
CFLAGS += -I../pedro/lab4

MOUSE_LIB = ../pedro/lab4/libmouse.a
KBC_LIB   = ../pedro/lab3/libkbc.a
TIMER_LIB = ../pedro/lab2/libtimer.a

DPADD += ${LIBLCF} ${MOUSE_LIB} ${KBC_LIB} ${TIMER_LIB}
LDADD += -llcf -L../pedro/lab4 -lmouse -L../pedro/lab3 -lkbc -L../pedro/lab2 -ltimer


.include <minix.lcom.mk>
