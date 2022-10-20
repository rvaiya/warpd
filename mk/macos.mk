.PHONY: rel all install clean

CFILES=$(shell find src/*.c)
OBJCFILES=$(shell find src/platform/macos -name '*.m')
OBJECTS=$(CFILES:.c=.o) $(OBJCFILES:.m=.o)

RELFLAGS=-Wl,-adhoc_codesign -framework cocoa -framework carbon

all: $(OBJECTS)
	-mkdir bin
	$(CC) -o bin/warpd $(OBJECTS) -framework cocoa -framework carbon
rel:
	$(CC) -o bin/warpd-arm $(CFILES) $(OBJCFILES) -target arm64-apple-macos $(CFLAGS) $(RELFLAGS)
	$(CC) -o bin/warpd-x86  $(CFILES) $(OBJCFILES) -target x86_64-apple-macos $(CFLAGS) $(RELFLAGS)
	lipo -create bin/warpd-arm bin/warpd-x86 -output bin/warpd && rm -r bin/warpd-*
	-rm -rf dist
	mkdir dist
	sudo DESTDIR=dist make install
	cd dist && tar czvf ../dist.tar.gz $$(find . -type f)
	-rm -rf dist
install:
	mkdir -p $(DESTDIR)/usr/local/bin/ \
		$(DESTDIR)/usr/local/share/man/man1/ \
		$(DESTDIR)/Library/LaunchAgents && \
	install -m644 files/warpd.1.gz $(DESTDIR)/usr/local/share/man/man1 && \
	install -m755 bin/warpd $(DESTDIR)/usr/local/bin/ && \
	install -m644 files/com.warpd.warpd.plist $(DESTDIR)/Library/LaunchAgents
uninstall:
	rm -f $(DESTDIR)/usr/local/share/man/man1/warpd.1.gz \
		$(DESTDIR)/usr/local/bin/warpd \
		$(DESTDIR)/Library/LaunchAgents/com.warpd.warpd.plist
clean:
	rm $(OBJECTS)
