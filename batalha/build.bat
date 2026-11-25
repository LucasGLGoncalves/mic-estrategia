@echo off
rem Compila a Batalha Naval no Windows (MinGW, MSYS2 ou w64devkit)
cd /d "%~dp0"
gcc -std=c11 -Wall -Wextra -O2 main.c jogo.c desenho.c tabuleiro.c ia.c ..\motor\term.c ..\motor\tela.c ..\motor\ui.c ..\motor\rng.c -o batalha.exe
if exist batalha.exe (echo Pronto! Rode batalha.exe) else (echo A compilacao falhou.)
pause
