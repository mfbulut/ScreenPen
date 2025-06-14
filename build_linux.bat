@echo off
if not exist "build\assets\" (
    xcopy /E /I /Y "assets" "build\assets"
)

zig cc src/main.c -I./raylib/linux/include -L./raylib/linux/lib -lraylib -target x86_64-linux-gnu -O3 -s -o ./build/ScreenPen