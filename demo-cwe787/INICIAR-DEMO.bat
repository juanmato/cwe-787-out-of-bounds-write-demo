@echo off
chcp 65001 >nul
title Demo CWE-787 - TechStore
cd /d "%~dp0"
echo.
echo   Iniciando la demo CWE-787 (TechStore)...
echo   Se abrira el navegador en http://localhost:8000
echo.
start "TechStore - Demo CWE-787" techstore.exe
timeout /t 1 /nobreak >nul
start "" http://localhost:8000
echo   Listo. Para detener la demo, cerra la ventana "TechStore - Demo CWE-787".
echo.
