CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -lpthread

all: speed-typer

clean:
	rm -rf speed-typer  speed-typer.dSYM

speed-typer: speed-typer.c util.c util.h interface.c interface.h
	$(CC) $(CFLAGS) -o speed-typer speed-typer.c util.c interface.c -lncurses
