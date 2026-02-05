# Windows native build configuration
CC=gcc
WINDRES=windres

# Compiler flags
CFLAGS=-DWINDOWS -DVERSION='"v1.3.5"'
LDFLAGS=-luser32 -lgdi32 -lshell32 -lcomctl32 -mwindows

# Source files (excluding Linux warpd.c)
CORE_SOURCES=$(filter-out src/warpd.c, $(wildcard src/*.c))
PLATFORM_SOURCES=$(wildcard src/platform/windows/*.c)
WINDOWS_SOURCES=$(wildcard src/windows/*.c)

ALL_SOURCES=$(CORE_SOURCES) $(PLATFORM_SOURCES) $(WINDOWS_SOURCES)
OBJECTS=$(ALL_SOURCES:.c=.o)

# Icon resource
ICON_RC=src/windows/icon.rc
ICON_RES=src/windows/icon.res

# Output
TARGET=bin/warpd.exe

# Default target
all: $(TARGET)

# Compile icon resource
$(ICON_RES): $(ICON_RC)
	@echo Compiling icon resource...
	$(WINDRES) $< -O coff -o $@

# Compile C files
%.o: %.c
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) $(ICON_RES)
	@echo Linking $(TARGET)...
	@mkdir -p bin
	$(CC) -o $@ $(OBJECTS) $(ICON_RES) $(LDFLAGS)
	@echo Build complete: $(TARGET)

# Debug build
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean $(TARGET)

# Clean
clean:
	@echo Cleaning...
	-@rm -f $(OBJECTS)
	-@rm -f $(ICON_RES)
	-@rm -f $(TARGET)
	@echo Clean complete.

.PHONY: all debug clean
