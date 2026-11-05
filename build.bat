@echo off
setlocal
cd /d "%~dp0"

set "RAYLIB=..\_clang_libs\raylib"

clang --target=x86_64-pc-windows-msvc -O2 -fms-runtime-lib=dll -DNDEBUG -DWINDOWS -fno-stack-protector main.c ^
    -I"%RAYLIB%\include" ^
    -L"%RAYLIB%\lib" -lraylib -ldwmapi -lgdi32 -lwinmm -luser32 -lshell32 ^
    -Wl,/SUBSYSTEM:WINDOWS,/ENTRY:mainCRTStartup,/OPT:REF,/OPT:ICF,/INCREMENTAL:NO -o ScreenPen.exe || exit /b 1

if exist "ScreenPen.exe" llvm-strip --strip-debug "ScreenPen.exe"
