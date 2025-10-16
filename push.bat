@echo off
:: Script para hacer un git push automático
:: ---------------------------------------

:: Mensaje por defecto si no se pasa uno
set MESSAGE=Auto commit

:: Si el usuario escribió un mensaje lo tomamos
if not "%~1"=="" set MESSAGE=%~1

echo ===========================
echo  Haciendo git push...
echo ===========================

:: Agregar todos los cambios
git add .

:: Crear commit con el mensaje
git commit -m "%MESSAGE%"

:: Enviar al repositorio remoto
git push origin main

echo ===========================
echo   ¡Push completado!
echo ===========================

pause