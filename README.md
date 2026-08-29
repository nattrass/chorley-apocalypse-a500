# amiga-demo

## What is this?
A bare-metal Amiga demo written in C++ for the 68000, built on Bartman/Abyss'
[amiga-debug](https://github.com/BartmanAbyss/vscode-amiga-debug) VSCode extension template.

It takes over the machine from AmigaOS (saving and restoring interrupts, DMA and the system
copper list), then runs a 320x256 5-bitplane lowres display driven by two copper lists:

* A copper-bar gradient and per-line colour changes.
* A hardware-scrolled playfield whose `bplcon1` value is rewritten every frame from a sine table
  in the vertical blank interrupt.
* 16 blitter "bobs" (cookie-cut with a mask) bounced around a sine path, cleared and re-blitted
  each frame.
* ProTracker music via [ThePlayer 6.1a](https://www.pouet.net/prod.php?which=19922)
  (`#define MUSIC` in [main.cpp](main.cpp) turns it off).
* WinUAE debugger resource registration (`debug_register_bitmap`/`_palette`/`_copperlist`) so
  bitmaps, palettes and copper lists show up in the extension's graphics debugger. The overlay
  helpers (`debug_rect`, `debug_text`, ...) are available but not currently drawn.

Left mouse button exits back to the OS.

## Setup
1. This project is tightly bound to VSCode as it's dependent on a VSCode extension. Sorry! Install VSCode
2. Install the 'Amiga C/C++ Compile, Debug & Profile' extension.
3. Press F5 to build and run.

The [launch configurations](.vscode/launch.json) cover AROS (no ROM needed), A500, A1200 and A4000.
The A500/A1200/A4000 targets need Kickstart ROM paths set in the extension's `amiga.rom-paths`
settings; AROS works out of the box. Everything targets a plain 68000, so the A500 config is the
one to trust.

## Building outside VSCode
`make` (via the extension's bundled `gnumake.exe`) compiles every `.c`/`.cpp`/`.s`/`.asm` in the
root and subdirectories into `obj/`, links with `-flto -fwhole-program -Ttext=0`, and converts the
ELF to an Amiga hunk executable with `elf2hunk`. Output lands in `out/`:

* `out/a.exe` - the runnable Amiga binary
* `out/a.elf`, `out/a.map`, `out/a.s` - ELF, link map and full disassembly for debugging

`make clean` empties `obj/` and `out/`. The toolchain (`m68k-amiga-elf-gcc`, `vasmm68k_mot`,
`elf2hunk`) ships with the extension and must be on `PATH` - the
[compile task](.vscode/tasks.json) does this for you.

## Layout
| Path | What |
| --- | --- |
| [main.cpp](main.cpp) | The whole demo: system takeover, copper lists, blitter, VBL interrupt, player glue |
| [Makefile](Makefile) | Build rules for C/C++/gas/vasm sources |
| [support/](support/) | Startup/runtime support from the extension template, plus the doynax depacker |
| [gfx/](gfx/) | Source PNGs, KingCon converter and [convert.cmd](gfx/convert.cmd) |
| `image.bpl` / `image.pal` | Background bitmap (interleaved, 5 bitplanes) and raw palette |
| `bob.bpl` | 32x96 masked sprite sheet, interleaved |
| `player610.6.no_cia.bin`, `testmod.p61` | ThePlayer 6.1a binary and the module it plays |
| `.vscode/` | Launch configs, build tasks and recommended extensions |

Binary assets are committed, so you only need to re-run `gfx/convert.cmd` after editing the PNGs.
It shells out to the bundled `KingCon.exe` and writes `image.*`/`bob.bpl` back into the project root.

## Credits
* Template, toolchain and debugger: Bartman/Abyss - `vscode-amiga-debug`
* ThePlayer 6.1A: Copyright (c) 1992-95 Jarno Paananen
* `testmod.p61`: module by Skylord/Sector 7
