# V_mini_me Roadmap

This file is the working reference for the intended scope and development direction of V_mini_me.

The main design goal is to remain a deliberately small, native reactive PNG avatar with very low CPU, RAM, GPU, and dependency overhead. New features should add visible usefulness without turning the project into a general-purpose animation or tracking suite.

## Current baseline

V_mini_me currently provides:

- Native C++ implementation.
- Linux/X11 and native Windows builds.
- Transparent PNG avatar window.
- Mouse-direction tracking.
- Horizontal, four-way, quadrant, and eight-way direction modes.
- Microphone RMS detection.
- Optional talking image variants.
- Talking bounce.
- Avatar scaling.
- Human-readable `config.ini`.
- Text-based setup/control interface.
- Minimal runtime dependencies.

This baseline should remain the core of the project.

## Planned additions, easiest to hardest

### 1. Idle bob

Status: implemented.

A gentle configurable vertical movement around the avatar's normal resting position.

```ini
idle_bob=false
idle_bob_pixels=3
idle_bob_period_ms=2400
```

The implementation is positional only, with no physics or animation subsystem.

### 2. Idle sway

Status: implemented.

The same lightweight idea horizontally. Bob and sway are independent.

```ini
idle_sway=false
idle_sway_pixels=2
idle_sway_period_ms=3000
```

### 3. Folder-based expression switching

Treat each expression as another normal avatar folder rather than introducing a new image/state format.

Example:

```text
avatar/
├── default/
├── happy/
├── annoyed/
└── surprised/
```

A small number of configurable hotkeys should switch between these folders quickly. Each folder keeps the same normal directional/talking file rules, so expression switching reuses the existing avatar-loading model.

Prefer a simple folder swap/reload over a general state machine.

### 4. Looping PNG frames

This replaces the earlier separate blinking and "boil animation" ideas with one mechanism.

Users draw the frames themselves and V_mini_me simply loops through sequential PNG files for the active direction/state.

Example:

```text
up.png
up_2.png
up_3.png

up_talking.png
up_talking_2.png
up_talking_3.png
```

The same mechanism can be used for hand-drawn boiling lineart, blinking, breathing, or other tiny loops. Missing extra frames should simply mean that state stays static.

Likely controls:

```ini
loop_animation=true
loop_fps=6
```

Keep frame discovery and playback deliberately simple. GIF support is not planned.

### 5. Optional graphical launcher

This is the final planned addition.

The preferred design is a separate optional launcher/configuration utility, likely using raylib + raygui. It must not be linked into the core avatar runtime.

Proposed layout:

```text
V_mini_me/
├── V_mini_me
├── config.ini
├── avatar/
└── V_mini_me_launcher   # optional download
```

The launcher should:

1. Locate V_mini_me in its own directory.
2. Read the existing `config.ini` format.
3. Present common settings graphically.
4. Save those settings back to `config.ini`.
5. Launch V_mini_me.
6. Exit so the GUI library consumes no resources while the avatar is running.

The launcher should remain independently downloadable. The normal V_mini_me package must continue to work without raylib, raygui, a GUI toolkit, or an installer.

## Features intentionally out of scope

Unless the project's goals change substantially, avoid adding:

- Webcam or computer-vision tracking.
- Skeletal/deformation model systems.
- General physics systems.
- Full animation timelines.
- Speech recognition.
- Embedded scripting languages.
- Plugin frameworks.
- Web-based UI runtimes.
- Heavy GUI frameworks in the core runtime.
- A game engine dependency for the avatar runtime.

## Release direction

1. Keep the existing directional and microphone behavior stable.
2. Idle bob. Done.
3. Idle sway. Done.
4. Add folder-based expression switching.
5. Add lightweight looping PNG frames if the folder-switching work remains stable.
6. Test Linux and Windows after meaningful groups of changes.
7. Improve documentation/config examples as features settle.
8. Build the optional graphical launcher last.

The launcher does not need to block a core 1.0 release.

## Decision rule for future features

Before adding a feature, ask:

1. Does it noticeably improve a simple PNG avatar?
2. Can it be implemented without a large dependency or subsystem?
3. Does it preserve very low runtime resource use?
4. Can users who do not want it leave it completely disabled?

If the answer to most of these is no, it probably does not belong in V_mini_me.
