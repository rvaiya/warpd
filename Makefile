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
	PLATFORM_FILES=../src/platform/macos/*.m
	PREFIX=/usr/local
else
	CFLAGS+=-lXfixes\
		-lXext\
		-lXi\
		-lXtst\
		-lX11\
		-lXft\
		-I/usr/include/freetype2

	PLATFORM_FILES=../src/platform/X/*.c
endif

all: platform bin
bin:
	-mkdir bin
	$(CC) $(FILES) $(CFLAGS) lib/platform.a -o bin/warpd
platform:
	-mkdir lib
	cd lib && $(CC) -c $(PLATFORM_FILES) $(CFLAGS) && ar rcs platform.a *.o && rm *.o
assets:
	./gen_assets.py 

install:
	install -m644 warpd.1.gz $(DESTDIR)/$(PREFIX)/share/man/man1/
	install -m755 bin/warpd $(DESTDIR)/$(PREFIX)/bin/
uninstall:
	rm $(DESTDIR)/$(PREFIX)/share/man/man1/warpd.1.gz\
		$(DESTDIR)/$(PREFIX)/bin/warpd

.PHONY: all platform assets install uninstall bin
