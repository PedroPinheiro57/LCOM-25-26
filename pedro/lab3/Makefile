PROG = lab3

SRCS = lab3.c

CFLAGS += -pedantic -DLAB3
CFLAGS += -I../lab2

LIB_NAME = libkbc.a
LIB_SRCS = kbc.c
LIB_OBJS = ${LIB_SRCS:.c=.o}

DPADD += ${LIBLCF} ${LIB_NAME} ../lab2/libtimer.a
LDADD += -llcf -L. -lkbc -L../lab2 -ltimer

CLEANFILES += ${LIB_NAME} ${LIB_OBJS}

all: lib

lib: ${LIB_OBJS}
	ar rcs ${LIB_NAME} ${LIB_OBJS}

.include <minix.lcom.mk>