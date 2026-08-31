#include "display.h"
#include "copper.h"
#include "tiles.h"
#include "map.h"
#include "system.h"
#include <hardware/custom.h>

extern volatile struct Custom *custom;

__attribute__((always_inline)) static inline int posMod(int v, int m) {
	int r = v % m;
	return (r < 0) ? r + m : r;
}

void blitTile(const UBYTE* tileSheet, UBYTE* playfield, int tileIdx, int bufTileX, int bufTileY) {
	WaitBlt();
	const UBYTE* src = tileSheet + tileIdx * TILE_BYTES;
	UBYTE* dst = playfield + (bufTileY * TILE_SIZE * PLAYFIELD_LINE_BYTES) + (bufTileX * (TILE_SIZE / 8));

	custom->bltcon0 = 0x09f0; // A_TO_D | DEST | SRCA
	custom->bltcon1 = 0;
	custom->bltafwm = 0xffff;
	custom->bltalwm = 0xffff;
	custom->bltamod = 0;
	custom->bltdmod = PLAYFIELD_ROW_BYTES - 2;
	custom->bltapt = (APTR)src;
	custom->bltdpt = (APTR)dst;
	custom->bltsize = ((TILE_SIZE * BITPLANES) << 6) | (TILE_SIZE / 16);
}

// The buffer invariant: map column c always lives in buffer slot c % BUF_COLS and map row r in
// slot r % BUF_ROWS, which is what puts world pixel x at buffer pixel x % PLAYFIELD_HALF_W and
// lets the copper scroll by moving the bitplane pointers alone. Every column is written twice,
// BUF_COLS apart, so the display window never straddles the end of the buffer.
static void blitMapTile(const UBYTE* map, const UBYTE* tileSheet, UBYTE* playfield, int c, int r) {
	const int tile = getMapTile(map, c, r);
	const int bc   = posMod(c, BUF_COLS);
	const int br   = posMod(r, BUF_ROWS);
	blitTile(tileSheet, playfield, tile, bc, br);
	blitTile(tileSheet, playfield, tile, bc + BUF_COLS, br);
}

void initPlayfield(UBYTE* playfield, const UBYTE* tileSheet, const UBYTE* map, int startCamX, int startCamY) {
	const int c0 = (startCamX / TILE_SIZE) - BUF_ANCHOR;
	const int r0 = (startCamY / TILE_SIZE) - BUF_ANCHOR;

	for (int r = r0; r < r0 + BUF_ROWS; r++)
		for (int c = c0; c < c0 + BUF_COLS; c++)
			blitMapTile(map, tileSheet, playfield, c, r);
}

void updateTileSeams(int oldTileX, int oldTileY, int newTileX, int newTileY,
                     const UBYTE* map, const UBYTE* tileSheet, UBYTE* playfield) {
	// Loaded window before and after the move. Anything in the new window that was not in the
	// old one has to be blitted in; its slot is currently holding the column or row that just
	// dropped off the far side.
	const int c0 = newTileX - BUF_ANCHOR, r0 = newTileY - BUF_ANCHOR;
	const int oldC0 = oldTileX - BUF_ANCHOR, oldR0 = oldTileY - BUF_ANCHOR;

	if (newTileX != oldTileX) {
		int first, last;
		if (newTileX > oldTileX) { first = oldC0 + BUF_COLS; last = c0 + BUF_COLS - 1; }
		else                     { first = c0;               last = oldC0 - 1; }
		if (last - first >= BUF_COLS) first = last - BUF_COLS + 1;  // jumped clear of the window

		for (int c = first; c <= last; c++)
			for (int r = r0; r < r0 + BUF_ROWS; r++)
				blitMapTile(map, tileSheet, playfield, c, r);
	}

	if (newTileY != oldTileY) {
		int first, last;
		if (newTileY > oldTileY) { first = oldR0 + BUF_ROWS; last = r0 + BUF_ROWS - 1; }
		else                     { first = r0;               last = oldR0 - 1; }
		if (last - first >= BUF_ROWS) first = last - BUF_ROWS + 1;

		for (int r = first; r <= last; r++)
			for (int c = c0; c < c0 + BUF_COLS; c++)
				blitMapTile(map, tileSheet, playfield, c, r);
	}
}

USHORT* buildCopperList(USHORT* copList, int camX, int camY, const UBYTE* playfield, const UBYTE* hud) {
	USHORT* copPtr = copList;

	int bufX = playfieldReadX(camX);
	int bufY = camY % PLAYFIELD_H;
	int wordOffset = (bufX / 16) * 2;
	int subX = 15 - (camX & 15);
	UWORD bplcon1_val = subX | (subX << 4);

	copPtr = screenScanDefault(copPtr);

	// 32 Colors from gamePalette
	for (int i = 0; i < 32; i++) {
		copPtr = copSetColor(copPtr, i, gamePalette[i]);
	}

	// Playfield display registers
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0 << 10)/*dual pf*/ | (1 << 9)/*color*/ | (BITPLANES << 12);
	*copPtr++ = offsetof(struct Custom, bplcon1);
	*copPtr++ = bplcon1_val;
	*copPtr++ = offsetof(struct Custom, bplcon2);
	*copPtr++ = 1 << 6;

	// Bitplane modulos (interleaved 5 planes on the double-wide buffer)
	*copPtr++ = offsetof(struct Custom, bpl1mod);
	*copPtr++ = PLAYFIELD_MODULO;
	*copPtr++ = offsetof(struct Custom, bpl2mod);
	*copPtr++ = PLAYFIELD_MODULO;

	// Top of playfield bitplane pointers
	const UBYTE* planes[BITPLANES];
	for (int p = 0; p < BITPLANES; p++) {
		planes[p] = playfield + (bufY * PLAYFIELD_LINE_BYTES) + (p * PLAYFIELD_ROW_BYTES) + wordOffset;
	}
	copPtr = copSetPlanes(0, copPtr, planes, BITPLANES);

	// Vertical wrap via copper: if the visible view extends past the bottom of the buffer
	if (bufY + VIEW_H > PLAYFIELD_H) {
		int wrapLine = 44 + (PLAYFIELD_H - bufY);
		copPtr = copWaitY(copPtr, wrapLine);
		const UBYTE* wrapPlanes[BITPLANES];
		for (int p = 0; p < BITPLANES; p++) {
			wrapPlanes[p] = playfield + (p * PLAYFIELD_ROW_BYTES) + wordOffset;
		}
		copPtr = copSetPlanes(0, copPtr, wrapPlanes, BITPLANES);
	}

	// HUD Split at line 252 (Y = 44 + 208 = 252 = 0xFC)
	copPtr = copWaitY(copPtr, 252);
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0 << 10) | (1 << 9) | (HUD_BITPLANES << 12); // 3 planes for HUD (8 colors)
	*copPtr++ = offsetof(struct Custom, bplcon1);
	*copPtr++ = 0; // no horizontal scroll for HUD
	*copPtr++ = offsetof(struct Custom, bpl1mod);
	*copPtr++ = (SCREEN_W / 8) * (HUD_BITPLANES - 1); // 40 * 2 = 80 bytes
	*copPtr++ = offsetof(struct Custom, bpl2mod);
	*copPtr++ = (SCREEN_W / 8) * (HUD_BITPLANES - 1);

	const UBYTE* hudPlanes[HUD_BITPLANES];
	for (int p = 0; p < HUD_BITPLANES; p++) {
		hudPlanes[p] = hud + (p * (SCREEN_W / 8));
	}
	copPtr = copSetPlanes(0, copPtr, hudPlanes, HUD_BITPLANES);

	// End copper list
	*copPtr++ = 0xffff;
	*copPtr++ = 0xfffe;

	return copPtr;
}

#define HUD_LINE_BYTES ((SCREEN_W / 8) * HUD_BITPLANES)

// drawTextPlanar only touches the pixels a glyph sets, so a shorter number would leave the tail
// of the last one behind. Anything redrawn every frame gets its box cleared first.
static void clearHudBox(UBYTE* hudBuffer, int x, int y, int w, int h) {
	for (int row = y; row < y + h; row++) {
		UBYTE* linePtr = hudBuffer + row * HUD_LINE_BYTES;
		for (int px = x; px < x + w; px++) {
			const int   byteOff = px >> 3;
			const UBYTE mask    = (UBYTE)(1 << (7 - (px & 7)));
			for (int p = 0; p < HUD_BITPLANES; p++)
				linePtr[p * (SCREEN_W / 8) + byteOff] &= (UBYTE)~mask;
		}
	}
}

static void formatNumber(char* out, int value, int digits) {
	for (int i = digits - 1; i >= 0; i--) {
		out[i] = (char)('0' + (value % 10));
		value /= 10;
	}
	out[digits] = 0;
}

void initHUD(UBYTE* hudBuffer) {
	memclr(hudBuffer, HUD_BYTES);

	// 1. Metallic top border
	for (int y = 0; y < 3; y++) {
		UBYTE* row = hudBuffer + y * HUD_LINE_BYTES;
		for (int b = 0; b < (SCREEN_W / 8); b++) {
			row[0 * (SCREEN_W / 8) + b] = 0xff;
			row[1 * (SCREEN_W / 8) + b] = (y < 2) ? 0xff : 0x00;
			row[2 * (SCREEN_W / 8) + b] = (y == 0) ? 0xff : 0x00;
		}
	}

	// 2. Side borders
	for (int y = 3; y < HUD_H; y++) {
		UBYTE* row = hudBuffer + y * HUD_LINE_BYTES;
		row[0]  |= 0x80;
		row[39] |= 0x01;
	}

	// 3. Fixed text. Eight pixels per character, so nothing may start past x=248.
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES,   8,  8, "CHORLEY APOCALYPSE", 7);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES, 168,  8, "M2: PLAYER", 5);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES,   8, 20, "JOY/WASD MOVE FIRE=SPACE", 3);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES,   8, 32, "T: STRESS  ESC/LMB: EXIT", 6);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES, 208, 20, "BOBS:", 7);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES, 208, 32, "LINE:", 7);
}

void hudSetCounters(UBYTE* hudBuffer, int bobs, int rasterLine) {
	char digits[4];

	if (bobs > 99) bobs = 99;
	clearHudBox(hudBuffer, 248, 20, 24, 8);
	formatNumber(digits, bobs, 2);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES, 248, 20, digits, 7);

	if (rasterLine > 999) rasterLine = 999;
	clearHudBox(hudBuffer, 248, 32, 24, 8);
	formatNumber(digits, rasterLine, 3);
	drawTextPlanar(hudBuffer, HUD_LINE_BYTES, HUD_BITPLANES, 248, 32, digits, 7);
}
