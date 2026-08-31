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

#define PLAYFIELD_HALF_W 384                    // 24 tiles: screen + 3 tiles of margin
#define PLAYFIELD_W      (PLAYFIELD_HALF_W * 2) // 768
#define PLAYFIELD_H      272                    // 17 tiles: view + margin

// Tile grid of one buffer half, and the buffer's world window.
//
// The buffer slot for map column c is always c % BUF_COLS, and for map row r always r % BUF_ROWS
// -- that mapping is what puts world pixel x at buffer pixel x % PLAYFIELD_HALF_W. What the
// margin buys is *which* columns are loaded: the window is anchored BUF_ANCHOR tiles before the
// camera tile, so a bob straddling the left or top screen edge still has buffer to be clipped
// against instead of wrapping round to the opposite edge.
//
//   columns loaded: [camTileX - BUF_ANCHOR, camTileX - BUF_ANCHOR + BUF_COLS - 1]
//   rows    loaded: [camTileY - BUF_ANCHOR, camTileY - BUF_ANCHOR + BUF_ROWS - 1]

#define BUF_COLS        (PLAYFIELD_HALF_W / TILE_SIZE)   // 24
#define BUF_ROWS        (PLAYFIELD_H / TILE_SIZE)        // 17
#define BUF_ANCHOR      1                                // tiles of margin before the camera tile

#define VIEW_COLS       (SCREEN_W / TILE_SIZE + 1)       // 21 columns can be on screen at once
#define VIEW_ROWS       (VIEW_H / TILE_SIZE + 1)         // 14 rows can be on screen at once

#define PLAYFIELD_ROW_BYTES  (PLAYFIELD_W / 8)                        // 96 per plane-row
#define PLAYFIELD_LINE_BYTES (PLAYFIELD_ROW_BYTES * BITPLANES)        // 480, interleaved
#define PLAYFIELD_BYTES      (PLAYFIELD_LINE_BYTES * PLAYFIELD_H)     // 130,560

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
// a copy of the mask row.
//
// A bob is stored one word wider than it draws. The blitter shifts A and B right to reach a
// sub-word X, and the pixels shifted out of the last word have to land somewhere -- that guard
// word is where. Without it a bob could only be drawn on 16-pixel boundaries.

#define BOB_W           32
#define BOB_H           32
#define BOB_WORDS       (BOB_W / 16 + 1)                             // 3: 2 of bob, 1 of guard
#define BOB_ROW_BYTES   (BOB_WORDS * 2 * 2)                          // 12: data words then mask
#define BOB_FRAME_BYTES (BOB_H * BITPLANES * BOB_ROW_BYTES)          // 1,920

#define BOB_DIRECTIONS  8
#define BOB_ANIM_FRAMES 2
#define CLASS_BOB_BYTES (BOB_FRAME_BYTES * BOB_DIRECTIONS * BOB_ANIM_FRAMES)   // 30,720
#define PLAYERS_LOADED  2                       // only the two picked classes live in chip

// Bullets are 16x16 in the same layout, one frame per direction, no animation.

#define BULLET_W           16
#define BULLET_H           16
#define BULLET_WORDS       (BULLET_W / 16 + 1)                       // 2: 1 of bob, 1 of guard
#define BULLET_ROW_BYTES   (BULLET_WORDS * 2 * 2)                    // 8
#define BULLET_FRAME_BYTES (BULLET_H * BITPLANES * BULLET_ROW_BYTES) // 640
#define BULLET_SHEET_BYTES (BULLET_FRAME_BYTES * BOB_DIRECTIONS)     // 5,120

#define MAX_BULLETS        24                   // the pool, and the M2 gate bob count

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

// Bob clipping needs margin on both sides of the view, not only after it. Before the view it
// comes from BUF_ANCHOR; after it, from whatever the view leaves over. A bob blit is BOB_WORDS
// wide, so the trailing margin has to cover a whole blit and not merely the bob.
static_assert(BUF_ANCHOR >= 1, "a bob at the left or top screen edge has no buffer to clip against");
static_assert(BUF_COLS - BUF_ANCHOR - VIEW_COLS >= BOB_WORDS - 1,
	"not enough columns after the view for a bob blit at the right screen edge");
static_assert(BUF_ROWS - BUF_ANCHOR - VIEW_ROWS >= BOB_H / TILE_SIZE,
	"not enough rows after the view for a bob at the bottom screen edge");

static_assert(PLAYFIELD_BYTES == 130560, "playfield size drifted from the DESIGN.md budget");
static_assert(CLASS_BOB_BYTES == 30720, "class bob size drifted from the DESIGN.md budget");

// The whole point: if a subsystem grows past the target, fail here and not on an A500.
static_assert(CHIP_BUDGET_TOTAL < CHIP_AVAILABLE, "chip RAM budget exceeded for a 512KB chip A500");
static_assert(CHIP_AVAILABLE - CHIP_BUDGET_TOTAL > 150000,
	"less than 150KB left for per-world tile sets and enemies");
