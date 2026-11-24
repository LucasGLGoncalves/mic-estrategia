# Império Estelar - Makefile (Linux, macOS, MSYS2/MinGW)
CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-format-truncation
LDLIBS  ?= -lm
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
BIN     := imperio

ifeq ($(OS),Windows_NT)
  BIN := imperio.exe
  LDLIBS :=
  CFLAGS += -D__USE_MINGW_ANSI_STDIO=1
endif

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

src/%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -c $< -o $@

test: $(BIN)
	./$(BIN) --selftest

clean:
	rm -f src/*.o $(BIN) imperio.exe

.PHONY: all test clean
