# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

**Chorley Apocalypse** — a top-down run-and-gun with character building for the Amiga 500, in
the style of The Chaos Engine (Bitmap Brothers, 1993), set in a post-apocalyptic Chorley,
Lancashire. Steampunk foundations, cyberpunk intrusion, real Chorley landmarks as level design.

[DESIGN.md](DESIGN.md) is the design bible and the authority on game content — the four worlds, six
classes, the Brass economy, enemies and art direction. Read it before making a design decision;
update it when one changes.

[PLAN.md](PLAN.md) is the delivery plan and the authority on build order — what gets built in which
milestone, what "done" means for each, and the profiling gates. Its "Current position" section at
the bottom says what to work on next; keep it accurate as milestones land.

**Status: pre-alpha — the engine exists, the game does not.** M0-M2 are done: the system takeover
is modularised, a 50Hz four-way scrolling tilemap runs over a procedural 128x128 map, and a player
with eight-way movement, eight-way latched fire and a bullet pool draws and restores over it. Every
pixel on screen is placeholder art generated at startup — numbered tiles, a stick-figure Mill-Hand,
no Chorley yet. `image.bpl` / `bob.bpl` are leftover art from the
[amiga-debug](https://github.com/BartmanAbyss/vscode-amiga-debug) VSCode extension template this
repo started life as, still committed but unreferenced; `testmod.p61` is that template music and is
still playing.

There is no OS runtime once it starts: it takes the machine over from AmigaOS, drives the custom
chips directly, and hands control back on exit.

## Target and the chip budget

A500, 1MB, OCS. 68000 at 7MHz, 320x256 lowres, 5 bitplanes, 32 colours, 50Hz. Chosen because it is
the specification Chaos Engine itself shipped on, so the target is known to be achievable — treat it
as fixed rather than as something to relax when a subsystem turns out to be tight.

A 1MB A500 is **512KB chip RAM plus 512KB trapdoor slow RAM, and slow RAM is not DMA-visible**.
Every bitplane, bob, tile, copper list and audio sample has to fit the 512KB chip half; code, tile
maps and precalc tables go in slow RAM. This is far tighter than "1MB" suggests and it drives every
asset and engine decision in the project.

`game/gamedefs.h` encodes that budget as `static_assert`s — display and playfield geometry, tile and
bob layouts, and per-subsystem `BUDGET_*` reserves that sum to a compile-time total. Currently
362,720 of 524,288 bytes, with ~158KB held back for per-world tile sets and enemies. It is included
from `main.cpp` so it always compiles. **When a subsystem needs to grow, raise its `BUDGET_*` define
and let the assert re-check the total — do not delete the assert.**

## Build

The toolchain ships inside the VSCode extension and must be on `PATH`. On this machine:

```sh
AMIGABIN=$(echo "$HOME"/.vscode/extensions/bartmanabyss.amiga-debug-*/bin/win32)
export PATH="$PATH:$AMIGABIN:$AMIGABIN/opt/bin"
"$AMIGABIN/gnumake.exe" -j4      # -> out/a.exe, out/a.elf, out/a.map, out/a.s
"$AMIGABIN/gnumake.exe" clean    # empties obj/ and out/
```

The glob is deliberate: the version changes whenever the extension updates, and the path is
machine-specific, so never hardcode it into a tracked file. Inside VSCode, F5 (or the `compile`
task) runs the same make with
`-j4 program=${config:amiga.program}`; `.vscode/launch.json` has AROS / A500 / A1200 / A4000
targets. AROS needs no Kickstart ROM; the others need `amiga.rom-paths` set. Everything compiles
`-m68000`, so **A500 is the configuration that matters** — no 68020+ instructions, no AGA registers.

There are no tests and no linter. Verification is running it under WinUAE via F5 — the A500
configuration, since that is the shipping target and not merely a compatibility check.

### Build system notes

- The Makefile globs `*.cpp`/`*.c` in the root and every immediate subdirectory, so new C/C++ files
  are picked up automatically. `.asm` files (assembled with `vasmm68k_mot`) are globbed too.
- **`.s` files are NOT globbed** — `s_sources` is a hardcoded list. Adding a gas-syntax `.s` file
  requires editing that line in the Makefile.
- Link is `-flto -fwhole-program -nostdlib -Ttext=0`. There is no libc: `memset`/`memcpy`/`memmove`/
  `strlen`/`memclr` come from `support/gcc8_c_support.c`, and `-fno-exceptions -fno-rtti` means no
  C++ exceptions, RTTI, or `new`/`delete`.
- `out/a.s` is a full annotated disassembly of the linked binary — the fastest way to check what the
  compiler did to a hot loop.
- `obj/DELETE.ME` and `out/DELETE.ME` are tracked placeholders keeping those directories in git.
  `make clean` deletes them; restore with `git checkout obj out`.


## Architecture

The engine as it stands after M2: a scrolling tilemap with a player, bullets and bob restore over
it. The template demo it grew out of is gone from `main.cpp`, but its idioms — the system takeover,
copper list building, the blitter register sequence, the interrupt install — carry straight
through, so follow them rather than replacing them.

| File | Holds |
| --- | --- |
| `main.cpp` | Allocation, init, the frame loop, exit |
| `system.cpp` / `.h` | `TakeSystem`/`FreeSystem`, the VBR interrupt install, `WaitVbl`/`WaitLine`/`WaitBlt` |
| `copper.h` | `copSetPlanes`/`copWaitY`/`copSetColor`/`screenScanDefault`, all `always_inline` |
| `display.cpp` / `.h` | `blitTile`, the playfield window, `buildCopperList`, the HUD |
| `tiles.cpp` / `.h` | The 32-colour palette, the procedural tile sheet, `drawTextPlanar` |
| `map.cpp` / `.h` | The procedural 128x128 Chorley map and `getMapTile` |
| `bob.cpp` / `.h` | `bobDraw`/`bobRestore` and the procedural player and bullet bobs |
| `player.cpp` / `.h` | Player state, eight-way move and fire, the bullet pool, the camera |
| `keyboard.cpp` / `.h` | Raw CIA-A keyboard polling |
| `music.cpp` / `.h` | The P61 glue |

### Lifecycle (`main`)

`OpenLibrary(graphics/dos)` → allocate → warpmode-wrapped procedural generation → `TakeSystem()` →
fill the playfield → build the copper lists → install the VBL handler → frame loop until LMB or Esc
→ `FreeMem` → `FreeSystem()` → `CloseLibrary`.

`TakeSystem()`/`FreeSystem()` are a matched pair and must stay symmetric: they save and restore
`adkcon`, `intena`, `dmacon`, the active `View`, the system copper lists (`GfxBase->copinit` /
`LOFlist`) and the level-3 autovector at `VBR+0x6c`. Anything that leaks past `FreeSystem()` hangs
or corrupts the host OS on exit. Between them, **no AmigaOS calls except `AllocMem`/`FreeMem`** —
interrupts are disabled and the OS is forbidden.

`warpmode(1)`/`warpmode(0)` bracket the procedural tile, map and bob generation so WinUAE
fast-forwards it instead of the profiler attributing that time to the game. Wrap any new table
generation the same way.

### The playfield buffer

One buffer, `PLAYFIELD_W x PLAYFIELD_H` (768x272) by 5 interleaved planes, holding a window of the
map. Two rules define it and everything else follows:

- **Map column `c` always lives in buffer slot `c % BUF_COLS`, map row `r` in slot `r % BUF_ROWS`.**
  So world pixel x is at buffer pixel `x % PLAYFIELD_HALF_W` and world pixel y at
  `y % PLAYFIELD_H`, and scrolling is nothing but moving the bitplane pointers.
- **Every tile is blitted twice, `BUF_COLS` apart.** The right half duplicates the left, so the
  320-pixel display window never straddles the end of a buffer row and horizontal scrolling never
  hitches. Vertical wrap is not duplicated — the copper jumps the bitplane pointers back to the top
  of the buffer at the wrap line instead.

The loaded window is anchored `BUF_ANCHOR` tiles *before* the camera tile rather than on it. That
one tile of lead is what lets a bob straddling the left screen edge be clipped losslessly; without
it the first source word of the blit would have to be dropped, taking the pixels it shifts into the
second word with it, and up to 15 columns of bob would disappear at the edge.

`playfieldReadX()` in `display.h` is the buffer x the copper starts reading at, and `bobDraw` takes
its anchor from the same function. **They must agree** — the two halves hold identical background,
but a bob drawn into the half the display is not reading is simply invisible.

`initPlayfield` fills the window, `updateTileSeams` blits the columns and rows a camera move
uncovered, and `buildCopperList` turns a camera position into bitplane pointers plus the `bplcon1`
sub-word scroll. The copper lists are double buffered: the one built this frame is latched by
`cop1lc` and takes effect at the next vertical blank.

### Bobs

`BOB_FRAME_BYTES` of interleaved planes with an interleaved mask: per pixel line, per plane,
`BOB_WORDS` data words followed by `BOB_WORDS` mask words. The frame is stored one word wider than
the bob draws, because the blitter shift that reaches a sub-word X has to put the pixels it shifts
out of the last word somewhere.

`bobDraw` is a single cookie-cut blit — `bltcon0 = (shift << 12) | 0x0fca` with A = mask, B =
source, C and D = the playfield — clipped to the loaded window in whole words horizontally and
exact rows vertically, and split into two blits when it crosses the buffer's vertical wrap.

`bobRestore` puts the background back by **re-blitting the map tiles the bob covered**, not by
restoring saved pixels. Because the buffer is indexed by world position, the tiles are the backup:
it costs no scratch chip RAM, it survives the seam blitter having rewritten the area, and
overlapping bobs need no ordering. `bobDraw` records the tile rect and the buffer half it used in a
`BobRect` so the restore is independent of where the camera has moved to since.

### Frame order

The order in the loop is load-bearing and the reason bobs survive a scrolling background:

1. `WaitLine(0x10)` — sync above the display window
2. `entitiesRestore` — the playfield is pure background again
3. read input, `entitiesUpdate`, `cameraFollow`
4. `updateTileSeams` for whatever the camera uncovered
5. `entitiesDraw` — draw and record
6. `buildCopperList` and hand it to `cop1lc`

Restore must come before the seams, or a restore would put stale tiles back over a fresh seam. The
playfield is single-buffered, so from step 2 onward the blitter is racing the raster down the
screen; the HUD shows the raster line reached at the end of the frame, which is the number to watch.

### Memory / assets

`INCBIN(name, file)` puts data in `.rodata`; `INCBIN_CHIP(name, file)` puts it in
`.INCBIN.MEMF_CHIP` so it lands in chip RAM and is usable by the blitter/copper/audio with no copy.
Anything a custom chip touches (bitplanes, copper lists, sample data, the P61 module) **must** be
chip mem; the P61 player binary itself is CPU-only and is plain `INCBIN`.

Everything the game currently draws is generated procedurally at startup into `AllocMem(...,
MEMF_CHIP)` buffers, so the only remaining `INCBIN` asset is `testmod.p61`. `image.bpl`,
`image.pal` and `bob.bpl` are leftover template art, still committed but no longer referenced;
`gfx/convert.cmd` and the bundled `KingCon.exe` are how real art will be converted, interleaved,
when it arrives.

### Rendering

Single-buffered: bobs are drawn straight into the displayed playfield. Tiles go in with an
`A_TO_D` blit (`bltcon0 = 0x09f0`), bobs with the cookie-cut minterm `0xca` described above. Every
blitter register write must be preceded by `WaitBlt()` — the local OS-free one in `system.h`, never
graphics.library's.

The HUD occupies lines 252-255 and below via a copper split down to `HUD_BITPLANES` planes, drawn
into its own buffer by `initHUD` and updated each frame by `hudSetCounters`.

### Interrupt

`interruptHandler()` is `__attribute__((interrupt))` and installed directly at `VBR+0x6c` via
`SetInterruptHandler` — it is not an AmigaOS `Interrupt` struct. It clears `intreq` twice (A4000
erratum), updates the scroll word, calls `p61Music()`, and bumps `frameCounter`. Keep it short; it
runs with the OS disabled.

### Music

ThePlayer 6.1a is a raw binary blob with a three-entry jump table at offsets 0/4/8
(`init`/`music`/`end`), called through inline asm in `p61Init`/`p61Music`/`p61End`. Those wrappers
push and pop every register the player clobbers because GCC cannot see across the `jsr` — copy the
pattern exactly for any new blob (`doynaxdepack` uses the same idiom). The player needs `INTF_EXTER`
enabled in addition to `INTF_VERTB`. `#define MUSIC` at the top of `main.cpp` toggles the subsystem.

### Debugger integration

`debug_register_bitmap`/`_palette`/`_copperlist` make resources browsable in the extension's
graphics debugger — register any new bitmap or copper list you add. `debug_clear`/`debug_rect`/
`debug_filled_rect`/`debug_text` draw a WinUAE overlay in PAL coordinates (0,0)-(768,576), so
multiply screen Y by 2 — available, though nothing in the demo currently draws an overlay.
`debug_start_idle()`/`debug_stop_idle()` (used inside `WaitVbl`) tell the profiler that time is
spin-waiting, not work. All of these are inert on real hardware.

## Gotchas

- **`.vscode/settings.json` is deliberately untracked** (see `.gitignore`). The amiga-debug
  extension's `configureM68k()` rewrites `m68k.includePaths` and `m68k.vasm.provideDiagnostics`
  into workspace settings on activation, using an absolute path containing the local username and
  the installed extension version. That makes the file machine-specific and not shareable, so it
  is gitignored and `.vscode/settings.json.example` records the project-level settings instead.
  Never re-add it to git, and never copy a resolved toolchain path into a tracked file.
- The `#ifdef __INTELLISENSE__` guards in `support/gcc8_c_support.h` exist because IntelliSense
  doesn't understand 68000 register constraints. Don't "clean them up".
- `support/` is template code from the extension; prefer changing `main.cpp` over editing it, since
  it gets replaced wholesale when the template is updated.
