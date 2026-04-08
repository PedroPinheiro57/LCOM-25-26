PROG = lab3
SRCS = lab3.c kbc.c timer.c utils.c

# Tell Minix make where to look for missing .c files
.PATH: ${.CURDIR}/../lab2

CFLAGS += -pedantic -DLAB3
CFLAGS += -I../lab2

LDADD  += -llcf

.include <minix.lcom.mk>