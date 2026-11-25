@echo off
rem Compila o Posto Avancado no Windows (MinGW, MSYS2 ou w64devkit)
cd /d "%~dp0"
gcc -std=c11 -Wall -Wextra -O2 main.c jogo.c piloto.c telas.c salvar.c testes.c ..\motor\term.c ..\motor\tela.c ..\motor\ui.c ..\motor\rng.c ..\motor\ruido.c -o posto.exe
if exist posto.exe (echo Pronto! Rode posto.exe) else (echo A compilacao falhou.)
pause
