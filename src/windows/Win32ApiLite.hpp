#pragma once

// Minimal Win32 declarations used by V_mini_me's Windows build.
//
// The project intentionally avoids a heavyweight application framework. This
// header contains only the small slice of Win32/GDI+/WinMM that the program
// needs. It also lets the Linux build environment cross-compile the Windows
// executable without requiring a full Windows SDK installation.

#define WINAPI __stdcall
#define CALLBACK __stdcall
#define DECLSPEC_IMPORT __declspec(dllimport)

using BYTE = unsigned char;
using WORD = unsigned short;
using DWORD = unsigned long;
using UINT = unsigned int;
using INT = int;
using LONG = long;
using ULONG_PTR = unsigned long long;
using DWORD_PTR = unsigned long long;
using UINT_PTR = unsigned long long;
using SIZE_T = unsigned long long;
using WPARAM = unsigned long long;
using LPARAM = long long;
using LRESULT = long long;
using BOOL = int;
using ATOM = unsigned short;
using MMRESULT = unsigned int;

using HANDLE = void*;
using HINSTANCE = void*;
using HMODULE = void*;
using HWND = void*;
using HDC = void*;
using HBITMAP = void*;
using HGDIOBJ = void*;
using HCURSOR = void*;
using HICON = void*;
using HMENU = void*;
using HWAVEIN = void*;
using LPVOID = void*;
using LPCVOID = const void*;
using WCHAR = wchar_t;
using LPWSTR = WCHAR*;
using LPCWSTR = const WCHAR*;
using LPSTR = char*;
using LPCSTR = const char*;

struct POINT { LONG x; LONG y; };
struct SIZE { LONG cx; LONG cy; };
struct RECT { LONG left; LONG top; LONG right; LONG bottom; };

using WNDPROC = LRESULT (CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

struct WNDCLASSEXW {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    INT cbClsExtra;
    INT cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HGDIOBJ hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
    HICON hIconSm;
};

struct MSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
};

struct BLENDFUNCTION {
    BYTE BlendOp;
    BYTE BlendFlags;
    BYTE SourceConstantAlpha;
    BYTE AlphaFormat;
};

struct BITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};

struct RGBQUAD { BYTE rgbBlue; BYTE rgbGreen; BYTE rgbRed; BYTE rgbReserved; };
struct BITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; };

struct WAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
};

struct WAVEHDR {
    LPSTR lpData;
    DWORD dwBufferLength;
    DWORD dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD dwFlags;
    DWORD dwLoops;
    WAVEHDR* lpNext;
    DWORD_PTR reserved;
};

// GDI+ flat API declarations. Only PNG decode functions are needed.
struct GpImage;
struct GpBitmap;
struct GpRect { INT X; INT Y; INT Width; INT Height; };
struct BitmapData {
    UINT Width;
    UINT Height;
    INT Stride;
    INT PixelFormat;
    LPVOID Scan0;
    UINT_PTR Reserved;
};
struct GdiplusStartupInput {
    UINT GdiplusVersion;
    LPVOID DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
};
using GpStatus = INT;

constexpr BOOL TRUE_VALUE = 1;
constexpr BOOL FALSE_VALUE = 0;
constexpr DWORD INVALID_FILE_ATTRIBUTES = 0xFFFFFFFFUL;
constexpr DWORD FILE_ATTRIBUTE_DIRECTORY = 0x00000010UL;
constexpr DWORD GENERIC_READ = 0x80000000UL;
constexpr DWORD GENERIC_WRITE = 0x40000000UL;
constexpr DWORD FILE_SHARE_READ = 0x00000001UL;
constexpr DWORD CREATE_ALWAYS = 2;
constexpr DWORD OPEN_EXISTING = 3;
constexpr DWORD FILE_ATTRIBUTE_NORMAL = 0x00000080UL;
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
constexpr DWORD STD_INPUT_HANDLE = (DWORD)-10;
constexpr DWORD STD_OUTPUT_HANDLE = (DWORD)-11;
constexpr DWORD INFINITE_WAIT = 0xFFFFFFFFUL;
constexpr DWORD WAIT_OBJECT_0 = 0;
constexpr DWORD HEAP_ZERO_MEMORY = 0x00000008UL;
constexpr UINT CP_UTF8 = 65001;

constexpr UINT WM_DESTROY = 0x0002;
constexpr UINT WM_CLOSE = 0x0010;
constexpr UINT WM_MOUSEMOVE = 0x0200;
constexpr UINT WM_LBUTTONDOWN = 0x0201;
constexpr UINT WM_LBUTTONUP = 0x0202;
constexpr UINT WM_RBUTTONUP = 0x0205;
constexpr WPARAM MK_LBUTTON = 0x0001;
constexpr UINT PM_REMOVE = 0x0001;

constexpr DWORD WS_POPUP = 0x80000000UL;
constexpr DWORD WS_EX_TOPMOST = 0x00000008UL;
constexpr DWORD WS_EX_TOOLWINDOW = 0x00000080UL;
constexpr DWORD WS_EX_LAYERED = 0x00080000UL;
constexpr DWORD WS_EX_NOACTIVATE = 0x08000000UL;
constexpr INT SW_SHOWNOACTIVATE = 4;
constexpr INT SM_CXSCREEN = 0;
constexpr INT SM_CYSCREEN = 1;
constexpr BYTE AC_SRC_OVER = 0;
constexpr BYTE AC_SRC_ALPHA = 1;
constexpr DWORD ULW_ALPHA = 0x00000002UL;
constexpr UINT DIB_RGB_COLORS = 0;
constexpr DWORD BI_RGB = 0;
#define IDC_ARROW_VALUE ((LPCWSTR)(ULONG_PTR)32512)

constexpr WORD WAVE_FORMAT_PCM = 1;
constexpr UINT WAVE_MAPPER = 0xFFFFFFFFU;
constexpr DWORD CALLBACK_NULL = 0;
constexpr DWORD WHDR_DONE = 0x00000001UL;
constexpr MMRESULT MMSYSERR_NOERROR = 0;

constexpr UINT ImageLockModeRead = 0x0001;
constexpr INT PixelFormat32bppPARGB = 0x000E200B;

extern "C" {
// kernel32.dll
DECLSPEC_IMPORT HANDLE WINAPI GetStdHandle(DWORD);
DECLSPEC_IMPORT BOOL WINAPI ReadConsoleW(HANDLE, LPVOID, DWORD, DWORD*, LPVOID);
DECLSPEC_IMPORT BOOL WINAPI WriteConsoleW(HANDLE, LPCVOID, DWORD, DWORD*, LPVOID);
DECLSPEC_IMPORT DWORD WINAPI GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
DECLSPEC_IMPORT HMODULE WINAPI GetModuleHandleW(LPCWSTR);
DECLSPEC_IMPORT LPWSTR WINAPI GetCommandLineW();
DECLSPEC_IMPORT DWORD WINAPI GetFileAttributesW(LPCWSTR);
DECLSPEC_IMPORT HANDLE WINAPI CreateFileW(LPCWSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE);
DECLSPEC_IMPORT DWORD WINAPI GetFileSize(HANDLE, DWORD*);
DECLSPEC_IMPORT BOOL WINAPI ReadFile(HANDLE, LPVOID, DWORD, DWORD*, LPVOID);
DECLSPEC_IMPORT BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, DWORD*, LPVOID);
DECLSPEC_IMPORT BOOL WINAPI CloseHandle(HANDLE);
DECLSPEC_IMPORT HANDLE WINAPI GetProcessHeap();
DECLSPEC_IMPORT LPVOID WINAPI HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI HeapFree(HANDLE, DWORD, LPVOID);
DECLSPEC_IMPORT void WINAPI Sleep(DWORD);
DECLSPEC_IMPORT DWORD WINAPI GetTickCount();
DECLSPEC_IMPORT void WINAPI ExitProcess(UINT);
DECLSPEC_IMPORT HANDLE WINAPI CreateThread(LPVOID, SIZE_T, DWORD (WINAPI*)(LPVOID), LPVOID, DWORD, DWORD*);
DECLSPEC_IMPORT DWORD WINAPI WaitForMultipleObjects(DWORD, const HANDLE*, BOOL, DWORD);
DECLSPEC_IMPORT DWORD WINAPI WaitForSingleObject(HANDLE, DWORD);
DECLSPEC_IMPORT LONG WINAPI InterlockedExchange(volatile LONG*, LONG);
DECLSPEC_IMPORT LONG WINAPI InterlockedCompareExchange(volatile LONG*, LONG, LONG);
DECLSPEC_IMPORT INT WINAPI WideCharToMultiByte(UINT, DWORD, LPCWSTR, INT, LPSTR, INT, LPCSTR, BOOL*);
DECLSPEC_IMPORT INT WINAPI MultiByteToWideChar(UINT, DWORD, LPCSTR, INT, LPWSTR, INT);

// user32.dll
DECLSPEC_IMPORT ATOM WINAPI RegisterClassExW(const WNDCLASSEXW*);
DECLSPEC_IMPORT HWND WINAPI CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, INT, INT, INT, INT, HWND, HMENU, HINSTANCE, LPVOID);
DECLSPEC_IMPORT LRESULT WINAPI DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
DECLSPEC_IMPORT BOOL WINAPI ShowWindow(HWND, INT);
DECLSPEC_IMPORT BOOL WINAPI UpdateWindow(HWND);
DECLSPEC_IMPORT BOOL WINAPI PeekMessageW(MSG*, HWND, UINT, UINT, UINT);
DECLSPEC_IMPORT BOOL WINAPI TranslateMessage(const MSG*);
DECLSPEC_IMPORT LRESULT WINAPI DispatchMessageW(const MSG*);
DECLSPEC_IMPORT BOOL WINAPI GetCursorPos(POINT*);
DECLSPEC_IMPORT INT WINAPI GetSystemMetrics(INT);
DECLSPEC_IMPORT BOOL WINAPI UpdateLayeredWindow(HWND, HDC, const POINT*, const SIZE*, HDC, const POINT*, DWORD, const BLENDFUNCTION*, DWORD);
DECLSPEC_IMPORT HWND WINAPI SetCapture(HWND);
DECLSPEC_IMPORT BOOL WINAPI ReleaseCapture();
DECLSPEC_IMPORT BOOL WINAPI DestroyWindow(HWND);
DECLSPEC_IMPORT void WINAPI PostQuitMessage(INT);
DECLSPEC_IMPORT HCURSOR WINAPI LoadCursorW(HINSTANCE, LPCWSTR);

// gdi32.dll
DECLSPEC_IMPORT HDC WINAPI CreateCompatibleDC(HDC);
DECLSPEC_IMPORT BOOL WINAPI DeleteDC(HDC);
DECLSPEC_IMPORT HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
DECLSPEC_IMPORT BOOL WINAPI DeleteObject(HGDIOBJ);
DECLSPEC_IMPORT HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO*, UINT, LPVOID*, HANDLE, DWORD);

// gdiplus.dll
DECLSPEC_IMPORT GpStatus WINAPI GdiplusStartup(ULONG_PTR*, const GdiplusStartupInput*, LPVOID);
DECLSPEC_IMPORT void WINAPI GdiplusShutdown(ULONG_PTR);
DECLSPEC_IMPORT GpStatus WINAPI GdipCreateBitmapFromFile(LPCWSTR, GpBitmap**);
DECLSPEC_IMPORT GpStatus WINAPI GdipGetImageWidth(GpImage*, UINT*);
DECLSPEC_IMPORT GpStatus WINAPI GdipGetImageHeight(GpImage*, UINT*);
DECLSPEC_IMPORT GpStatus WINAPI GdipBitmapLockBits(GpBitmap*, const GpRect*, UINT, INT, BitmapData*);
DECLSPEC_IMPORT GpStatus WINAPI GdipBitmapUnlockBits(GpBitmap*, BitmapData*);
DECLSPEC_IMPORT GpStatus WINAPI GdipDisposeImage(GpImage*);

// winmm.dll
DECLSPEC_IMPORT MMRESULT WINAPI waveInOpen(HWAVEIN*, UINT, const WAVEFORMATEX*, DWORD_PTR, DWORD_PTR, DWORD);
DECLSPEC_IMPORT MMRESULT WINAPI waveInPrepareHeader(HWAVEIN, WAVEHDR*, UINT);
DECLSPEC_IMPORT MMRESULT WINAPI waveInUnprepareHeader(HWAVEIN, WAVEHDR*, UINT);
DECLSPEC_IMPORT MMRESULT WINAPI waveInAddBuffer(HWAVEIN, WAVEHDR*, UINT);
DECLSPEC_IMPORT MMRESULT WINAPI waveInStart(HWAVEIN);
DECLSPEC_IMPORT MMRESULT WINAPI waveInReset(HWAVEIN);
DECLSPEC_IMPORT MMRESULT WINAPI waveInClose(HWAVEIN);
}
