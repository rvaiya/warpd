#CC=cl.exe
CC=x86_64-w64-mingw32-gcc
CFLAGS+=-DWINDOWS -luser32 -lgdi32 -mwindows
OBJFILES=$(shell find src/*.c src/windows/*.c src/platform/windows/*.c ! -name 'warpd.c' )

ifeq ($(CC), cl.exe)
OBJFILES:=$(OBJFILES:%.c=%.obj)
else
OBJFILES:=$(OBJFILES:%.c=%.o)
endif

%.obj: %.c
	$(CC) /c /Fo:$@ $<

all: $(OBJFILES)
	-mkdir bin
	$(CC) -o bin/warpd.exe src/windows/icon.res $(OBJFILES) $(CFLAGS)
	#$(CC) /Fe:bin/warpd.exe $(OBJFILES) user32.lib gdi32.lib
clean:
	rm $(OBJFILES)
	rm -rf bin
