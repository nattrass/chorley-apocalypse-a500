#include "bob.h"
#include "display.h"
#include "map.h"
#include "system.h"
#include <hardware/custom.h>

extern volatile struct Custom *custom;

// 0 = north, then clockwise. Diagonals are 11/16 ~= 0.707 so all eight have the same length.
const short dirX[BOB_DIRECTIONS] = {   0,  11,  16,  11,   0, -11, -16, -11 };
const short dirY[BOB_DIRECTIONS] = { -16, -11,   0,  11,  16,  11,   0, -11 };

__attribute__((always_inline)) static inline int posMod(int v, int m) {
	int r = v % m;
	return (r < 0) ? r + m : r;
}

// --- drawing ------------------------------------------------------------------------------

bool bobDraw(const RenderCtx* ctx, const UBYTE* frame, int srcWords, int h, int wx, int wy, BobRect* rec) {
	// The world window the buffer currently holds, anchored BUF_ANCHOR tiles before the camera.
	const int winCol0 = (ctx->camX >> 4) - BUF_ANCHOR;
	const int winX0   = winCol0 << 4;
	const int winY0   = ((ctx->camY >> 4) - BUF_ANCHOR) << 4;

	// Buffer x of screen x 0, and from it the buffer x of the bob. Taking the anchor from the
	// copper rather than from the map column keeps the bob in the half the display is actually
	// reading: the two buffer halves hold identical pixels, but only one of them is on screen.
	const int readX = playfieldReadX(ctx->camX);
	const int bx    = readX + (wx - ctx->camX);
	const int loWord = (readX + (winX0 - ctx->camX)) >> 4;   // first loaded word, always >= 0

	// Horizontal clip, in whole words. Clip the blit rather than the bob: the buffer holds only
	// BUF_COLS columns of the world and anything outside them has no address to be drawn at, so
	// an unclipped bob at the screen edge would wrap round and reappear on the other side.
	//
	// BUF_ANCHOR is what makes the clip lossless. Dropping the first source word also drops the
	// pixels it would have shifted into the second, and with the window anchored on the camera
	// tile those lost pixels sit exactly on the left screen edge. One tile of lead puts them off
	// screen instead.
	const int shift = wx & 15;
	int destWord = bx >> 4;
	int words    = srcWords;
	int skip     = loWord - destWord;
	if (skip > 0) { destWord += skip; words -= skip; } else { skip = 0; }
	const int over = (destWord + words) - (loWord + BUF_COLS);
	if (over > 0) words -= over;
	if (words <= 0) return false;

	// Vertical clip, exact to the row.
	int y0   = wy;
	int rows = h;
	if (y0 < winY0) { rows -= (winY0 - y0); y0 = winY0; }
	if (y0 + rows > winY0 + PLAYFIELD_H) rows = winY0 + PLAYFIELD_H - y0;
	if (rows <= 0) return false;

	const int srcRowBytes = srcWords * 4;                      // data words, then mask words
	const UBYTE* srcData  = frame + (y0 - wy) * BITPLANES * srcRowBytes + skip * 2;
	const UBYTE* srcMask  = srcData + srcWords * 2;

	const UWORD con0   = (UWORD)((shift << 12) | 0x0fca);      // A=mask B=source C=dest D=dest
	const UWORD con1   = (UWORD)(shift << 12);
	const UWORD srcMod = (UWORD)(srcRowBytes - words * 2);
	const UWORD dstMod = (UWORD)(PLAYFIELD_ROW_BYTES - words * 2);

	// Vertical wrap: the copper jumps the bitplane pointers back to the top of the buffer, so a
	// bob crossing that line is two blits, the second starting at buffer row 0.
	int by    = posMod(y0, PLAYFIELD_H);
	int rows1 = rows;
	if (by + rows1 > PLAYFIELD_H) rows1 = PLAYFIELD_H - by;

	UBYTE* dst = ctx->playfield + by * PLAYFIELD_LINE_BYTES + destWord * 2;

	WaitBlt();
	custom->bltcon0 = con0;
	custom->bltcon1 = con1;
	custom->bltafwm = 0xffff;
	custom->bltalwm = 0xffff;
	custom->bltamod = srcMod;
	custom->bltbmod = srcMod;
	custom->bltcmod = dstMod;
	custom->bltdmod = dstMod;
	custom->bltapt  = (APTR)srcMask;
	custom->bltbpt  = (APTR)srcData;
	custom->bltcpt  = (APTR)dst;
	custom->bltdpt  = (APTR)dst;
	custom->bltsize = (UWORD)(((rows1 * BITPLANES) << 6) | words);

	if (rows1 < rows) {
		const int    off  = rows1 * BITPLANES * srcRowBytes;
		UBYTE* const dst2 = ctx->playfield + destWord * 2;
		WaitBlt();
		custom->bltcon0 = con0;
		custom->bltcon1 = con1;
		custom->bltamod = srcMod;
		custom->bltbmod = srcMod;
		custom->bltcmod = dstMod;
		custom->bltdmod = dstMod;
		custom->bltapt  = (APTR)(srcMask + off);
		custom->bltbpt  = (APTR)(srcData + off);
		custom->bltcpt  = (APTR)dst2;
		custom->bltdpt  = (APTR)dst2;
		custom->bltsize = (UWORD)((((rows - rows1) * BITPLANES) << 6) | words);
	}

	rec->bufC0 = (UBYTE)destWord;
	rec->nc    = (UBYTE)words;
	rec->mapC0 = (short)(winCol0 + (destWord - loWord));
	rec->mapR0 = (short)(y0 >> 4);
	rec->nr    = (UBYTE)(((y0 + rows - 1) >> 4) - (y0 >> 4) + 1);
	return true;
}

void bobRestore(const RenderCtx* ctx, const BobRect* rec) {
	for (int i = 0; i < rec->nr; i++) {
		const int r  = rec->mapR0 + i;
		const int br = posMod(r, BUF_ROWS);
		for (int j = 0; j < rec->nc; j++) {
			const int c = rec->mapC0 + j;
			blitTile(ctx->tileSheet, ctx->playfield, getMapTile(ctx->map, c, r), rec->bufC0 + j, br);
		}
	}
}

// --- procedural bob art -------------------------------------------------------------------

// Colour 0 is transparent: it clears the mask bit rather than writing black.
static void setBobPixel(UBYTE* frame, int words, int x, int y, int color) {
	UBYTE* const rowBase = frame + (y * BITPLANES) * (words * 4);
	const int    byteOff = x >> 3;
	const UBYTE  bit     = (UBYTE)(1 << (7 - (x & 7)));

	for (int p = 0; p < BITPLANES; p++) {
		UBYTE* planeRow = rowBase + p * (words * 4);
		UBYTE* data     = planeRow + byteOff;
		UBYTE* mask     = planeRow + words * 2 + byteOff;
		if (color > 0) {
			*mask |= bit;
			if ((color >> p) & 1) *data |= bit; else *data = (UBYTE)(*data & ~bit);
		} else {
			*mask = (UBYTE)(*mask & ~bit);
			*data = (UBYTE)(*data & ~bit);
		}
	}
}

static void fillDisc(UBYTE* frame, int words, int size, int cx, int cy, int r, int color) {
	const int r2 = r * r;
	for (int y = cy - r; y <= cy + r; y++) {
		if (y < 0 || y >= size) continue;
		for (int x = cx - r; x <= cx + r; x++) {
			if (x < 0 || x >= size) continue;
			const int dx = x - cx, dy = y - cy;
			if (dx * dx + dy * dy <= r2) setBobPixel(frame, words, x, y, color);
		}
	}
}

// A thick line drawn as a run of discs. Startup only, so the cost does not matter.
static void fillStroke(UBYTE* frame, int words, int size, int x0, int y0, int x1, int y1, int r, int color) {
	const int dx = x1 - x0, dy = y1 - y0;
	int steps = (dx < 0 ? -dx : dx);
	const int ady = (dy < 0 ? -dy : dy);
	if (ady > steps) steps = ady;
	if (steps == 0) steps = 1;
	for (int s = 0; s <= steps; s++)
		fillDisc(frame, words, size, x0 + dx * s / steps, y0 + dy * s / steps, r, color);
}

// The Mill-Hand, in placeholder form: a mill worker in a rust coat and a brass helmet, with the
// barrel pointing wherever the shots are going. Frames differ only in the leg swing.
void generatePlayerBobs(UBYTE* sheet) {
	memclr(sheet, CLASS_BOB_BYTES);

	for (int d = 0; d < BOB_DIRECTIONS; d++) {
		const int vx = dirX[d], vy = dirY[d];   // facing
		const int px = -vy,     py = vx;        // and its perpendicular

		for (int f = 0; f < BOB_ANIM_FRAMES; f++) {
			UBYTE* const frame = sheet + (d * BOB_ANIM_FRAMES + f) * BOB_FRAME_BYTES;
			const int cx = 16, cy = 16;
			const int swing = f ? 3 : -3;

			// Drop shadow, so the figure reads against a busy tile set.
			fillDisc(frame, BOB_WORDS, BOB_W, cx, cy + 7, 8, 1);

			// Legs, alternating along the facing axis
			fillDisc(frame, BOB_WORDS, BOB_W,
			         cx + (px * 5 + vx * swing) / 16, cy + (py * 5 + vy * swing) / 16, 3, 9);
			fillDisc(frame, BOB_WORDS, BOB_W,
			         cx - (px * 5 - vx * swing) / 16, cy - (py * 5 - vy * swing) / 16, 3, 9);

			// Torso: dark rust coat, lit away from the facing
			fillDisc(frame, BOB_WORDS, BOB_W, cx, cy, 8, 12);
			fillDisc(frame, BOB_WORDS, BOB_W, cx - vx * 2 / 16, cy - vy * 2 / 16, 6, 13);
			fillDisc(frame, BOB_WORDS, BOB_W, cx - vx * 4 / 16, cy - vy * 4 / 16, 4, 10);

			// Shoulders
			fillDisc(frame, BOB_WORDS, BOB_W, cx + px * 6 / 16, cy + py * 6 / 16, 4, 3);
			fillDisc(frame, BOB_WORDS, BOB_W, cx - px * 6 / 16, cy - py * 6 / 16, 4, 3);

			// Gun barrel
			fillStroke(frame, BOB_WORDS, BOB_W,
			           cx + px * 5 / 16 + vx * 4 / 16,  cy + py * 5 / 16 + vy * 4 / 16,
			           cx + px * 5 / 16 + vx * 14 / 16, cy + py * 5 / 16 + vy * 14 / 16, 1, 4);
			fillDisc(frame, BOB_WORDS, BOB_W,
			         cx + px * 5 / 16 + vx * 14 / 16, cy + py * 5 / 16 + vy * 14 / 16, 1, 6);

			// Head: brass helmet, highlight up and to the left, dark visor at the front
			const int hx = cx + vx * 3 / 16, hy = cy + vy * 3 / 16;
			fillDisc(frame, BOB_WORDS, BOB_W, hx, hy, 5, 20);
			fillDisc(frame, BOB_WORDS, BOB_W, hx - 1, hy - 1, 3, 22);
			fillDisc(frame, BOB_WORDS, BOB_W, hx + vx * 3 / 16, hy + vy * 3 / 16, 1, 0);
		}
	}
}

void generateBulletBobs(UBYTE* sheet) {
	memclr(sheet, BULLET_SHEET_BYTES);

	for (int d = 0; d < BOB_DIRECTIONS; d++) {
		UBYTE* const frame = sheet + d * BULLET_FRAME_BYTES;
		const int vx = dirX[d], vy = dirY[d];
		const int cx = 8, cy = 8;

		// Brass tail streaking back, cyan core, white head.
		fillStroke(frame, BULLET_WORDS, BULLET_H,
		           cx - vx * 6 / 16, cy - vy * 6 / 16, cx, cy, 1, 20);
		fillStroke(frame, BULLET_WORDS, BULLET_H,
		           cx - vx * 3 / 16, cy - vy * 3 / 16,
		           cx + vx * 3 / 16, cy + vy * 3 / 16, 1, 25);
		fillDisc(frame, BULLET_WORDS, BULLET_H, cx + vx * 4 / 16, cy + vy * 4 / 16, 1, 31);
	}
}
