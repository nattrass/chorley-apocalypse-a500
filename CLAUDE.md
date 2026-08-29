# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A bare-metal Amiga (68000) demo in C++, built on the template from Bartman/Abyss'
[amiga-debug](https://github.com/BartmanAbyss/vscode-amiga-debug) VSCode extension. There is no OS
runtime once the demo starts: it takes the machine over from AmigaOS, drives the custom chips
directly, and hands control back on exit. `main.cpp` is the entire demo.

## Build

The toolchain ships inside the VSCode extension and must be on `PATH`. On this machine:

```sh
AMIGABIN="/c/Users/PaulNattrass/.vscode/extensions/bartmanabyss.amiga-debug-1.8.2/bin/win32"
export PATH="$PATH:$AMIGABIN:$AMIGABIN/opt/bin"
"$AMIGABIN/gnumake.exe" -j4      # -> out/a.exe, out/a.elf, out/a.map, out/a.s
"$AMIGABIN/gnumake.exe" clean    # empties obj/ and out/
```

The version in that path changes when the extension updates — glob `bartmanabyss.amiga-debug-*`
rather than hardcoding it. Inside VSCode, F5 (or the `compile` task) runs the same make with
`-j4 program=${config:amiga.program}`; `.vscode/launch.json` has AROS / A500 / A1200 / A4000
targets. AROS needs no Kickstart ROM; the others need `amiga.rom-paths` set. Everything compiles
`-m68000`, so **A500 is the configuration that matters** — no 68020+ instructions, no AGA registers.

There are no tests and no linter. Verification is running the demo under WinUAE via F5.

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

### Lifecycle (`main`)

`OpenLibrary(graphics/dos)` → warpmode-wrapped precalc → `TakeSystem()` → build copper list →
install VBL handler → main loop until left mouse button → `FreeSystem()` → `CloseLibrary`.

`TakeSystem()`/`FreeSystem()` are a matched pair and must stay symmetric: they save and restore
`adkcon`, `intena`, `dmacon`, the active `View`, the system copper lists (`GfxBase->copinit` /
`LOFlist`) and the level-3 autovector at `VBR+0x6c`. Anything that leaks past `FreeSystem()` hangs
or corrupts the host OS on exit. Between them, **no AmigaOS calls except `AllocMem`/`FreeMem`** —
interrupts are disabled and the OS is forbidden.

`warpmode(1)`/`warpmode(0)` bracket slow precalc so WinUAE fast-forwards it instead of the profiler
attributing that time to the demo. Wrap any new table generation the same way.

### Display

Two copper lists run per frame:

- `copper1` — built at runtime into chip mem (`AllocMem(1024, MEMF_CHIP)`). `screenScanDefault()`
  sets up 320x256 lowres (`ddfstrt`/`ddfstop`/`diwstrt`/`diwstop`), then `bplcon0` (5 planes),
  `bplcon1` (scroll), `bplcon2`, both bitplane modulos (`4*lineSize`, the interleaved stride), the
  5 bitplane pointers and all 32 colors, then jumps to `copper2` via `copjmp2`.
- `copper2` — a static `.MEMF_CHIP`-sectioned array of raw copper words: the grey gradient bars on
  lines `0x41`–`0x4f`.

`copSetPlanes` / `copWaitXY` / `copWaitY` / `copSetColor` are all `always_inline` and take/return a
moving `USHORT* copListEnd` cursor — that is the idiom for appending to a copper list; keep it.

`scroll` is a raw pointer into `copper1` at the `bplcon1` data word. The VBL handler rewrites it each
frame from `sinus15[]`, hardware-scrolling the playfield without touching the bitmap. Poking a copper
list word from the interrupt is the pattern used throughout.

### Memory / assets

`INCBIN(name, file)` puts data in `.rodata`; `INCBIN_CHIP(name, file)` puts it in
`.INCBIN.MEMF_CHIP` so it lands in chip RAM and is usable by the blitter/copper/audio with no copy.
Anything a custom chip touches (bitplanes, copper lists, sample data, the P61 module) **must** be
chip mem; the P61 player binary itself is CPU-only and is plain `INCBIN`.

Asset layouts (all interleaved, produced by `gfx/convert.cmd` via the bundled `KingCon.exe`):

| File | Layout |
| --- | --- |
| `image.bpl` | 320x256, 5 planes interleaved — 40 bytes per plane-row, 200 bytes per screen line, 51200 total |
| `image.pal` | 32 raw `UWORD` OCS colors |
| `bob.bpl` | 6 frames of 32x16, 5 planes, each plane row followed by a copy of the mask row → 8 bytes per blit row, 640 bytes per frame |

Binary assets are committed; only re-run `gfx/convert.cmd` (from inside `gfx/`) after editing the PNGs.

### Rendering loop

Single-buffered, drawing straight into the displayed `image` bitmap. Lines 200–255 are the bob band:
each frame the loop waits for line `0x10`, clears the band with an `A_TO_D` blit (`(56*5)` rows),
then cookie-cuts 16 bobs into it with minterm `0xca` (A=source, B=mask, C=background, D=dest) plus
`bltcon0`/`bltcon1` shifts for sub-word X. Positions come from the `sinus32`/`sinus40` byte tables
indexed by `frameCounter`.

Every blitter register write must be preceded by `WaitBlit()`. Note that `custom->dmacon =
DMAF_BLITTER` deliberately clears blitter DMA before `copjmp1` to dodge the copper-jump hardware
bug, then re-enables it.

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

- `.vscode/settings.json` `m68k.includePaths` is an absolute path pinned to the installed extension
  version (currently `bartmanabyss.amiga-debug-1.8.2`). It affects VSCode IntelliSense only, not the
  build, but it goes stale whenever the extension updates — repoint it if IntelliSense starts
  reporting bogus errors.
- The `#ifdef __INTELLISENSE__` guards in `support/gcc8_c_support.h` exist because IntelliSense
  doesn't understand 68000 register constraints. Don't "clean them up".
- `support/` is template code from the extension; prefer changing `main.cpp` over editing it, since
  it gets replaced wholesale when the template is updated.
