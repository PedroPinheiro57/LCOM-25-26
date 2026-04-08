PROG=lab3

SRCS = lab3.c kbc.c utils.c

LIB_NAME = libkbc.a
LIB_SRCS = kbc.c utils.c
LIB_OBJS = ${LIB_SRCS:.c=.o}

CFLAGS += -I../lab2
CFLAGS += -DLAB3
CPPFLAGS += -pedantic

DPADD += ${LIBLCF} ${LIB_NAME}
LDADD += -llcf -L. -lkbc

LDADD += -L../lab2 -ltimer

CLEANFILES += ${LIB_NAME} ${LIB_OBJS}

all: lib

lib: ${LIB_OBJS}
	ar rcs ${LIB_NAME} ${LIB_OBJS}

.include <minix.lcom.mk>