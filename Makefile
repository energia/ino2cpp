ifeq ($(OS),Windows_NT)
	_OS := windows
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		_OS := linux
	endif
	ifeq ($(UNAME_S),Darwin)
		_OS := macos
	endif
endif

CC		:= gcc
CFLAGS		:= -Wall -Wextra -g -O0
BIN		:= bin
SRC		:= src
INCLUDE	:= include/pcre2
LIB		:= lib/libpcre2/$(_OS)
LIBRARIES	:= -lpcre2-8

ifeq ($(_OS),windows)
EXECUTABLE	:= ino2cpp.exe
SOURCEDIRS	:= $(SRC)
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
else
EXECUTABLE	:= ino2cpp
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
endif

CINCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))
CLIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))

SOURCES		:= $(wildcard $(patsubst %,%/*.c, $(SOURCEDIRS)))
OBJECTS		:= $(SOURCES:.c=.o)

all: $(BIN)/$(EXECUTABLE)

test: 
	@echo $(OS)
	@echo $(CINCLUDES)
	@echo $(OBJECTS)
	@echo $(SOURCES)

.PHONY: clean


clean:
	-$(RM) $(BIN)/$(EXECUTABLE)
	-$(RM) $(OBJECTS)


run: all
	./$(BIN)/$(EXECUTABLE)

$(OBJECTS): $(SOURCES)
	$(CC) -c $(CFLAGS) $(CINCLUDES) $^ -o $@

$(BIN)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(CLIBS) $^ -o $@ $(LIBRARIES)

