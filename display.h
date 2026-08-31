#pragma once
#ifndef CHORLEY_DISPLAY_H
#define CHORLEY_DISPLAY_H

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include <exec/types.h>

// Blit a 16x16 tile (5 planes interleaved) into the double-wide playfield buffer
void blitTile(const UBYTE* tileSheet, UBYTE* playfield, int tileIdx, int bufTileX, int bufTileY);

// Fill the initial 22x17 view from map (duplicated at +22 cols)
void initPlayfield(UBYTE* playfield, const UBYTE* tileSheet, const UBYTE* map, int startCamX, int startCamY);

// Update playfield seams when camera moves across tile boundaries
void updateTileSeams(int oldTileX, int oldTileY, int newTileX, int newTileY,
                     const UBYTE* map, const UBYTE* tileSheet, UBYTE* playfield);

// Buffer x at which the copper starts reading, i.e. the buffer position of screen x 0.
//
// Normally that is just camX within one buffer half, but a bob straddling the left screen edge
// has to be clipped against a whole word of buffer *before* the read start, and when the read
// start lands inside the first tile there is none. The two halves hold identical pixels, so in
// that case the copper reads the far copy instead, which has 384 pixels of room in front of it.
// bobDraw uses the same anchor, and the two must agree or bobs land in the half nobody is
// looking at.
__attribute__((always_inline)) inline int playfieldReadX(int camX) {
	int x = camX % PLAYFIELD_HALF_W;
	if (x < TILE_SIZE) x += PLAYFIELD_HALF_W;
	return x;
}

// Build copper list for current camera position and HUD split
USHORT* buildCopperList(USHORT* copList, int camX, int camY, const UBYTE* playfield, const UBYTE* hud);

// Render the 320x48 3-bitplane HUD panel
void initHUD(UBYTE* hudBuffer);

// Refresh the live bob count and the raster line reached at the end of frame work. The line
// number is the M2 gate readout: it is how far down the frame the CPU and blitter got.
void hudSetCounters(UBYTE* hudBuffer, int bobs, int rasterLine);

#endif // CHORLEY_DISPLAY_H
