cd /D "%~dp0"
mkdir obj
cl /Foobj\ /Fe:warpd.exe *.c icon.res ..\config.c ..\daemon.c ..\grid.c ..\grid_drw.c ..\hint.c ..\histfile.c ..\history.c ..\input.c ..\mode-loop.c ..\mouse.c ..\normal.c ..\screen.c ..\scroll.c ..\platform\windows\*.c user32.lib gdi32.lib shell32.lib
rmdir /s /q obj
