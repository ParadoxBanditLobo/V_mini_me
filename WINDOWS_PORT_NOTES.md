# Windows implementation notes

The Windows x86-64 port is now implemented. This file keeps its original name so existing README links do not break.

## Design goal

The Windows build follows the same priority as the Linux build: keep the program small, native, readable, and inexpensive to leave running beside a game or OBS.

It deliberately avoids Qt, GTK, SDL, .NET, Electron, and a game engine.

## Windows backend

The Windows build uses only system APIs:

- **Win32/User32** - transparent layered window, global cursor position, dragging, message handling
- **GDI/GDI+** - PNG decoding and the 32-bit premultiplied-alpha overlay canvas
- **WinMM `waveIn`** - 16 kHz mono microphone capture for simple RMS threshold detection
- **Kernel32 console/thread/file APIs** - text menu, config I/O, and the avatar session thread

The final executable imports only:

```text
kernel32.dll
user32.dll
gdi32.dll
gdiplus.dll
winmm.dll
```

There is no Visual C++ Redistributable or .NET dependency in the supplied x86-64 build.

## Source layout

```text
src/windows/V_mini_me_win.cpp
src/windows/Win32ApiLite.hpp
windows-build/*.def
build_windows_x64.sh
build_windows_x64.bat
```

`Win32ApiLite.hpp` contains only the Win32 declarations actually used by the program. This is intentional: it keeps the cross-build reproducible without pulling a Windows SDK into the Linux build environment.

## Feature parity

The Windows build currently supports the same core v0.3 behavior:

- text setup menu
- external avatar folder
- scale setting
- horizontal / four-way / quadrant / eight-way cursor direction modes
- optional `*_talking.png` images
- microphone reaction
- talking bounce
- fallback to the normal directional image when a talking image is absent
- left-drag to move the avatar
- right-click to return to setup
- runtime setup / reload / help / quit commands
- remembering the dragged window position in `config.ini`

`mic_device` differs slightly by platform. Blank means the default recording device on both systems. The Windows build accepts a numeric WinMM input-device ID when a specific device is desired.

## Compatibility

The distributed x86-64 executable is linked with a Windows 7 (`6.01`) subsystem target. The APIs selected are intentionally old and broadly available, but Windows 7/8 behavior should still be considered unverified until tested on those systems. Windows 10/11 are the primary practical targets.

A future 32-bit Windows build is feasible from the same design, but is not included yet.

## Building

Linux cross-build with LLVM:

```sh
./build_windows_x64.sh
```

Windows with LLVM in `PATH`:

```text
build_windows_x64.bat
```

The output is `out-windows-x64/V_mini_me.exe`.
