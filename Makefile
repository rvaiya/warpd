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
	PLATFORM_CFLAGS+=-framework cocoa -g -Wno-unused 

	PLATFORM_FILES=$(shell find src/platform/macos/*.m)
	PLATFORM_OBJECTS=$(PLATFORM_FILES:.m=.o)

	PREFIX=/usr/local
else
	CFLAGS+=-I/usr/include/freetype2

	PLATFORM_CFLAGS+=-lXfixes\
		-lXext\
		-lXi\
		-lXtst\
		-lX11\
		-lXft

	PLATFORM_FILES=$(shell find src/platform/X/*.c)
	PLATFORM_OBJECTS=$(PLATFORM_FILES:.c=.o)
endif

FILES=$(shell find src/*.c)
OBJECTS=$(FILES:.c=.o) $(PLATFORM_OBJECTS)

all: $(OBJECTS)
	$(CC) $(PLATFORM_CFLAGS) $(CFLAGS) -o bin/warpd $(OBJECTS)
assets:
	./gen_assets.py 
clean:
	rm $(OBJECTS)
install:
	install -m644 warpd.1.gz $(DESTDIR)/$(PREFIX)/share/man/man1/
	install -m755 bin/warpd $(DESTDIR)/$(PREFIX)/bin/
uninstall:
	rm $(DESTDIR)/$(PREFIX)/share/man/man1/warpd.1.gz\
		$(DESTDIR)/$(PREFIX)/bin/warpd

.PHONY: all platform assets install uninstall bin
