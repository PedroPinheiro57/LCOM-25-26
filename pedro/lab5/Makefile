PROG = lab5

SRCS = lab5.c

CFLAGS += -pedantic

# --- libvideo (built here) ---
VIDEO_LIB      = libvideo.a
VIDEO_LIB_SRCS = video.c
VIDEO_LIB_OBJS = ${VIDEO_LIB_SRCS:.c=.o}

DPADD += ${LIBLCF} ${VIDEO_LIB}
LDADD += -llcf -L. -lvideo

CLEANFILES += ${VIDEO_LIB} ${VIDEO_LIB_OBJS}

all: lib

lib: ${VIDEO_LIB_OBJS}
	ar rcs ${VIDEO_LIB} ${VIDEO_LIB_OBJS}

.include <minix.lcom.mk>
