PROG = lab4
 
SRCS = lab4.c
 
CFLAGS += -pedantic
CFLAGS += -I../lab3  
                      
# --- libmouse (built here) ---
MOUSE_LIB      = libmouse.a
MOUSE_LIB_SRCS = mouse.c
MOUSE_LIB_OBJS = ${MOUSE_LIB_SRCS:.c=.o}
 
# --- libkbc (reused from lab3) ---
KBC_LIB     = ../lab3/libkbc.a
 
DPADD += ${LIBLCF} ${MOUSE_LIB} ${KBC_LIB} ../lab2/libtimer.a
LDADD += -llcf -L. -lmouse -L../lab3 -lkbc -L../lab2 -ltimer
 
CLEANFILES += ${MOUSE_LIB} ${MOUSE_LIB_OBJS}
 
all: lib
 
lib: ${MOUSE_LIB_OBJS}
	ar rcs ${MOUSE_LIB} ${MOUSE_LIB_OBJS}
 
.include <minix.lcom.mk>
 