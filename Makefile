COMMIT=$(shell git rev-parse --short HEAD)
VERSION=1.2.2
DESTDIR=
PREFIX=/usr

CFLAGS=-g\
       -Wall\
       -Wextra\
       -pedantic\
       -Wno-unused-parameter\
       -std=c99\
       -DVERSION=\"$(VERSION)\"\
       -DCOMMIT=\"$(COMMIT)\"

ifeq ($(PLATFORM), macos)
	PLATFORM_FLAGS=-framework cocoa

	PLATFORM_FILES=$(shell find src/platform/macos/*.m)
	PLATFORM_OBJECTS=$(PLATFORM_FILES:.m=.o)

	PREFIX=/usr/local
else ifeq ($(PLATFORM), wayland)
	PLATFORM_FLAGS=-lwayland-client\
			-lxkbcommon\
			-lcairo\
			-lrt\

	PLATFORM_FILES=$(shell find src/platform/wayland/ -name '*.c')
	PLATFORM_OBJECTS=$(PLATFORM_FILES:.c=.o)
else
	CFLAGS+=-I/usr/include/freetype2
	PLATFORM_FLAGS=-lXfixes\
			-lXext\
			-lXinerama\
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
	-mkdir bin
	$(CC)  $(CFLAGS) -o bin/warpd $(OBJECTS) $(PLATFORM_FLAGS)
fmt:
	find . -name '*.[chm]' ! -name 'cfg.[ch]'|xargs clang-format -i
assets:
	./gen_assets.py 
clean:
	-rm $(OBJECTS)
install:
	install -m644 warpd.1.gz $(DESTDIR)$(PREFIX)/share/man/man1/
	install -m755 bin/warpd $(DESTDIR)$(PREFIX)/bin/
uninstall:
	rm $(DESTDIR)$(PREFIX)/share/man/man1/warpd.1.gz\
		$(DESTDIR)$(PREFIX)/bin/warpd

.PHONY: all platform assets install uninstall bin
