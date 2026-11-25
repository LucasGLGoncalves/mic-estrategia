# Posto Avançado - o esqueleto do capítulo 46 (Linux, macOS, MSYS2, w64devkit)
# SEU JOGO: troque o nome do programa e acrescente os seus arquivos em FONTES.
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
PROGRAMA = posto
MOTOR = ../motor/term.c ../motor/tela.c ../motor/ui.c ../motor/rng.c ../motor/ruido.c
FONTES = main.c jogo.c piloto.c telas.c salvar.c testes.c $(MOTOR)

$(PROGRAMA): $(FONTES) *.h ../motor/*.h
	$(CC) $(CFLAGS) $(FONTES) -o $(PROGRAMA) -lm

teste: $(PROGRAMA)
	./$(PROGRAMA) --testa

simula: $(PROGRAMA)
	./$(PROGRAMA) --simula 8 --semente 123

limpa:
	rm -f $(PROGRAMA) $(PROGRAMA).exe posto.sav posto.sav.tmp

.PHONY: teste simula limpa
