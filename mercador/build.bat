@echo off
rem Compila o Mercador das Estrelas no Windows (MinGW, MSYS2 ou w64devkit)
cd /d "%~dp0"
gcc -std=c11 -Wall -Wextra -O2 main.c mundo.c viagem.c telas.c salvar.c ..\motor\term.c ..\motor\tela.c ..\motor\ui.c ..\motor\rng.c -o mercador.exe
if exist mercador.exe (echo Pronto! Rode mercador.exe) else (echo A compilacao falhou.)
pause
