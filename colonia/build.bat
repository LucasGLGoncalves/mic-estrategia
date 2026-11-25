@echo off
rem Compila a Colonia no Windows (MinGW, MSYS2 ou w64devkit)
cd /d "%~dp0"
gcc -std=c11 -Wall -Wextra -O2 main.c colonia.c piloto.c paisagem.c telas.c salvar.c ..\motor\term.c ..\motor\tela.c ..\motor\ui.c ..\motor\rng.c ..\motor\ruido.c -o colonia.exe
if exist colonia.exe (echo Pronto! Rode colonia.exe) else (echo A compilacao falhou.)
pause
