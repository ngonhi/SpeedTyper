CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -lpthread

all: worm

clean:
	rm -rf worm worm.dSYM

worm: worm.c util.c util.h utility.c utility.h
	$(CC) $(CFLAGS) -o worm worm.c util.c utility.c -lncurses
	
