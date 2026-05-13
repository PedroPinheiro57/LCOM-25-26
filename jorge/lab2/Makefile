# Name of the program (Minix service)
PROG = lab2 

# Source code files for the executable
# We remove timer.c and utils.c from here because they will be in the library instead
SRCS = lab2.c 

# Additional compilation flags
CFLAGS += -pedantic

# --- Library Configuration ---

LIB_NAME = libtimer.a
LIB_SRCS = timer.c utils.c
LIB_OBJS = ${LIB_SRCS:.c=.o}

# Tell make to track the library and look in the current directory (.)
DPADD += ${LIBLCF} ${LIB_NAME}
LDADD += -llcf -L. -ltimer

# Ensure the library and object files are removed during 'make clean'
CLEANFILES += ${LIB_NAME} ${LIB_OBJS}

# --- Compilation Targets ---

# Force 'lib' to build before the 'all' target (the executable)
all: lib

lib: ${LIB_OBJS}
	ar rcs ${LIB_NAME} ${LIB_OBJS}
# ^ THE LINE ABOVE MUST START WITH A TAB CHARACTER ^

# Include LCOM's makefile that does all the "heavy lifting"
.include <minix.lcom.mk>