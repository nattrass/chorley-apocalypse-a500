#pragma once
#ifndef CHORLEY_SPRITES_H
#define CHORLEY_SPRITES_H

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include <exec/types.h>

// Bullets on hardware sprites instead of the blitter.
//
// After the M2 gate, the frame is blitter-bound: a 32x32 bob costs about 38 raster lines of
// draw plus restore and a 16x16 one about 24, against a budget of 296. Sprite DMA costs the
// blitter nothing at all, so anything that can ride a sprite stops competing for the only
// resource that is actually scarce.
//
// What the hardware gives, and what it costs:
//
//   - 8 channels, each 16 pixels wide and any height, 3 colours plus transparent.
//   - The colours come from fixed triples in the upper half of the palette, and with 5
//     bitplanes the playfield owns those same registers, so every channel pair used costs the
//     tile art three colours. Bullets take channels 6 and 7, which share the single triple
//     29-31 -- the cheapest pair available, because only the hazard stripes were using it.
//     Going to four channels means also spending 25-27, which the Winter Hill signal tiles
//     want; do that only if enemy fire turns out to need the capacity.
//   - One channel can carry many bullets down the screen by chaining their control words,
//     but only one at a time: two bullets whose rows overlap need two channels.
//
// That last point is the real limit. Bullets fired along a horizontal run all share a row
// band, so a sideways stream can want more channels than exist. Rather than drop those
// bullets, spritesBuild reports which ones it could not place and the caller draws those as
// bobs -- so the worst case is exactly the old cost and the common case is far cheaper.

#define SPR_FIRST_CHANNEL 6
#define SPR_CHANNELS      2
#define SPR_TOTAL         8

// Two control words, then one data word per plane per line, then two words to end the chain.
#define SPR_ENTRY_WORDS   (2 + BULLET_H * 2)
#define SPR_CHAIN_WORDS   (2 + MAX_BULLETS * SPR_ENTRY_WORDS)
#define SPR_BUFFER_WORDS  (SPR_CHANNELS * SPR_CHAIN_WORDS + 2)   // + the null chain
#define SPR_BUFFER_BYTES  (SPR_BUFFER_WORDS * 2)

// One 16x16 two-plane image per facing.
#define SPR_IMAGE_WORDS   (BULLET_H * 2)
#define SPR_SHEET_WORDS   (BOB_DIRECTIONS * SPR_IMAGE_WORDS)
#define SPR_SHEET_BYTES   (SPR_SHEET_WORDS * 2)

// Display geometry, from screenScanDefault() in copper.h.
#define DIW_X0            129
#define DIW_Y0            44

struct SpriteEnt {
	short x, y;     // world pixels, top-left
	UBYTE dir;      // 0..7, indexes the sheet
};

// Procedural placeholder bullet images, generated at startup inside warpmode().
void generateBulletSprites(UWORD* sheet);

// Build this frame's sprite chains.
//
// ents[0..n-1] are the live bullets in world coordinates. Writes one chain per channel into
// buf and fills chainStart[] with the address to point each SPRxPT at, including the channels
// bullets do not use. placed[i] is set to 1 for every entity that got a sprite slot and 0 for
// every one the caller still has to draw as a bob. Returns how many were placed.
int spritesBuild(UWORD* buf, const UWORD* sheet, const SpriteEnt* ents, int n,
                 const UWORD* chainStart[SPR_TOTAL], UBYTE* placed, int camX, int camY);

#endif // CHORLEY_SPRITES_H
