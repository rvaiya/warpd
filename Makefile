all:
	-mkdir bin
	gcc warp.c -o bin/warp -lXfixes -lXtst -lX11
man:
	-mkdir bin
	pandoc -s -t man -o - man.md|gzip > warp.1.gz
install:
	install -m755 bin/warp /usr/bin
	install -m644 warp.1.gz /usr/share/man/man1/
