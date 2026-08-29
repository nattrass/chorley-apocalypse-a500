# Chorley Apocalypse — Delivery Plan

How [DESIGN.md](DESIGN.md) gets built, in increments that each end in something you can run.

## The rule that makes this incremental

**Every milestone ends with a binary you can F5 into WinUAE and look at.** No milestone is
"refactor the entity system" or "design the map format" — each one adds something observable, and
if it can't be described as a thing you can see on screen, it isn't a milestone, it's part of one.

**Worlds are releases.** Once the engine carries one complete level, each world is an independent
increment that ships on its own. You are never holding sixteen half-finished levels waiting for a
big-bang launch: v0.1 is a real, complete, playable thing, and every release after it is v0.1 plus
more.

**Risk goes first.** The two things that can invalidate the design are the scroll engine holding
50Hz and bobs drawing over a scrolling playfield. Both are proven in M1 and M2, before any content
is built on top of them.

---

## Phase 1 — Engine (M0-M5)

No game content. Placeholder tiles and one white square for a player. The goal is a machine that
can run a level, proven at 50Hz, before a single pixel of Chorley is drawn.

### M0 — Clear the decks
Break the template demo into modules and leave a game loop behind.

- Split `main.cpp`: `system.cpp` (takeover/restore), `copper.cpp` (list building helpers),
  `music.cpp` (P61 glue), `main.cpp` (init → loop → exit).
- Drop the demo's bob and scroll code; keep the idioms.
- **Fix the `WaitBlit()` calls in the main loop.** They currently `jsr` into graphics.library with
  the OS disabled — replace with the local OS-free `WaitBlt()`. Harmless in a demo, not in a game
  loop that runs for an hour.
- Add a frame-time counter so profiling has something to read from M1 onward.

**Done when:** builds, runs, black screen, music plays, LMB exits cleanly, and no AmigaOS call
happens between `TakeSystem()` and `FreeSystem()`.

**Current status:** the M0 split is largely complete: `main.cpp` now coordinates init/loop/exit,
`system.cpp` owns takeover/restore and interrupt plumbing, `music.cpp` contains the P61 glue, and
`copper.h` holds the copper helpers. The binary builds, boots to a black screen, and exits cleanly
with LMB. The remaining blocker is audio: the module loads and the player init succeeds, but the
sound output is still not heard in WinUAE, so this milestone is not yet considered complete.

### M1 — Scrolling tilemap ← *the big risk*
The single most important milestone in the project.

- Procedural placeholder tile sheet, ~32 tiles, generated at startup inside `warpmode()`. Number
  them visibly so wrap and alignment bugs are obvious on screen.
- Procedural 128x128 map.
- Double-wide playfield buffer, copper list with bitplane pointers and the HUD split.
- 4-way scroll from joystick, edge tile blitting, vertical wrap via copper.

**Done when:** you can scroll the whole 2048x2048 map smoothly, no tearing, no glitch at any wrap
boundary, holding 50Hz.

> **GATE — profile before going further.** Measure with the extension's profiler. You need
> **≥40% of the frame still free** with nothing but scrolling on screen; bobs will eat the rest.
> If it fails, the fallbacks in order: narrow the buffer and accept a hitch at wrap; drop the view
> to 4 bitplanes (16 colours); shrink the visible play area. Take that decision here, not in M6
> when there is art riding on it.

### M2 — Player
- Bob draw and restore over a *scrolling* background — the second hard problem. Restore-behind has
  to survive the playfield moving under it.
- 8-direction movement, 8-direction independent fire (Chaos Engine's feel: move on the stick, fire
  direction latched while the button is held).
- Camera follows with a dead zone so small movements don't shake the screen.
- Bullets as 16x16 bobs from a fixed pool.

**Done when:** you can run around the map and shoot in eight directions at 50Hz.

> **GATE — profile again with ~24 bobs live.** This is the real budget test. If it fails here, the
> lever is bob count and bob size, and DESIGN.md's enemy counts come down to match.

### M3 — The map becomes real
- Attribute layer: `ATTR_SOLID`, `ATTR_WATER`, `ATTR_HAZARD`, `ATTR_TRIGGER`.
- Player-vs-map and bullet-vs-map collision.

**Done when:** walls stop you, walls stop bullets, water reads as hazard.

### M4 — Enemies
- Enemy pool, spawn from map data, state machine: idle → chase → shoot → die.
- Damage in both directions, health, death animation.

**Done when:** **the first real gameplay loop exists.** You can walk into a room, get shot at,
shoot back, and clear it.

### M5 — Level structure and HUD
- Nodes, switches, gates, level exit.
- Brass and Chorley cake pickups.
- HUD panel in the copper-split region: health, Brass, class.

**Done when:** you can start a level, find the nodes, open the way, and reach the exit.

---

## Phase 2 — First release (M6)

### M6 — v0.1 "The Flat Iron"
The first build you can hand to somebody.

- World 1 tile set: market square, Market Street, St Laurence's, the arches.
- Four maps.
- Scrappers and Cogsmen.
- The Bellfounder boss.
- One playable class (the Mill-Hand — the all-rounder, so it reads as neutral).
- Title screen, World 1 music.

**Done when:** a stranger can load it and play four levels through to a boss.

> **GATE — is it fun?** This is the real question and this is the first moment you can honestly ask
> it. Everything after M6 multiplies whatever you have here by four worlds and six classes. If the
> core loop is flat, fix it now, while there is one tile set to redraw and not four.

---

## Phase 3 — Systems (M7-M8)

### M7 — Character building
- Six classes with the DESIGN.md stat lines.
- Astley Hall between-levels screen.
- Brass economy and per-class upgrade costs.
- Save/continue — password is cheaper than disk save and fits the era.

### M8 — Two players
- Second joystick, shared camera with clamping so neither player can be pushed off screen.
- The Curate's revive.
- CPU-controlled player 2 *(deferrable — see scope levers)*.

**→ v0.2 = v0.1 + classes + upgrades + co-op.**

---

## Phase 4 — Content (M9-M11)

Each world is the same shape of work and each one ships: tile set, four maps, ~6 enemies, a boss,
music. The engine does not change.

| Milestone | Release | World |
| --- | --- | --- |
| M9 | v0.3 | Botany Bay — the mill and the canal |
| M10 | v0.4 | Camelot — the abandoned park |
| M11 | v0.5 | Winter Hill — the climb and the mast |

Do Botany Bay first. It is the strongest world in the design and the most different from the Flat
Iron, so it is the one that proves the game has range.

---

## Phase 5 — Ship (M12)

### M12 — v1.0
- Intro, outro, credits.
- Disk build and loading. The doynax depacker is already in `support/`.
- **Testing on real hardware.** WinUAE is not the target; a real A500 is. Budget real time for this
  — timing, disk loading and memory behaviour all differ.

---

## The art and audio track

Runs in parallel from the moment M1 fixes the tile format. It does not block engine work, and
engine work does not block it.

| Asset | Volume | Notes |
| --- | --- | --- |
| Tile sets | 4 worlds x ~200 tiles | `gfx/convert.cmd` and KingCon already do this |
| Character bobs | 6 classes x 16 frames | 8 directions x 2 frames. **The largest single art job in the project** |
| Enemy bobs | ~24 types | Per-world sets |
| Bosses | 4 | Large, bespoke |
| Music | 5-6 P61 modules | One per world, plus title |

Character art is the volume risk. 96 character frames is a lot of pixel art before anything else
gets drawn, which is exactly why the class count is the first scope lever below.

---

## Scope levers

Pull these in order when time or RAM runs short. Listed worst-consequence-last on purpose.

1. **Six classes → three.** Halves the largest art job and keeps the mechanic intact.
2. **Four worlds → two.** Ship the Flat Iron and Botany Bay. They are the strongest pair and the
   game still has a beginning, a middle and an end.
3. **Four levels per world → three.**
4. **Drop CPU player 2.** Human co-op only.
5. **32 colours → 16.** Frees blitter time and chip RAM. Genuine last resort — it costs the look,
   and the look is most of what makes this read as Chaos Engine.

Note what is *not* on the list: the A500 target. That was fixed deliberately, and relaxing it to an
A1200 is not a scope lever, it is a different project.

---

## Standing rules

- Every milestone ends runnable under WinUAE, on the **A500** launch config — not AROS.
- Profile at M1, M2, M4, and once per world after that. Frame time only goes one way if unwatched.
- The `static_assert`s in `game/gamedefs.h` stay green. Growing a subsystem means raising its
  `BUDGET_*` define so the total is re-checked — never deleting the assert.
- Update [DESIGN.md](DESIGN.md) when a design decision changes. A stale bible is worse than none.

## Current position

M0 is in progress and functionally close: the demo has been split into modules, the black-screen
launch loop works, and the clean system takeover/restore path is in place. The open issue is sound:
the P61 init succeeds but the music still does not emit in WinUAE, so this milestone is blocked on
audio debugging rather than structure. **Next action: finish the M0 sound fix and re-validate the
milestone exit criteria.**
