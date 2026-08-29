@echo off
setlocal

rem Build the lightweight x86-64 Windows executable with LLVM/Clang.
rem Requires clang++ and lld-link in PATH. No Visual Studio project is needed.

set ROOT=%~dp0
set OUT=%ROOT%out-windows-x64
if not exist "%OUT%" mkdir "%OUT%"

for %%L in (kernel32 user32 gdi32 gdiplus winmm) do (
  lld-link /lib /def:"%ROOT%windows-build\%%L.def" /machine:x64 /out:"%OUT%\%%L.lib" || exit /b 1
)

clang++ --target=x86_64-pc-windows-msvc ^
  -I"%ROOT%src\windows" ^
  -c "%ROOT%src\windows\V_mini_me_win.cpp" ^
  -o "%OUT%\V_mini_me_win.obj" ^
  -O2 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector ^
  -fno-builtin -nostdlib -nostdinc++ || exit /b 1

lld-link ^
  /out:"%OUT%\V_mini_me.exe" ^
  /entry:mainCRTStartup ^
  /subsystem:console,6.01 ^
  /machine:x64 ^
  /opt:ref /opt:icf ^
  "%OUT%\V_mini_me_win.obj" ^
  "%OUT%\kernel32.lib" "%OUT%\user32.lib" "%OUT%\gdi32.lib" ^
  "%OUT%\gdiplus.lib" "%OUT%\winmm.lib" || exit /b 1

echo Built %OUT%\V_mini_me.exe
endlocal
