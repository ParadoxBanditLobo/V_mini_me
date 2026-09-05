// Build wrapper for the Windows raylib launcher.
//
// raylib and Win32 export several identifiers with the same names
// (Rectangle, CloseWindow, ShowCursor, DrawText).  Include both APIs once with
// the Win32 spellings temporarily renamed, then include the actual launcher
// implementation.  The include guards keep the original source's includes
// from being processed a second time.

#include "raylib.h"

#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define DrawText Win32DrawText
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef DrawText
#undef ShowCursor
#undef CloseWindow
#undef Rectangle

#include "V_mini_me_launcher.cpp"
