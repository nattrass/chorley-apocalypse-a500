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

// Build copper list for current camera position and HUD split
USHORT* buildCopperList(USHORT* copList, int camX, int camY, const UBYTE* playfield, const UBYTE* hud);

// Render the 320x48 3-bitplane HUD panel
void initHUD(UBYTE* hudBuffer);

#endif // CHORLEY_DISPLAY_H
