#pragma once
#ifndef CHORLEY_TILES_H
#define CHORLEY_TILES_H

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include <exec/types.h>

// 32-color palette in OCS format (0x0RGB) according to DESIGN.md
extern const UWORD gamePalette[32];

// Generate 32 distinct procedural tiles (16x16, 5 bitplanes interleaved),
// each visibly numbered 00..31 with high-contrast borders and unique textures.
void generateTileSheet(UBYTE* tileSheet);

// Draw string using built-in font into an interleaved planar bitmap
void drawTextPlanar(UBYTE* buffer, int lineBytes, int numPlanes, int x, int y, const char* text, int color);

#endif // CHORLEY_TILES_H
