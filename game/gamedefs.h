// Chorley Apocalypse - core geometry and the chip RAM budget.
//
// Target is a 1MB A500: 512KB chip + 512KB trapdoor slow RAM. Slow RAM is NOT DMA-visible,
// so every bitplane, bob, tile, copper list and sample has to fit the 512KB chip half.
// The static_asserts at the bottom are the budget from DESIGN.md section 3, made executable
// so it fails the build rather than failing on hardware.

#pragma once

// --- Display -------------------------------------------------------------------------------

#define SCREEN_W        320
#define SCREEN_H        256
#define BITPLANES       5                       // 32 colours, OCS lowres

#define HUD_H           48                      // status panel, copper-split to fewer planes
#define HUD_BITPLANES   3
#define VIEW_H          (SCREEN_H - HUD_H)      // 208 visible rows of playfield

// --- Tiles ---------------------------------------------------------------------------------

#define TILE_SIZE       16
#define TILE_BYTES      (TILE_SIZE / 8 * TILE_SIZE * BITPLANES)   // 160, interleaved
#define TILE_COUNT      256
#define TILESHEET_BYTES (TILE_BYTES * TILE_COUNT)

// --- Playfield -----------------------------------------------------------------------------
//
// Double-wide buffer. Every incoming tile column is blitted twice, at x and x + PLAYFIELD_W/2,
// so the display window never has to wrap horizontally mid-line: we scroll the bitplane
// pointer across the first half, then jump it back by half a buffer, which is invisible
// because the content is duplicated. Costs 2x horizontal memory and buys a hitch-free
// infinite horizontal scroll.
//
// Vertical wrap is handled by a copper split that rewrites the bitplane pointers back to the
// top of the buffer at the wrap line, so the buffer only needs screen height + margin.

#define PLAYFIELD_HALF_W 352                    // 22 tiles: screen + 2 tiles of margin
#define PLAYFIELD_W      (PLAYFIELD_HALF_W * 2) // 704
#define PLAYFIELD_H      272                    // 17 tiles: view + margin

#define PLAYFIELD_ROW_BYTES  (PLAYFIELD_W / 8)                        // 88 per plane-row
#define PLAYFIELD_LINE_BYTES (PLAYFIELD_ROW_BYTES * BITPLANES)        // 440, interleaved
#define PLAYFIELD_BYTES      (PLAYFIELD_LINE_BYTES * PLAYFIELD_H)     // 119,680

#define PLAYFIELD_MODULO (PLAYFIELD_ROW_BYTES * (BITPLANES - 1) + (PLAYFIELD_ROW_BYTES - SCREEN_W / 8))

// --- Map -----------------------------------------------------------------------------------
// Tile index layer + attribute layer, both in slow RAM.

#define MAP_W           128
#define MAP_H           128
#define MAP_BYTES       (MAP_W * MAP_H)         // per layer

// Attribute bits for the collision layer (milestone 3)
#define ATTR_SOLID      (1 << 0)
#define ATTR_WATER      (1 << 1)
#define ATTR_HAZARD     (1 << 2)
#define ATTR_TRIGGER    (1 << 3)

// --- Bobs ----------------------------------------------------------------------------------
// Interleaved with an interleaved mask, same layout as bob.bpl: each plane row is followed by
// a copy of the mask row, so a 32px wide bob is 8 bytes per blit row.

#define BOB_W           32
#define BOB_H           32
#define BOB_FRAME_BYTES (BOB_H * BITPLANES * (BOB_W / 8 * 2))        // 1280

#define BOB_DIRECTIONS  8
#define BOB_ANIM_FRAMES 2
#define CLASS_BOB_BYTES (BOB_FRAME_BYTES * BOB_DIRECTIONS * BOB_ANIM_FRAMES)   // 20,480
#define PLAYERS_LOADED  2                       // only the two picked classes live in chip

#define HUD_BYTES       (SCREEN_W / 8 * HUD_H * HUD_BITPLANES)       // 5,760

// --- Chip RAM budget -----------------------------------------------------------------------
// Reserves for subsystems not yet written, so the budget stays honest as they land.

#define BUDGET_ENEMY_BOBS   40000
#define BUDGET_FX_BOBS      10000
#define BUDGET_MODULE       60000
#define BUDGET_COPPERLISTS   4000
#define BUDGET_SCRATCH      10000

#define CHIP_BUDGET_TOTAL ( \
	PLAYFIELD_BYTES + \
	TILESHEET_BYTES + \
	(CLASS_BOB_BYTES * PLAYERS_LOADED) + \
	HUD_BYTES + \
	BUDGET_ENEMY_BOBS + BUDGET_FX_BOBS + BUDGET_MODULE + \
	BUDGET_COPPERLISTS + BUDGET_SCRATCH)

#define CHIP_AVAILABLE  (512 * 1024)

// The playfield must be a whole number of tiles in both axes or the edge blitter has to
// handle partial tiles, which it does not.
static_assert(PLAYFIELD_HALF_W % TILE_SIZE == 0, "playfield half-width must be a whole tile count");
static_assert(PLAYFIELD_H % TILE_SIZE == 0, "playfield height must be a whole tile count");

// Margin of at least one tile each way, or a scroll step can outrun the edge blit.
static_assert(PLAYFIELD_HALF_W >= SCREEN_W + TILE_SIZE, "playfield needs >=1 tile of horizontal margin");
static_assert(PLAYFIELD_H >= VIEW_H + TILE_SIZE, "playfield needs >=1 tile of vertical margin");

static_assert(PLAYFIELD_BYTES == 119680, "playfield size drifted from the DESIGN.md budget");
static_assert(CLASS_BOB_BYTES == 20480, "class bob size drifted from the DESIGN.md budget");

// The whole point: if a subsystem grows past the target, fail here and not on an A500.
static_assert(CHIP_BUDGET_TOTAL < CHIP_AVAILABLE, "chip RAM budget exceeded for a 512KB chip A500");
static_assert(CHIP_AVAILABLE - CHIP_BUDGET_TOTAL > 150000,
	"less than 150KB left for per-world tile sets and enemies");
