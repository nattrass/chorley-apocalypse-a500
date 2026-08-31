# Chorley Apocalypse — Design Bible

A top-down run-and-gun with character building, in the style of The Chaos Engine
(Bitmap Brothers, 1993), set in a post-apocalyptic Chorley, Lancashire.

Steampunk foundations, cyberpunk intrusion. Victorian industry that never stopped running,
and a signal coming off Winter Hill that nobody can switch off.

---

## 1. Premise

In 1887 a Chorley mill owner sank an engine beneath Botany Bay — a machine to drive every
loom on the Leeds & Liverpool canal from a single boiler. It worked. It has not stopped since.

The transmitter on Winter Hill was raised over its exhaust column. What went up the mast was
never only television. Everything inside the broadcast footprint — the town, the moor, the mill,
the dead theme park at Charnock Richard — has been running the Engine's instructions for
longer than anyone can account for. Machines that should be scrap are walking. People who go
up the hill come back with aerials.

You are two mercenaries paid in salvage to walk from the Flat Iron to the mast and shut it down.

The structure is Chaos Engine's: four worlds, four levels each, two players, six classes,
upgrades bought between levels. Reach the top of the hill, kill what is broadcasting.

## 2. What we are cloning, precisely

From The Chaos Engine, deliberately:

- **Two-player co-operative**, both on screen at once, camera tracking the midpoint. Player 2
  can be CPU-controlled. This is the signature of the game; design for it from day one even if
  second-player input lands later.
- **Six mercenary classes**, two picked per run. Each has a movement speed, a stamina pool,
  a special ability and a weapon upgrade ceiling.
- **Four stat tracks** bought between levels: Speed, Stamina, Special, Weapon.
- **Node/gate level structure.** Levels are not corridors. You find switches that open the way
  forward, backtrack through cleared ground, and the exit only lights once the level's nodes
  are triggered.
- **Muted, heavy art.** Chaos Engine is not colourful. It is brown, grey, olive and rust with
  very small amounts of extremely bright signal colour. The restraint is the whole look.

Deliberately *not* cloned: the fantasy-Victorian bestiary, and arbitrary map layouts. Chorley's
real geography is the level design.

## 3. Target and budget

**A500, 1MB, OCS. 68000 at 7MHz. 320x256 lowres, 5 bitplanes, 32 colours, 50Hz.**

The critical constraint: a 1MB A500 is 512KB chip + 512KB trapdoor slow RAM, and **slow RAM is
not DMA-accessible**. Bitplanes, bobs, tiles, copper lists, sample data and the module all have
to live in the 512KB chip half. Code, tile maps, enemy tables and precalc go in slow RAM.

### Chip RAM budget (512KB)

| Item | Bytes | Notes |
| --- | ---: | --- |
| Playfield, double-wide | 130,560 | 768x272x5 interleaved, 96 bytes/plane-row. Each incoming tile column is blitted twice so the display window never wraps mid-line. Three tiles of horizontal margin, one of them ahead of the camera so bobs at the left edge have somewhere to be clipped against |
| Tile sheet, 256 tiles | 40,960 | 16x16x5 interleaved, 160 bytes/tile |
| Player bobs, 2 classes loaded | 61,440 | 32x32, 8 directions x 2 frames, 1,920 bytes/frame with mask and the shift guard word |
| Enemy bobs | ~40,000 | Per-world set, loaded on level entry |
| Bullets, explosions, pickups | ~10,000 | 16x16 |
| HUD panel | 5,760 | 320x48x3 via copper split — fewer bitplanes below the play area |
| Music module | ~60,000 | P61, one per world |
| Copper lists | ~4,000 | Double buffered, plus the panel split |
| Blitter scratch / restore | ~10,000 | |
| **Total** | **362,720** | ~158KB headroom for level-specific assets |

The headroom is the point. Each Chaos Engine world has its own tile set and enemy set; a
per-world load into that ~158KB is what makes four visually distinct worlds affordable.
`game/gamedefs.h` holds these numbers as `static_assert`s, so the build fails rather than the
hardware if a subsystem outgrows its line.

**Open question, to settle at the M2 gate:** the figure above buys a *double-wide* playfield, not
a double-buffered one — the two cost the same and only one is affordable. Double-wide gives
hitch-free horizontal scrolling but leaves the playfield single-buffered, so bob drawing has to
chase the beam. If M2 shows that tearing is unacceptable, the alternatives are a narrow
double-buffered playfield with a scroll hitch at each wrap, or dropping to 4 bitplanes to afford
both. Do not commit to art volume until this is settled.

Two consequences of single buffering are settled, though. Restoring behind a bob is done by
re-blitting the map tiles it covered rather than by saving the pixels underneath: the buffer is
indexed by world position, so the tiles *are* the backup, which costs no scratch RAM, survives
the playfield scrolling between the draw and the restore, and does not care in what order
overlapping bobs are drawn. And bobs are stored one word wider than they draw, because the
blitter shift that reaches a sub-word X has to put the pixels it shifts out somewhere -- that is
the 1,920 rather than 1,280 bytes per frame in the table above.

### Slow RAM (512KB)

Code, the tile maps (128x128 tile index + 128x128 attribute layer = 32KB per level), enemy
spawn tables, class stat tables, sine/atan precalc, and save/continue state.

### Performance budget

At 50Hz on a 7MHz 68000 a frame is roughly 128,000 cycles, and blitter time is the real
currency. Per frame: restore and redraw ~24 bobs at 32x32x5 with mask, plus one tile column
or row when scrolling. That is close to the whole budget. Enemy AI, collision and the RPG
layer have to fit in what the CPU manages while the blitter works — which is why enemy counts
stay in the twenties and bullets are 16x16 rather than 32x32.

## 4. The four worlds

Real Chorley geography, walked outward and upward from the town centre. Each world is a tile
set, a palette shift, an enemy set and a boss.

### World 1 — The Flat Iron
Chorley town centre. The Flat Iron market square, Market Street, St Laurence's parish church,
the railway arches. Collapsed market stalls as cover, cobbles, shuttered shopfronts. The
teaching world: movement, shooting, nodes, the first gates.
**Boss:** the Bellfounder — St Laurence's bells, driven by the Engine, walking on a frame.

### World 2 — Botany Bay
The five-storey Victorian mill on the Leeds & Liverpool canal, and the canal itself. Locks and
towpaths outdoors, then interiors: loom floors, the boiler house, the drive shafts. Water is a
hazard, lock gates are switches, the mill's own machinery is the obstacle. This is the
steampunk core and should be the best-looking world in the game.
**Boss:** the Great Loom — a full-floor machine that fills the screen with shuttle fire.

### World 3 — Camelot
The abandoned theme park at Charnock Richard, shut since 2012, fibreglass castle rotting in a
field. Rusted coaster track, a drained log flume, the jousting arena, collapsed marquees. The
surreal world — brightest palette, most wrong. Funfair colours gone bad.
**Boss:** the Dragon — the coaster train itself, still running its circuit, still on the rails.

### World 4 — Winter Hill
The climb. Rivington Pike and the Terraced Gardens, up through bog and fog onto the moor, to
the transmitter mast. Sightlines cut to almost nothing by fog; the mast visible from every
level as a silhouette getting nearer. Cyberpunk finally surfaces here — the drones, the
aerial-grown things, the signal colours.
**Boss:** the Mast. Whatever has been broadcasting.

### Between levels — Astley Hall
The Jacobean hall in Astley Park is the safe house. Upgrade screen, class stats, Brass spent,
the map of how far you have got. The one warm, lit interior in the game.

### Landmarks to place as set pieces
Yarrow Valley, Healey Nab, Duxbury Park, Victory Park (Chorley FC), the hospital, the Coppull
mills, Frederick's, the railway station arches, the Big Lamp.

## 5. The six classes

Chaos Engine's archetypes recast as Lancashire industrial trades. Two are picked per run, and
only those two are loaded into chip RAM.

| Class | Speed | Stamina | Special | Weapon ceiling | Role |
| --- | :-: | :-: | --- | :-: | --- |
| **The Navvy** | 2 | 9 | Dynamite — heavy area damage | 6 | Canal digger. The tank. Slow, enormous stamina |
| **The Mill-Hand** | 4 | 7 | Steam vent — short-range cone, stuns | 7 | Loom worker. The all-rounder |
| **The Weaver** | 7 | 3 | Shuttle burst — very high rate of fire, brief | 8 | Fast, fragile, highest damage ceiling |
| **The Curate** | 4 | 6 | Ministry — heals and revives the other player | 5 | St Laurence's. Support, and the only class that can revive |
| **The Rambler** | 9 | 4 | Flare — reveals the map and marks nodes | 6 | Winter Hill trespasser. Fastest, the scout |
| **The Toff** | 5 | 5 | Hired gun — a temporary second shooter | 9 | Astley Hall gentry. Balanced, best ceiling, costliest upgrades |

Stats run 1-10 and behave as Chaos Engine's do: Speed is literal pixels-per-frame movement,
Stamina is both the health bar and how much a single hit takes off it.

The Curate's revive is what makes co-op matter mechanically. The Rambler's flare is what makes
the Winter Hill fog survivable. Class choice should change the run, not just the numbers.

## 6. Progression and economy

**Brass** is the currency — salvaged fittings, scrap, coin. Where there's muck there's brass.

- Brass drops from enemies and hides in the levels behind optional gates.
- Spent at Astley Hall between levels across the four tracks: Speed, Stamina, Special, Weapon.
- Costs escalate per point and scale per class — the Toff's weapon track reaches 9 but costs
  roughly double the Weaver's per point.
- **Chorley cakes** are the health pickup. Non-negotiable.
- Both players spend from a shared pot, which builds the co-op argument into the economy.

Weapon tiers, shared across classes and capped by the class ceiling:
single shot → twin → spread → piercing → homing → beam → overcharge.

## 7. Enemies

Per-world sets, roughly six types each plus the boss, loaded into the per-world budget.

- **Scrappers** — humans with jury-rigged guns. Fast, weak, everywhere. World 1.
- **Cogsmen** — clockwork automata that wind down and restart. Worlds 1-2.
- **Loom-Wights** — mill machinery walking on shuttle legs. World 2.
- **Boilers** — slow, huge, explode. Worlds 2-3.
- **Cast-offs** — fairground animatronics still running their routine. World 3.
- **Bog Hounds** — what the moor made. Fast, low, ambush out of standing water. World 4.
- **Mast Drones** — flying, ranged, the cyberpunk element. World 4.

## 8. Art direction

32 colours, and the discipline is spending most of them on mud.

| Range | Count | Use |
| --- | :-: | --- |
| Greys | 8 | Stone, cobble, fog, concrete, the mast |
| Brick and rust | 6 | Mill, terraces, corroded iron |
| Moor greens and olive | 6 | Bog, grass, canal water, Rivington |
| Brass and copper | 4 | Machinery, pickups, the Engine |
| Signal | 4 | Cyan and magenta. The broadcast. Used *sparingly* — these are the only saturated colours in the game and they should always mean something |
| UI and sprite | 4 | HUD, player highlights |

Per-world palette shifts happen inside that structure: Botany Bay pushes brick and brass,
Camelot pushes the signal range towards funfair reds, Winter Hill desaturates nearly everything
to grey and lets only the signal colours through the fog.

Copper gradients do the sky and the fog. The existing `copper2` bar code is the seed of this.

## 9. Engine milestones

Summarised here for design context. **[PLAN.md](PLAN.md) is the authority on build order**, with
per-milestone completion criteria, the profiling gates and the scope levers.

1. **Scrolling tilemap.** 4-way hardware scroll, 352x272 double-buffered playfield, 16x16 tiles,
   edge tile blitting, copper-split HUD panel. Nothing else. This is the technical risk.
2. **Player.** One character, 8-direction movement and independent 8-direction fire, bob draw
   and restore, camera tracking.
3. **Collision and map.** Tile attribute layer — solid, water, hazard, trigger. Bullets, walls.
4. **Enemies.** Spawn tables, a simple state machine, damage in both directions.
5. **Two players.** Second input, shared camera, the Curate's revive.
6. **The RPG layer.** Brass, the Astley Hall upgrade screen, class stats, save/continue.
7. **Content.** Tile sets, maps and enemies per world, boss by boss.

Milestone 1 decides whether this is possible on the target. Build it first and profile it on
the A500 configuration before committing to any of the rest.
