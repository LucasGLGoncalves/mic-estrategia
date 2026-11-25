# Makefile do Reino de Hamurabi (capítulo 13)
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
OBJETOS = main.o reino.o eventos.o entrada.o

hamurabi: $(OBJETOS)
	$(CC) $(CFLAGS) $(OBJETOS) -o hamurabi

main.o: main.c entrada.h reino.h
	$(CC) $(CFLAGS) -c main.c

reino.o: reino.c reino.h entrada.h eventos.h
	$(CC) $(CFLAGS) -c reino.c

eventos.o: eventos.c eventos.h reino.h entrada.h
	$(CC) $(CFLAGS) -c eventos.c

entrada.o: entrada.c entrada.h
	$(CC) $(CFLAGS) -c entrada.c

limpa:
	rm -f $(OBJETOS) hamurabi hamurabi.exe

.PHONY: limpa
