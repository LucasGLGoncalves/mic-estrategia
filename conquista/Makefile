# Conquista (capítulo 28) - Linux, macOS, MSYS2 e w64devkit
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
MOTOR = ../motor/term.c ../motor/tela.c ../motor/ui.c ../motor/rng.c ../motor/ruido.c ../motor/caminho.c
FONTES = main.c mapa.c jogo.c ia.c desenho.c $(MOTOR)

conquista: $(FONTES) *.h ../motor/*.h
	$(CC) $(CFLAGS) $(FONTES) -o conquista -lm

# o computador contra ele mesmo, sem tela: make simula && ./simula 50
simula: simula.c mapa.c jogo.c ia.c *.h
	$(CC) $(CFLAGS) simula.c mapa.c jogo.c ia.c $(MOTOR) -o simula -lm

limpa:
	rm -f conquista conquista.exe simula simula.exe

.PHONY: limpa
