CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -lpthread
all: worm

clean:
	rm -rf worm worm.dSYM

worm: worm.c util.c util.h list.c list.h interface.c interface.h
	$(CC) $(CFLAGS) -o worm worm.c util.c list.c interface.c -lncurses
