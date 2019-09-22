all:
	-mkdir bin
	gcc -lXfixes -lXtst -lX11 warp.c -o bin/warp
man:
	-mkdir bin
	pandoc -s -t man -o - man.md|gzip > warp.1.gz
install:
	install -m755 bin/warp /usr/bin
	install -m644 warp.1.gz /usr/share/man/man1/
