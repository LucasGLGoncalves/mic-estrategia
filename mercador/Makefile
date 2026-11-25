# Mercador das Estrelas (capítulo 27) - Linux, macOS, MSYS2 e w64devkit
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
FONTES = main.c mundo.c viagem.c telas.c salvar.c ../motor/term.c ../motor/tela.c ../motor/ui.c ../motor/rng.c

mercador: $(FONTES) *.h ../motor/*.h
	$(CC) $(CFLAGS) $(FONTES) -o mercador -lm

limpa:
	rm -f mercador mercador.exe mercador.sav simula simula.exe

.PHONY: limpa

# o simulador de equilíbrio (capítulo 27): make simula && ./simula 500
simula: simula.c mundo.c telas.c *.h
	$(CC) $(CFLAGS) simula.c mundo.c telas.c ../motor/term.c ../motor/tela.c ../motor/ui.c ../motor/rng.c -o simula -lm
