# Colônia (capítulo 29) - Linux, macOS, MSYS2 e w64devkit
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
MOTOR = ../motor/term.c ../motor/tela.c ../motor/ui.c ../motor/rng.c ../motor/ruido.c
FONTES = main.c colonia.c piloto.c paisagem.c telas.c salvar.c $(MOTOR)

colonia: $(FONTES) *.h ../motor/*.h
	$(CC) $(CFLAGS) $(FONTES) -o colonia -lm

# o administrador automático, sem tela: make simula && ./simula
simula: simula.c colonia.c piloto.c *.h
	$(CC) $(CFLAGS) simula.c colonia.c piloto.c $(MOTOR) -o simula -lm

limpa:
	rm -f colonia colonia.exe simula simula.exe colonia.sav

.PHONY: limpa
