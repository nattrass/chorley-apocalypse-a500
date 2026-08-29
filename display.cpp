#include "display.h"
#include "copper.h"
#include "tiles.h"
#include "map.h"
#include "system.h"
#include <hardware/custom.h>

extern volatile struct Custom *custom;

void blitTile(const UBYTE* tileSheet, UBYTE* playfield, int tileIdx, int bufTileX, int bufTileY) {
	WaitBlt();
	const UBYTE* src = tileSheet + tileIdx * TILE_BYTES;
	UBYTE* dst = playfield + (bufTileY * TILE_SIZE * PLAYFIELD_LINE_BYTES) + (bufTileX * (TILE_SIZE / 8));

	custom->bltcon0 = 0x09f0; // A_TO_D | DEST | SRCA
	custom->bltcon1 = 0;
	custom->bltafwm = 0xffff;
	custom->bltalwm = 0xffff;
	custom->bltamod = 0;
	custom->bltdmod = PLAYFIELD_ROW_BYTES - 2; // 88 - 2 = 86
	custom->bltapt = (APTR)src;
	custom->bltdpt = (APTR)dst;
	custom->bltsize = ((TILE_SIZE * BITPLANES) << 6) | (TILE_SIZE / 16); // (80 << 6) | 1
}

void initPlayfield(UBYTE* playfield, const UBYTE* tileSheet, const UBYTE* map, int startCamX, int startCamY) {
	int startTileX = startCamX / TILE_SIZE;
	int startTileY = startCamY / TILE_SIZE;

	for (int ty = 0; ty < (PLAYFIELD_H / TILE_SIZE); ty++) {
		for (int tx = 0; tx < (PLAYFIELD_HALF_W / TILE_SIZE); tx++) {
			int mapTile = getMapTile(map, startTileX + tx, startTileY + ty);
			blitTile(tileSheet, playfield, mapTile, tx, ty);
			blitTile(tileSheet, playfield, mapTile, tx + 22, ty); // duplicate right half
		}
	}
}

void updateTileSeams(int oldTileX, int oldTileY, int newTileX, int newTileY,
                     const UBYTE* map, const UBYTE* tileSheet, UBYTE* playfield) {
	// Horizontal Seam Updates
	if (newTileX > oldTileX) {
		for (int tx = oldTileX + 1; tx <= newTileX; tx++) {
			int mapCol = tx + 21; // Entering column on right
			int bufCol = mapCol % (PLAYFIELD_HALF_W / TILE_SIZE); // % 22
			for (int r = 0; r < (PLAYFIELD_H / TILE_SIZE); r++) {
				int mapRow = newTileY + r;
				int tileIdx = getMapTile(map, mapCol, mapRow);
				int bufRow = mapRow % (PLAYFIELD_H / TILE_SIZE); // % 17
				blitTile(tileSheet, playfield, tileIdx, bufCol, bufRow);
				blitTile(tileSheet, playfield, tileIdx, bufCol + 22, bufRow);
			}
		}
	} else if (newTileX < oldTileX) {
		for (int tx = oldTileX - 1; tx >= newTileX; tx--) {
			int mapCol = tx; // Entering column on left
			int bufCol = mapCol % (PLAYFIELD_HALF_W / TILE_SIZE);
			for (int r = 0; r < (PLAYFIELD_H / TILE_SIZE); r++) {
				int mapRow = newTileY + r;
				int tileIdx = getMapTile(map, mapCol, mapRow);
				int bufRow = mapRow % (PLAYFIELD_H / TILE_SIZE);
				blitTile(tileSheet, playfield, tileIdx, bufCol, bufRow);
				blitTile(tileSheet, playfield, tileIdx, bufCol + 22, bufRow);
			}
		}
	}

	// Vertical Seam Updates
	if (newTileY > oldTileY) {
		for (int ty = oldTileY + 1; ty <= newTileY; ty++) {
			int mapRow = ty + 16; // Entering row at bottom
			int bufRow = mapRow % (PLAYFIELD_H / TILE_SIZE); // % 17
			for (int c = 0; c < (PLAYFIELD_HALF_W / TILE_SIZE); c++) {
				int mapCol = newTileX + c;
				int tileIdx = getMapTile(map, mapCol, mapRow);
				int bufCol = mapCol % (PLAYFIELD_HALF_W / TILE_SIZE); // % 22
				blitTile(tileSheet, playfield, tileIdx, bufCol, bufRow);
				blitTile(tileSheet, playfield, tileIdx, bufCol + 22, bufRow);
			}
		}
	} else if (newTileY < oldTileY) {
		for (int ty = oldTileY - 1; ty >= newTileY; ty--) {
			int mapRow = ty; // Entering row at top
			int bufRow = mapRow % (PLAYFIELD_H / TILE_SIZE);
			for (int c = 0; c < (PLAYFIELD_HALF_W / TILE_SIZE); c++) {
				int mapCol = newTileX + c;
				int tileIdx = getMapTile(map, mapCol, mapRow);
				int bufCol = mapCol % (PLAYFIELD_HALF_W / TILE_SIZE);
				blitTile(tileSheet, playfield, tileIdx, bufCol, bufRow);
				blitTile(tileSheet, playfield, tileIdx, bufCol + 22, bufRow);
			}
		}
	}
}

USHORT* buildCopperList(USHORT* copList, int camX, int camY, const UBYTE* playfield, const UBYTE* hud) {
	USHORT* copPtr = copList;

	int bufX = camX % PLAYFIELD_HALF_W;
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

	// Bitplane modulos (Interleaved 5 planes on 704-wide buffer = 400 bytes)
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

	// Vertical Wrap via Copper: if visible view extends past buffer bottom (line 272)
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

void initHUD(UBYTE* hudBuffer) {
	memclr(hudBuffer, HUD_BYTES);

	int lineBytes = (SCREEN_W / 8) * HUD_BITPLANES; // 120 bytes per scanline

	// 1. Draw metallic top border on HUD (lines 0..3)
	for (int y = 0; y < 3; y++) {
		UBYTE* row = hudBuffer + y * lineBytes;
		for (int b = 0; b < (SCREEN_W / 8); b++) {
			row[0 * (SCREEN_W / 8) + b] = 0xff; // Plane 0
			row[1 * (SCREEN_W / 8) + b] = (y < 2) ? 0xff : 0x00; // Plane 1
			row[2 * (SCREEN_W / 8) + b] = (y == 0) ? 0xff : 0x00; // Plane 2 (Highlight)
		}
	}

	// 2. Draw side and bottom frame borders
	for (int y = 3; y < HUD_H; y++) {
		UBYTE* row = hudBuffer + y * lineBytes;
		// Left border (byte 0)
		row[0] |= 0x80;
		// Right border (byte 39)
		row[39] |= 0x01;
	}

	// 3. Render HUD text
	drawTextPlanar(hudBuffer, lineBytes, HUD_BITPLANES, 8, 8,
	               "CHORLEY APOCALYPSE", 7);
	drawTextPlanar(hudBuffer, lineBytes, HUD_BITPLANES, 168, 8,
	               "W1: THE FLAT IRON", 5);

	drawTextPlanar(hudBuffer, lineBytes, HUD_BITPLANES, 8, 22,
	               "ARROWS/WASD/JOY: MOVE", 3);
	drawTextPlanar(hudBuffer, lineBytes, HUD_BITPLANES, 8, 34,
	               "SPACE: BOOST  ESC/LMB: EXIT", 6);

	// Health & Brass labels
	drawTextPlanar(hudBuffer, lineBytes, HUD_BITPLANES, 240, 22,
	               "BRASS: 0000", 7);
	drawTextPlanar(hudBuffer, lineBytes, HUD_BITPLANES, 240, 34,
	               "HEALTH: ||||", 7);
}
