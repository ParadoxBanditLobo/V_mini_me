# Optional Linux GUI launcher

This directory contains the optional raylib-based launcher for V_mini_me.
It is deliberately separate from the core avatar runtime.

## Intended distribution

The compiled file is named:

```text
V_mini_me_launcher
```

Place it beside the normal Linux release:

```text
V_mini_me/
├── V_mini_me
├── V_mini_me_launcher
├── config.ini
└── avatar/
```

The launcher resolves `V_mini_me` and `config.ini` relative to its own executable location, so it can be copied directly into an existing unzipped release folder.

## Current functional scope

The first Linux prototype intentionally prioritizes function over appearance. It can:

- Read the existing `config.ini` while preserving unknown/advanced lines.
- Edit the avatar folder.
- Edit scale and direction mode.
- Toggle microphone support and adjust threshold/release time.
- Toggle talking bounce and adjust bounce pixels.
- Toggle idle bob/sway and adjust their amount/period.
- Configure the existing eight Quick Expression folder slots.
- Save changes to `config.ini`.
- Start V_mini_me while keeping the launcher open as a live control panel.
- Send Quick Expression selections to the running core through its existing terminal command interface.
- Send runtime reload and quit commands.
- Launch V_mini_me and immediately close the launcher for set-and-forget use.

When the launcher starts the avatar, it writes `.vmini_launcher_runtime.ini` with `show_setup_on_start=false`. This prevents the terminal setup menu from blocking a GUI launch while preserving the user's normal `show_setup_on_start` preference in `config.ini`.

Closing the launcher does not terminate an already-running avatar. The input pipe simply closes and V_mini_me continues normally.

## Build

The current build target is Linux x86-64 with raylib 6.0.

```bash
./build_linux.sh /path/to/raylib-6.0_linux_amd64
```

The launcher links raylib statically, so the distributed launcher does not require a separately installed raylib shared library.

## Design boundary

Do not move raylib into the V_mini_me core runtime. The launcher must remain optional.
