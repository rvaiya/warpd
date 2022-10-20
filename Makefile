COMMIT=$(shell git rev-parse --short HEAD)
VERSION=1.3.4
PREFIX?=/usr/local

ifeq ($(shell uname -s), Darwin)
	PLATFORM?=macos
endif

ifeq ($(PLATFORM), macos)
	VERSION:=$(VERSION)-osx
endif

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
	include mk/macos.mk
else
	include mk/linux.mk
endif
