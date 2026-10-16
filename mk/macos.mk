.PHONY: rel all install clean sign signing-setup install-user uninstall-user

CFILES=$(shell find src/*.c)
OBJCFILES=$(shell find src/platform/macos -name '*.m')
OBJECTS=$(CFILES:.c=.o) $(OBJCFILES:.m=.o)

RELFLAGS=-framework cocoa -framework carbon

# Code signing identity. Defaults to the private identity created by
# `make signing-setup` if present (keeps the Accessibility permission across
# rebuilds), otherwise ad-hoc signing ('-').
SIGN_IDENTITY?=$(shell security find-certificate -c warpd-dev >/dev/null 2>&1 && echo warpd-dev || echo -)

USER_PREFIX?=$(HOME)/.local
USER_AGENT=$(HOME)/Library/LaunchAgents/com.warpd.warpd.plist

all: $(OBJECTS)
	-mkdir bin
	$(CC) -o bin/warpd $(OBJECTS) -framework cocoa -framework carbon
	$(MAKE) sign
sign:
	@[ "$(SIGN_IDENTITY)" = "-" ] && echo "Signing ad-hoc (run 'make signing-setup' to keep the Accessibility permission across rebuilds)" || true
	codesign --force --identifier warpd -s "$(SIGN_IDENTITY)" bin/warpd
signing-setup:
	./codesign/setup-identity.sh
rel: clean
	$(CC) -o bin/warpd-arm $(CFILES) $(OBJCFILES) -target arm64-apple-macos $(CFLAGS) $(RELFLAGS)
	$(CC) -o bin/warpd-x86  $(CFILES) $(OBJCFILES) -target x86_64-apple-macos $(CFLAGS) $(RELFLAGS)
	lipo -create bin/warpd-arm bin/warpd-x86 -output bin/warpd && rm -r bin/warpd-*
	$(MAKE) sign
	-rm -rf tmp dist
	mkdir tmp dist
	DESTDIR=tmp make install
	cd tmp && tar czvf ../dist/macos-$(VERSION).tar.gz $$(find . -type f)
	-rm -rf tmp
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
# Per-user install, no sudo required. (Re)starts the warpd agent.
install-user:
	mkdir -p $(USER_PREFIX)/bin $(USER_PREFIX)/share/man/man1 $(HOME)/Library/LaunchAgents
	install -m644 files/warpd.1.gz $(USER_PREFIX)/share/man/man1
	install -m755 bin/warpd $(USER_PREFIX)/bin/
	sed 's|/usr/local/bin/warpd|$(USER_PREFIX)/bin/warpd|' files/com.warpd.warpd.plist > $(USER_AGENT)
	-launchctl bootout gui/$$(id -u) $(USER_AGENT) 2>/dev/null
	launchctl bootstrap gui/$$(id -u) $(USER_AGENT)
uninstall-user:
	-launchctl bootout gui/$$(id -u) $(USER_AGENT) 2>/dev/null
	rm -f $(USER_AGENT) $(USER_PREFIX)/bin/warpd $(USER_PREFIX)/share/man/man1/warpd.1.gz
clean:
	-rm $(OBJECTS)
