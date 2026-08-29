# V_mini_me

A deliberately small native reactive PNG avatar written in C++ for Linux/X11 and Windows.

V_mini_me maps the mouse position to a discrete PNG direction and can react to microphone volume with optional `*_talking.png` images and a tiny talking bounce. It performs no camera capture, face tracking, speech recognition, browser rendering, or game-engine work.

Version 0.3 adds a text-only setup/control interface so common settings no longer require manually editing `config.ini`. No graphical UI toolkit was added.

## Platform status

- **Linux x86-64:** available.
- **Windows x86-64:** available as a native Win32 build.
- **32-bit Linux:** feasible, but not currently distributed.
- **Native Wayland:** not implemented; the Linux build currently targets X11/XWayland.

## Running

From a terminal:

```sh
./V_mini_me
```

By default the program first shows:

```text
========================================
            V_MINI_ME SETUP
========================================
  1. Avatar folder      avatar/default
  2. Scale              1.00
  3. Direction mode     eightway
  4. Microphone         ON
  5. Talking bounce     ON
  6. Setup on startup   ON

  S. Save and start
  Q. Save and quit
```

The setup menu writes a human-readable `config.ini`. The source repository intentionally does **not** track that runtime file because it can contain machine-local paths or device identifiers. Safe defaults are provided in `config.example.ini`, and the program also works with built-in defaults when `config.ini` does not yet exist.

While the avatar is running, the terminal accepts:

```text
S / setup   return to setup menu
R / reload  reload config.ini and avatar files
H / help    show commands
Q / quit    exit
```

Avatar-window controls:

- Left-drag: move the avatar window.
- Right-click: close the current avatar session and return to the terminal setup menu.
- Closing the avatar window or `Q` in the terminal exits.

If `show_setup_on_start=false`, the avatar starts immediately. Force the setup menu with:

```sh
./V_mini_me --setup
```

A different config file can still be passed:

```sh
./V_mini_me /path/to/config.ini
```

## Running on Windows

Extract the Windows release and double-click `V_mini_me.exe`, or run it from Command Prompt:

```text
V_mini_me.exe
```

The Windows build is a console application on purpose: the same lightweight text setup/control menu remains visible while the transparent avatar window runs. It does **not** require .NET, the Visual C++ Redistributable, Qt, SDL, or a game engine. It links only to Windows system DLLs (`kernel32`, `user32`, `gdi32`, `gdiplus`, and `winmm`).

The current x86-64 executable is linked with a Windows 7 (6.1) subsystem target. Windows 10/11 are the primary tested-intent targets; older versions should be treated as unverified until tested on real systems.

## Choosing an avatar

Option 1 automatically lists folders directly below `avatar/` that contain the required `center.png`. A custom relative or absolute folder can also be entered.

A typical avatar folder can contain:

```text
center.png
left.png
right.png
up.png
down.png
up_left.png
up_right.png
down_left.png
down_right.png
```

Talking variants add `_talking`:

```text
center_talking.png
right_talking.png
up_left_talking.png
...
```

Only `center.png` is mandatory. Missing directional/talking images fall back sensibly. If no talking variant exists, microphone reaction can still be visible through the talking bounce.

## Scaling

The terminal menu accepts a scale from `0.10` through `8.00`:

```text
0.50 = half size
1.00 = original size
2.00 = double size
```

The value remains stored as `scale=` in `config.ini`.

## Microphone reaction

Microphone detection remains intentionally minimal. V_mini_me records 16 kHz mono audio and calculates normalized RMS volume. It does not inspect speech content.

Advanced values remain editable in `config.ini`:

```ini
mic_enabled=true
mic_device=
mic_threshold=0.035
mic_release_ms=250

talk_bounce=true
bounce_pixels=4
bounce_interval_ms=150
```

A blank `mic_device` uses the default recording device. Linux uses a PulseAudio-compatible source (including PipeWire-Pulse on many desktops); Windows uses the default WinMM input device.

If microphone capture is unavailable, the program prints a warning and continues normally.

## Direction modes

- `horizontal` - left / center / right
- `fourway` - center plus dominant cardinal direction
- `quadrants` - center plus four diagonal quadrants
- `eightway` - center plus all eight compass directions

## Privacy-oriented repository defaults

The repository is arranged to reduce accidental publication of local information:

- `config.ini` is ignored because setup can save absolute avatar paths, microphone device identifiers, and window position to it.
- `config.example.ini` contains only safe defaults and is the tracked template.
- Custom avatar folders under `avatar/` are ignored by default; only the bundled `avatar/default/` example is tracked.
- `.env` files, credential/secret directories, common private-key formats, logs, dumps, build output, and editor-local files are ignored.

These rules reduce accidental commits, but Git cannot protect information that is deliberately force-added or already committed in history. Review staged files before publishing changes.

## Lightweight console design

The setup menu is ordinary terminal text. No GTK, Qt, Electron, browser engine, or other GUI toolkit is linked.

While running, Linux terminal input is watched with `poll(2)`. Its short wait replaces the previous fixed sleep, so the application is not running an additional busy input loop. The microphone capture thread remains blocking as before.

## Dependencies

### Linux

The Linux binary uses common system libraries:

- X11 (`libX11`)
- libpng
- standard C/C++ runtime libraries

Microphone support dynamically loads `libpulse-simple.so.0`. It is optional at runtime. On Debian/Devuan systems, typical runtime packages include `libx11-6`, `libpng16-16`, and `libpulse0` when microphone reaction is wanted. Transparency requires an X11 compositor and a 32-bit ARGB visual.

### Windows

The Windows build uses Win32 directly. PNG decoding uses the Windows GDI+ system component and microphone capture uses the long-standing WinMM `waveIn` API. The supplied executable has no MSVC runtime, .NET, or third-party GUI/runtime dependency.

## Building

### Linux

```sh
make
```

Run direction tests:

```sh
make test
```

If you want a config file before first run, copy the safe template:

```sh
cp config.example.ini config.ini
```

### Windows x86-64

The Windows source is in `src/windows/`. The included build scripts use LLVM/Clang and `lld-link` and intentionally do not require a full Windows SDK because the project declares only the small Win32 API surface it uses.

From Linux with LLVM installed:

```sh
./build_windows_x64.sh
```

Or on Windows with `clang++` and `lld-link` in `PATH`:

```text
build_windows_x64.bat
```

The result is written to `out-windows-x64/V_mini_me.exe`.

## Code layout

- `src/main.cpp` - application/session lifecycle and high-level state wiring
- `src/ConsoleMenu.cpp` - startup setup and non-blocking Linux terminal commands
- `src/Direction.cpp` - cursor-to-direction calculations
- `src/AppConfig.cpp` - config reader/writer
- `src/PngImage.cpp` - PNG decoding through libpng
- `src/AvatarSet.cpp` - idle/talking image loading and fallback behavior
- `src/MicrophoneMonitor.cpp` - low-cost microphone RMS capture
- `src/X11AvatarWindow.cpp` - Linux/X11 window, mouse query, dragging, drawing
- `src/windows/V_mini_me_win.cpp` - self-contained lightweight Win32 implementation
- `src/windows/Win32ApiLite.hpp` - minimal Windows API declarations used by the Win32 build

The text menu, avatar format, and config format are intentionally separated from X11-specific code so a future Windows build can reuse most of the application unchanged.

## Windows implementation notes

See `WINDOWS_PORT_NOTES.md` for details on the native Win32 build and why it uses WinMM/GDI+ directly.

## AI development disclosure

V_mini_me was developed with substantial assistance from OpenAI's ChatGPT, including code generation, debugging, documentation, and release preparation. See `AI_DISCLOSURE.md` for the full disclosure.

## License

V_mini_me is released under the MIT License. See `LICENSE`.
