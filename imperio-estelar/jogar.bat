@echo off
rem Abre o Imperio Estelar com o tamanho ideal de janela (120x36)
cd /d "%~dp0"
chcp 65001 >nul
if not defined WT_SESSION mode con: cols=120 lines=36
imperio.exe %*
