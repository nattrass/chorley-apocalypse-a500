#pragma once
#ifndef CHORLEY_BOB_H
#define CHORLEY_BOB_H

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include <exec/types.h>

// Bob drawing over the scrolling playfield.
//
// The playfield is single-buffered and scrolls under the bobs, so "restore behind" cannot mean
// "put back the pixels that were there" from a saved copy: the buffer is indexed by *world*
// position (map column c always lives in buffer slot c % BUF_COLS), so the background under a
// bob is fully described by the map tiles it covered. Restoring is therefore re-blitting those
// tiles from the tile sheet -- no scratch chip RAM, no ordering constraints between overlapping
// bobs, and immune to the seam blitter having rewritten the area in between.
//
// The frame order that makes it work is:
//
//   1. restore every rect drawn last frame          (playfield is pure background again)
//   2. move entities and the camera
//   3. blit the tile seams for the new camera
//   4. draw every bob, recording the rect it covered
//
// Step 1 before step 3 because a restore must not put stale tiles back over a fresh seam.

struct RenderCtx {
	UBYTE*       playfield;
	const UBYTE* tileSheet;
	const UBYTE* map;
	int          camX, camY;
};

// The tile rect a bob covered, in map coordinates plus the buffer half it was drawn in.
// Recorded by bobDraw, consumed by bobRestore on the next frame.
struct BobRect {
	short mapC0, mapR0;
	UBYTE bufC0;        // buffer tile column of mapC0: picks which of the two duplicate halves
	UBYTE nc, nr;
};

// Draw one bob frame with its mask, cookie-cut, at world pixel (wx, wy). Clips to the buffer's
// loaded window and splits across the vertical wrap. Returns false if nothing was drawn, in
// which case rec is not filled in.
bool bobDraw(const RenderCtx* ctx, const UBYTE* frame, int srcWords, int h, int wx, int wy, BobRect* rec);

// Put the background back by re-blitting the map tiles the rect covered.
void bobRestore(const RenderCtx* ctx, const BobRect* rec);

// Procedural placeholder art, generated at startup inside warpmode().
void generatePlayerBobs(UBYTE* sheet);   // CLASS_BOB_BYTES: 8 directions x 2 anim frames, 32x32
void generateBulletBobs(UBYTE* sheet);   // BULLET_SHEET_BYTES: 8 directions, 16x16

// Unit vectors for the 8 facings, in 1/16 pixel. 0 = north, then clockwise.
extern const short dirX[BOB_DIRECTIONS];
extern const short dirY[BOB_DIRECTIONS];

#endif // CHORLEY_BOB_H
