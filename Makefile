COMMIT=$(shell git rev-parse --short HEAD)
VERSION=1.0.1-beta
DESTDIR=
PREFIX=/usr

FILES=src/cfg.c\
      src/normal.c\
      src/history.c\
      src/warpd.c\
      src/input.c\
      src/hint.c\
      src/grid.c\
      src/mouse.c\
      src/scroll.c\
      -Wall\
      -Wextra

CFLAGS=-g\
       -Wall\
       -Wextra\
       -pedantic\
       -Wno-unused-function\
       -Wno-unused-parameter\
       -Wno-deprecated-declarations\
       -DVERSION=\"$(VERSION)\"\
       -DCOMMIT=\"$(COMMIT)\"

ifeq ($(shell uname), Darwin)
	CFLAGS+=-framework cocoa -g -Wno-unused 
	FILES+=src/platform/macos/*.m
	PREFIX=/usr/local
else
	CFLAGS+=-lXfixes\
		-lXext\
		-lXi\
		-lXtst\
		-lX11\
		-lXft\
		-I/usr/include/freetype2

	FILES+=src/platform/X/*.c
endif

all:
	-mkdir bin
	$(CC) $(FILES) $(CFLAGS) -o bin/warpd
assets:
	./gen_assets.py 

install:
	install -m644 warpd.1.gz $(DESTDIR)/$(PREFIX)/share/man/man1/
	install -m755 bin/warpd $(DESTDIR)/$(PREFIX)/bin/
uninstall:
	rm $(DESTDIR)/$(PREFIX)/share/man/man1/warpd.1.gz\
		$(DESTDIR)/$(PREFIX)/bin/warpd

.PHONY: all assets install uninstall
