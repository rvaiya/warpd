all:
	-mkdir bin
	gcc -g src/cfg.c src/grid.c src/hints.c src/main.c -o bin/warp -lXfixes -lXtst -lX11 -lXft -I/usr/include/freetype2
debug:
	-mkdir bin
	gcc -g -DDEBUG src/cfg.c src/grid.c src/hints.c src/main.c -o bin/warp -lXfixes -lXtst -lX11 -lXft -I/usr/include/freetype2
man:
	-mkdir bin
	pandoc -s -t man -o - man.md|gzip > warp.1.gz
install:
	install -m755 bin/warp /usr/bin
	install -m644 warp.1.gz /usr/share/man/man1/
