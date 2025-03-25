@echo off
setlocal enabledelayedexpansion

if not exist ..\build mkdir ..\build
pushd ..\build
cl /wd4201 /Zi /Od /nologo /W4 /DDR_DEBUG ..\code\main.c /link /incremental:no user32.lib gdi32.lib d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib winmm.lib
popd

endlocal