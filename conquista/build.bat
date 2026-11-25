@echo off
rem Compila a Conquista no Windows (MinGW, MSYS2 ou w64devkit)
cd /d "%~dp0"
gcc -std=c11 -Wall -Wextra -O2 main.c mapa.c jogo.c ia.c desenho.c ..\motor\term.c ..\motor\tela.c ..\motor\ui.c ..\motor\rng.c ..\motor\ruido.c ..\motor\caminho.c -o conquista.exe
if exist conquista.exe (echo Pronto! Rode conquista.exe) else (echo A compilacao falhou.)
pause
