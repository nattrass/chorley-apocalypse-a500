#pragma once
#ifndef CHORLEY_MAP_H
#define CHORLEY_MAP_H

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include <exec/types.h>

// Procedural 128x128 map generator representing post-apocalyptic Chorley
void generateMap(UBYTE* map);

// Get tile index at (tx, ty) with boundary safety
__attribute__((always_inline)) inline UBYTE getMapTile(const UBYTE* map, int tx, int ty) {
	if (tx < 0) tx = 0;
	else if (tx >= MAP_W) tx = MAP_W - 1;
	if (ty < 0) ty = 0;
	else if (ty >= MAP_H) ty = MAP_H - 1;
	return map[ty * MAP_W + tx];
}

#endif // CHORLEY_MAP_H
