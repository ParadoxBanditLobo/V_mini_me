// Windows build wrapper for the optional raylib launcher.
//
// The launcher only needs kernel/process/file/pipe declarations from Win32.
// It does not call User32 or GDI directly; raylib owns the GUI/window layer.
// Excluding those portions of windows.h prevents collisions with raylib names
// such as Rectangle, CloseWindow, ShowCursor, DrawText and LoadImage.

#include "raylib.h"

#define WIN32_LEAN_AND_MEAN
#define NOUSER
#define NOGDI
#include <windows.h>

#include "V_mini_me_launcher.cpp"
