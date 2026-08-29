# Chorley Apocalypse

A top-down run-and-gun with character building for the Amiga 500, in the style of
[The Chaos Engine](https://www.mobygames.com/game/1004/the-chaos-engine/) (Bitmap Brothers, 1993),
set in a post-apocalyptic Chorley, Lancashire. Steampunk foundations, cyberpunk intrusion.

An 1887 mill engine sunk beneath Botany Bay that never stopped running, a transmitter raised over
its exhaust column on Winter Hill, and everything inside the broadcast footprint still following
instructions. Two mercenaries walk from the Flat Iron to the mast to shut it down.

Four worlds — the town centre, the mill and the canal, the abandoned theme park at Charnock
Richard, and the climb over Rivington up to Winter Hill. Six classes, two picked per run, upgraded
with salvaged Brass between levels at Astley Hall.

**[DESIGN.md](DESIGN.md) is the design bible** — worlds, classes, economy, enemies and art
direction. **[PLAN.md](PLAN.md) is the delivery plan** — the milestone-by-milestone build order,
each one ending in something runnable, with the risk gates and scope levers.

## Status

Pre-alpha. **The game does not exist yet.** What builds and runs today is still the
[amiga-debug](https://github.com/BartmanAbyss/vscode-amiga-debug) template demo that this repo
started life as: a 320x256 5-bitplane display, copper bars, a sine-scrolled playfield, 16 masked
blitter bobs and ProTracker music. The images, sprite sheet and module in the project root are the
template's placeholder assets, not game art.

Done so far:

* [DESIGN.md](DESIGN.md) — the design bible.
* [game/gamedefs.h](game/gamedefs.h) — display and playfield geometry, map and bob layouts, and the
  chip RAM budget encoded as `static_assert`s so an oversized subsystem fails the build rather than
  failing on hardware. Currently 331,360 of 524,288 bytes committed, ~188KB reserved for per-world
  tile sets and enemies.

Next: **M0** — break the template demo into modules and leave a game loop behind. See
[PLAN.md](PLAN.md) for the full sequence.

## Target

**A500, 1MB, OCS. 68000 at 7MHz, 320x256 lowres, 5 bitplanes, 32 colours, 50Hz.**

Chosen because it is the specification The Chaos Engine itself shipped on, so the target is known
to be achievable. Everything compiles `-m68000` — no 68020+ instructions, no AGA registers.

The constraint that drives every design decision: a 1MB A500 is 512KB chip RAM plus 512KB trapdoor
*slow* RAM, and slow RAM is not DMA-accessible. Every bitplane, bob, tile, copper list and audio
sample has to fit in the 512KB chip half. Code, tile maps and precalc tables go in slow RAM.

## Setup
1. This project is tightly bound to VSCode as it's dependent on a VSCode extension. Sorry! Install VSCode
2. Install the 'Amiga C/C++ Compile, Debug & Profile' extension.
3. Press F5 to build and run.

The [launch configurations](.vscode/launch.json) cover AROS (no ROM needed), A500, A1200 and A4000.
The A500/A1200/A4000 targets need Kickstart ROM paths set in the extension's `amiga.rom-paths`
settings; AROS works out of the box. Everything targets a plain 68000, so the A500 config is the
one that matters — it is the shipping target, not just a compatibility check.

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
| [DESIGN.md](DESIGN.md) | The design bible - worlds, classes, economy, enemies, art direction |
| [PLAN.md](PLAN.md) | The delivery plan - milestones, risk gates, art track, scope levers |
| [game/](game/) | Game code. Currently [gamedefs.h](game/gamedefs.h) only: geometry and the chip RAM budget |
| [main.cpp](main.cpp) | Still the template demo: system takeover, copper lists, blitter, VBL interrupt, player glue |
| [Makefile](Makefile) | Build rules for C/C++/gas/vasm sources |
| [support/](support/) | Startup/runtime support from the extension template, plus the doynax depacker |
| [gfx/](gfx/) | Source PNGs, KingCon converter and [convert.cmd](gfx/convert.cmd) |
| `image.bpl` / `image.pal` | Placeholder background bitmap (interleaved, 5 bitplanes) and raw palette |
| `bob.bpl` | Placeholder 32x96 masked sprite sheet, interleaved |
| `player610.6.no_cia.bin`, `testmod.p61` | ThePlayer 6.1a binary and the module it plays |
| `.vscode/` | Launch configs, build tasks and recommended extensions |

Binary assets are committed, so you only need to re-run `gfx/convert.cmd` after editing the PNGs.
It shells out to the bundled `KingCon.exe` and writes `image.*`/`bob.bpl` back into the project root.

## Credits
* Design reference: The Chaos Engine, The Bitmap Brothers, 1993
* Template, toolchain and debugger: Bartman/Abyss - `vscode-amiga-debug`
* ThePlayer 6.1A: Copyright (c) 1992-95 Jarno Paananen
* `testmod.p61`: module by Skylord/Sector 7
