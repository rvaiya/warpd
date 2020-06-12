CC=gcc
CFLAGS=-lXfixes -lXext -lXi -lXtst -lX11 -lXft -I/usr/include/freetype2 -DCOMMIT=\"$(shell git describe --no-match --always --abbrev=40 --dirty)\" -o bin/warpd -Wall -Wextra

SRC=src/cfg.c src/input.c src/hint_drw.c src/discrete.c src/grid.c src/hints.c src/dbg.c src/main.c 

all:
	-mkdir bin
	$(CC) $(SRC) $(CFLAGS) 
debug:
	-mkdir bin
	$(CC) -g -DDEBUG $(SRC) $(CFLAGS) 
assets:
	python3 gen_assets.py
	pandoc -s -t man -o - man.md|gzip > warpd.1.gz
install:
	install -m755 bin/warpd /usr/bin
	install -m644 warpd.1.gz /usr/share/man/man1/
