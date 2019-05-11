CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -lpthread -fsanitize=address

all: worm

clean:
	rm -rf worm worm.dSYM

worm: worm.c util.c util.h
	$(CC) $(CFLAGS) -o worm worm.c util.c -lncurses
