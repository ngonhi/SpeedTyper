CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -lpthread -fsanitize=address

all: worm

clean:
	rm -rf worm worm.dSYM

worm: worm.c util.c util.h list.c list.h
	$(CC) $(CFLAGS) -o worm worm.c util.c list.c -lncurses
