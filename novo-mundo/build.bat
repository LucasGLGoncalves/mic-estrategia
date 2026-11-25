@echo off
rem Compila o Novo Mundo no Windows (MinGW/MSYS2/w64devkit/TDM-GCC ou Visual Studio)
setlocal
cd /d "%~dp0"

where gcc >nul 2>nul
if %errorlevel%==0 (
    echo Compilando com GCC ^(MinGW^)...
    gcc -O2 -std=c11 -D__USE_MINGW_ANSI_STDIO=1 -Wall -Wextra src\*.c -o novomundo.exe
    goto fim
)

where cl >nul 2>nul
if %errorlevel%==0 (
    echo Compilando com Microsoft C ^(MSVC^)...
    cl /nologo /O2 /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /Fe:novomundo.exe src\*.c
    del /q *.obj 2>nul
    goto fim
)

echo Nenhum compilador C encontrado.
echo Instale o MSYS2 (pacote mingw-w64-ucrt-x86_64-gcc) ou o w64devkit,
echo ou abra o "Developer Command Prompt for Visual Studio" e rode este arquivo de novo.
goto sair

:fim
if exist novomundo.exe (
    echo.
    echo Pronto! Rode jogar.bat ou novomundo.exe
) else (
    echo.
    echo A compilacao falhou.
)
:sair
endlocal
pause
