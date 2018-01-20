CC = gcc
CFLAGS = -std=c99 -c \
	-Wno-incompatible-pointer-types \
	-Wno-comment \
	-O3 \
	-g \
	-Wall \
	-pedantic \
	$(DEBUG)
#LIBS = -lexplain
LIBS = 

PROGRAM = cons
OBJS = $(SRCS:.c=.o)
SRCS = 	main.c \
	cons.c \
	object.c \
	str.c \
	token.c \
	read.c \
	eval.c

all: $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) $*.c -o $@

clean:
	rm -f $(OBJS) $(PROGRAM)
