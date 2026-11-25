# Batalha Naval (capítulo 26) - Linux, macOS, MSYS2 e w64devkit
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
FONTES = main.c jogo.c desenho.c tabuleiro.c ia.c ../motor/term.c ../motor/tela.c ../motor/ui.c ../motor/rng.c

batalha: $(FONTES) *.h ../motor/*.h
	$(CC) $(CFLAGS) $(FONTES) -o batalha -lm

limpa:
	rm -f batalha batalha.exe

.PHONY: limpa
