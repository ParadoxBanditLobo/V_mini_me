# V_mini_me Roadmap

This file is the working reference for the intended scope and development direction of V_mini_me.

The main design goal is to remain a deliberately small, native reactive PNG avatar with very low CPU, RAM, GPU, and dependency overhead. New features should be judged primarily by whether they add visible usefulness without pushing the project toward a full VTuber suite.

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

## Near-term priorities

### 1. Idle motion

Add very small configurable motion so static avatars feel less rigid.

Planned options:

- Idle vertical bob.
- Idle horizontal sway.
- Independent enable/disable switches.
- Configurable pixel amplitude.
- Configurable motion period/speed.
- Optional talking-motion multiplier if it remains simple.

Implementation should use simple positional offsets rather than a physics or animation system.

Example configuration direction:

```ini
idle_bob=true
bob_pixels=3
bob_period_ms=2200

idle_sway=false
sway_pixels=2
sway_period_ms=3000

talk_motion_multiplier=1.25
```

Resource target: effectively negligible additional CPU and memory use.

### 2. Benchmark and document resource use

Before significantly expanding the feature set, measure the project against comparable lightweight avatar software.

Primary comparison target:

- Veadotube Mini.

Secondary comparisons where practical:

- PNGTuber Plus.
- Other small PNGTuber/reactive-avatar tools.

Record at least:

- Idle CPU usage.
- Talking CPU usage.
- RAM usage.
- GPU usage where measurable.
- Executable/download size.
- Test avatar resolution and number of loaded images.

Benchmarks should be performed on the same hardware and under comparable conditions. Avoid making exact performance claims without measurements.

## Optional lightweight additions

These are candidates, not commitments.

### Blinking

Optional directional blink images such as:

```text
center_blink.png
left_blink.png
right_blink.png
...
```

Use a simple randomized timer. Missing blink images should fall back cleanly to the normal image.

This is considered a good fit if implementation remains small.

### Multi-PNG boil animation

Possible later support for a small number of alternate PNG frames per state, primarily for hand-drawn or "boiling lineart" animation.

Example:

```text
center.png
center_2.png
center_3.png
```

Potential controls:

```ini
animation_enabled=true
animation_fps=6
animation_random=true
```

Prefer a small fixed frame set over adding a general animation engine.

True GIF support is currently not a priority. Multi-PNG cycling provides most of the intended visual effect with simpler code and predictable resource use.

### Expression hotkeys

Potentially allow a small number of hotkeys to switch avatar sets or expressions.

Keep this simple. Avoid building a general state-machine or scripting system unless there is clear user demand.

### Update-rate control

Consider an explicit update/render-rate option so users can trade responsiveness for even lower resource consumption.

Example:

```ini
update_hz=30
```

## Optional graphical launcher

A GUI should not be integrated into the core avatar runtime unless there is a strong reason to do so.

The preferred approach is an **optional separate launcher/configuration utility**, likely using raylib + raygui.

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

Suggested GUI controls:

- Avatar folder selection.
- Scale.
- Direction mode.
- Microphone enable/disable.
- Microphone sensitivity.
- Microphone release time.
- Talking bounce enable/amount.
- Idle bob settings.
- Idle sway settings.
- Save and Start.

The launcher should remain optional and independently downloadable. The normal V_mini_me package should continue to work without raylib, raygui, a GUI toolkit, or an installer.

## Features intentionally out of scope

Unless the project's goals change substantially, avoid adding:

- Webcam face tracking.
- Computer-vision tracking.
- Live2D support.
- Skeletal animation.
- General physics systems.
- Full animation timelines.
- Speech recognition.
- Embedded scripting languages.
- Plugin frameworks.
- Electron/web-based UI.
- Heavy GUI frameworks in the core runtime.
- A game engine dependency for the avatar runtime.

These features are better served by existing full VTuber applications and would weaken V_mini_me's main advantage.

## Release direction

A sensible path toward a stable 1.0 is:

1. Keep the existing directional and microphone behavior stable.
2. Add configurable idle bob/sway.
3. Test for regressions on Linux and Windows.
4. Benchmark actual CPU/RAM/GPU use.
5. Improve documentation based on those measurements.
6. Consider optional blinking if it remains trivial and reliable.
7. Declare 1.0 once the core behavior, configuration, and cross-platform builds are dependable.

The optional GUI launcher and multi-PNG animation do not need to block 1.0.

## Decision rule for future features

Before adding a feature, ask:

1. Does it noticeably improve a simple PNG avatar?
2. Can it be implemented without a large dependency or subsystem?
3. Does it preserve very low runtime resource use?
4. Can users who do not want it leave it completely disabled?
5. Is it simpler than asking users to run a substantially heavier VTuber application instead?

If the answer to most of these is no, it probably does not belong in V_mini_me.
