WAYLAND=1
X=1

CFILES=$(shell find src/platform/linux/*.c src/*.c)

ifdef WAYLAND
	CFLAGS+=-lwayland-client\
		-lxkbcommon\
		-lcairo\
		-lrt\

	CFILES+=$(shell find src/platform/linux/wayland/ -name '*.c')
endif

ifdef X
	CFLAGS+=-I/usr/include/freetype2/\
		-lXfixes\
		-lXext\
		-lXinerama\
		-lXi\
		-lXtst\
		-lX11\
		-lXft

	CFILES+=$(shell find src/platform/linux/X/*.c)
endif

OBJECTS=$(CFILES:.c=.o)

all: $(OBJECTS)
	-mkdir bin
	$(CC)  -o bin/warpd $(OBJECTS) $(CFLAGS)
man:
	scdoc < man.md | gzip > warpd.1.gz
clean:
	-rm $(OBJECTS)
	-rm -r bin
install:
	install -m644 warpd.1.gz $(DESTDIR)$(PREFIX)/share/man/man1/
	install -m755 bin/warpd $(DESTDIR)$(PREFIX)/bin/
uninstall:
	rm $(DESTDIR)$(PREFIX)/share/man/man1/warpd.1.gz\
		$(DESTDIR)$(PREFIX)/bin/warpd

.PHONY: all platform assets install uninstall bin
