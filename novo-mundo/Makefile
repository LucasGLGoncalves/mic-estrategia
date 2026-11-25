# Novo Mundo - Makefile (Linux, macOS, MSYS2/MinGW)
CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra
LDLIBS  ?= -lm
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
BIN     := novomundo

ifeq ($(OS),Windows_NT)
  BIN := novomundo.exe
  LDLIBS :=
  CFLAGS += -D__USE_MINGW_ANSI_STDIO=1
endif

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

src/%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(BIN)
	./$(BIN) --selftest

clean:
	rm -f src/*.o $(BIN) novomundo.exe

.PHONY: all test clean
