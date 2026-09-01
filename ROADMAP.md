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
- Configurable idle bob and sway.
- Avatar scaling.
- Human-readable `config.ini`.
- Text-based setup/control interface.
- Minimal runtime dependencies.

This baseline should remain the core of the project.

## Planned additions, easiest to hardest

### 1. Idle bob

Status: implemented.

```ini
idle_bob=false
idle_bob_pixels=3
idle_bob_period_ms=2400
```

### 2. Idle sway

Status: implemented.

```ini
idle_sway=false
idle_sway_pixels=2
idle_sway_period_ms=3000
```

### 3. Quick Expressions / runtime folder switching

Status: implemented for the current test checkpoint.

Each expression is another normal avatar folder rather than a new image/state format.

Example:

```text
avatar/
├── default/
├── happy/
├── annoyed/
└── surprised/
```

Up to eight optional folders can be assigned in the nested Quick Expressions setup menu or with these config keys:

```ini
expression_1=avatar/default
expression_2=avatar/happy
expression_3=avatar/annoyed
expression_4=avatar/surprised
```

While the avatar is running, `E` / `expressions` opens the compact Quick Expressions menu. Choosing a slot changes the current runtime avatar folder without replacing the configured default `avatar_dir`.

The future graphical frontend should call the same conceptual runtime folder-switch behavior.

### 4. Looping PNG frames

Next planned core addition.

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

The same mechanism can cover hand-drawn boiling lineart, blinking, breathing, or other tiny loops. Missing extra frames should simply mean that state stays static.

Likely controls:

```ini
loop_animation=true
loop_fps=6
```

Keep frame discovery and playback deliberately simple. GIF support is not planned.

### 5. Optional graphical launcher/control panel

This is the final planned addition.

The preferred design is a separate optional launcher/configuration utility, likely using raylib + raygui. It must not be linked into the core avatar runtime.

The launcher should:

1. Locate V_mini_me in its own directory.
2. Read the existing `config.ini` format.
3. Present common settings graphically.
4. Expose Quick Expressions as convenient buttons/controls.
5. Save persistent settings back to `config.ini`.
6. Launch/control V_mini_me without adding GUI overhead to the core runtime.

The launcher should remain independently downloadable. The normal V_mini_me package must continue to work without raylib, raygui, a GUI toolkit, or an installer.

## Polish before core 1.0

- Keep the small `V_mini_me` ASCII title in the terminal setup screen.
- Test grouped changes on Linux and Windows.
- Keep documentation and config examples synchronized with behavior.
- Fix regressions rather than expanding scope unnecessarily.

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

1. Direction and microphone behavior stable.
2. Idle bob. Done.
3. Idle sway. Done.
4. Quick Expressions / runtime folder switching. Current checkpoint.
5. Test this checkpoint before stacking looping PNG playback on top.
6. Add lightweight looping PNG frames.
7. Documentation and regression polish.
8. Core 0.9/1.0 candidate.
9. Build the optional graphical launcher/control panel last.

The optional GUI does not need to block a core 1.0 release.

## Decision rule for future features

Before adding a feature, ask:

1. Does it noticeably improve a simple PNG avatar?
2. Can it be implemented without a large dependency or subsystem?
3. Does it preserve very low runtime resource use?
4. Can users who do not want it leave it completely disabled?

If the answer to most of these is no, it probably does not belong in V_mini_me.
