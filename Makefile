COMMIT=$(shell git rev-parse --short HEAD)
VERSION=1.3.1
DESTDIR=
PREFIX=/usr

CFLAGS:=-g\
       -Wall\
       -Wextra\
       -pedantic\
       -Wno-deprecated-declarations\
       -Wno-unused-parameter\
       -std=c99\
       -DVERSION=\"$(VERSION)\"\
       -DCOMMIT=\"$(COMMIT)\"\
       -D_DEFAULT_SOURCE  $(CFLAGS)

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

all: clean $(OBJECTS)
	-mkdir bin
	$(CC)  $(CFLAGS) -o bin/warpd $(OBJECTS) $(PLATFORM_FLAGS)
macos:
	 PLATFORM=macos CFLAGS='-target arm64-apple-macos' $(MAKE) && mv bin/warpd bin/warpd-arm
	 PLATFORM=macos CFLAGS='-target x86_64-apple-macos' $(MAKE) && mv bin/warpd bin/warpd-x86
	 lipo -create bin/warpd-x86 bin/warpd-arm -output bin/warpd &&  rm bin/warpd-*
fmt:
	find . -name '*.[chm]' ! -name 'cfg.[ch]'|xargs clang-format -i
man:
	scdoc < man.md | gzip > warpd.1.gz
clean:
	-rm $(OBJECTS)
install:
	install -m644 warpd.1.gz $(DESTDIR)$(PREFIX)/share/man/man1/
	install -m755 bin/warpd $(DESTDIR)$(PREFIX)/bin/
uninstall:
	rm $(DESTDIR)$(PREFIX)/share/man/man1/warpd.1.gz\
		$(DESTDIR)$(PREFIX)/bin/warpd

.PHONY: all platform assets install uninstall bin
