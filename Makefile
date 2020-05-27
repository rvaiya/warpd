CC=gcc
CFLAGS=-lXfixes -lXtst -lX11 -lXft -I/usr/include/freetype2 -DCOMMIT=\"$(shell git show-ref -s HEAD)\" -o bin/warp 
SRC=src/cfg.c src/discrete.c src/grid.c src/hints.c src/main.c 

all:
	-mkdir bin
	$(CC) $(SRC) $(CFLAGS) 
debug:
	-mkdir bin
	$(CC) -g -DDEBUG $(SRC) $(CFLAGS) 
assets:
	python3 gen_assets.py
	pandoc -s -t man -o - man.md|gzip > warp.1.gz
install:
	install -m755 bin/warp /usr/bin
	install -m644 warp.1.gz /usr/share/man/man1/
