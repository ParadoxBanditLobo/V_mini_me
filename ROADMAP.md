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

Settings:

```ini
idle_bob=false
idle_bob_pixels=3
idle_bob_period_ms=2400
```

The implementation is positional only, with no physics or animation subsystem. It is off by default so existing configurations keep their previous appearance.

### 2. Idle sway

Add the same lightweight concept horizontally.

Likely settings:

```ini
idle_sway=false
idle_sway_pixels=2
idle_sway_period_ms=3000
```

Bob and sway should remain independent so either can be disabled completely.

### 3. Blinking

Allow optional directional blink images such as:

```text
center_blink.png
left_blink.png
right_blink.png
...
```

Use a simple randomized timer. Missing blink images should fall back to the normal image.

### 4. Expression hotkeys

Allow a small number of hotkeys to switch avatar sets or expressions.

Keep this deliberately small. Do not build a scripting system or general state machine around it.

### 5. Multi-PNG boil animation

Allow a small number of alternate PNG frames per state for hand-drawn or boiling-lineart animation.

Example:

```text
center.png
center_2.png
center_3.png
```

Possible controls:

```ini
animation_enabled=true
animation_fps=6
animation_random=true
```

Prefer a small fixed PNG frame set over adding a general animation engine. GIF support is not currently planned.

### 6. Optional graphical launcher

This should be the last planned addition.

The preferred design is a separate optional launcher/configuration utility, likely using raylib + raygui. It should not be linked into the core avatar runtime.

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

A sensible path toward a stable 1.0 is:

1. Keep the existing directional and microphone behavior stable.
2. Idle bob. Done.
3. Add idle sway.
4. Consider blinking.
5. Test Linux and Windows after each addition.
6. Improve documentation and configuration examples as features settle.
7. Declare 1.0 once the core behavior, configuration, and cross-platform builds are dependable.

Expression hotkeys, multi-PNG animation, and the optional GUI launcher do not need to block 1.0.

## Decision rule for future features

Before adding a feature, ask:

1. Does it noticeably improve a simple PNG avatar?
2. Can it be implemented without a large dependency or subsystem?
3. Does it preserve very low runtime resource use?
4. Can users who do not want it leave it completely disabled?

If the answer to most of these is no, it probably does not belong in V_mini_me.
