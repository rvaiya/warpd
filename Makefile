VERSION=1.3.5
PREFIX?=/usr/local
COMMITSTR=$(shell commit=$$(git rev-parse --short HEAD 2> /dev/null) && echo " (built from: $$commit)")

ifeq ($(shell uname -s), Darwin)
	PLATFORM?=macos
endif

ifeq ($(PLATFORM), macos)
	VERSION:=$(VERSION)-osx
endif

%.o: %.c Makefile mk/*.mk
	$(CC) -c $< -o $@ $(CFLAGS)

CFLAGS:=-g\
       -Wall\
       -Wextra\
       -pedantic\
       -Wno-deprecated-declarations\
       -Wno-unused-parameter\
       -std=c99\
       -DVERSION='"v$(VERSION)$(COMMITSTR)"'\
       -D_DEFAULT_SOURCE \
       -D_FORTIFY_SOURCE=2  $(CFLAGS)

ifeq ($(PLATFORM), macos)
	include mk/macos.mk
else
	include mk/linux.mk
endif

man:
	scdoc < warpd.1.md | gzip > files/warpd.1.gz
