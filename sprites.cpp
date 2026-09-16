#include "sprites.h"
#include "bob.h"

// --- chain building -------------------------------------------------------------------------

// A sprite is visible on raster lines [vstart, vstop). Both are 9 bit, and the ninth bit of
// each plus the low bit of hstart live in the second control word.
__attribute__((always_inline)) static inline void writeControl(UWORD* w, int hstart, int vstart, int vstop) {
	w[0] = (UWORD)(((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff));
	w[1] = (UWORD)(((vstop & 0xff) << 8)
	             | ((vstart & 0x100) >> 6)
	             | ((vstop  & 0x100) >> 7)
	             |  (hstart & 1));
}

int spritesBuild(UWORD* buf, const UWORD* sheet, const SpriteEnt* ents, int n,
                 const UWORD* chainStart[SPR_TOTAL], UBYTE* placed, int camX, int camY) {
	// The first raster line below the play area. Sprites must not run into the HUD, which the
	// copper has already switched down to HUD_BITPLANES by then.
	const int lineTop = DIW_Y0;
	const int lineBot = DIW_Y0 + VIEW_H;

	UWORD* write[SPR_CHANNELS];      // where the next entry goes in each chain
	int    freeFrom[SPR_CHANNELS];   // first raster line each channel is available again
	for (int c = 0; c < SPR_CHANNELS; c++) {
		write[c]    = buf + c * SPR_CHAIN_WORDS;
		freeFrom[c] = 0;
	}

	// Entities in ascending screen y. A chain is fetched top to bottom, so entries have to be
	// appended in that order; insertion sort because n is at most MAX_BULLETS and nearly
	// sorted frame to frame.
	UBYTE order[MAX_BULLETS];
	int   count = 0;
	for (int i = 0; i < n && count < MAX_BULLETS; i++) {
		placed[i] = 0;
		int j = count++;
		while (j > 0 && ents[order[j - 1]].y > ents[i].y) { order[j] = order[j - 1]; j--; }
		order[j] = (UBYTE)i;
	}

	int done = 0;
	for (int k = 0; k < count; k++) {
		const int i  = order[k];
		const int sx = ents[i].x - camX;
		const int sy = ents[i].y - camY;

		// Off the sides or past the play area: nothing to draw, by sprite or by bob.
		if (sx <= -BULLET_W || sx >= SCREEN_W || sy <= -BULLET_H || sy >= VIEW_H) {
			placed[i] = 1;
			continue;
		}

		// Vertical clip. A bullet crossing the top of the play area starts part way down its
		// image, so the data pointer moves with vstart.
		int vstart  = sy + DIW_Y0;
		int vstop   = vstart + BULLET_H;
		int rowSkip = 0;
		if (vstart < lineTop) { rowSkip = lineTop - vstart; vstart = lineTop; }
		if (vstop > lineBot)  vstop = lineBot;
		const int rows = vstop - vstart;
		if (rows <= 0) { placed[i] = 1; continue; }

		// First channel already finished above this bullet. Chains are built in y order, so a
		// channel is reusable as soon as its previous entry has stopped.
		int ch = -1;
		for (int c = 0; c < SPR_CHANNELS; c++) {
			if (freeFrom[c] <= vstart) { ch = c; break; }
		}
		if (ch < 0) continue;   // no channel free in this band; placed stays 0 -> caller bobs it

		const int    hstart = sx + DIW_X0;
		const UWORD* img    = sheet + ents[i].dir * SPR_IMAGE_WORDS + rowSkip * 2;
		UWORD*       w      = write[ch];

		writeControl(w, hstart, vstart, vstop);
		w += 2;
		for (int r = 0; r < rows; r++) {
			*w++ = img[r * 2];
			*w++ = img[r * 2 + 1];
		}

		write[ch]    = w;
		freeFrom[ch] = vstop;
		placed[i]    = 1;
		done++;
	}

	// Terminate every chain, and point the channels bullets do not use at a bare terminator.
	UWORD* const nullChain = buf + SPR_CHANNELS * SPR_CHAIN_WORDS;
	nullChain[0] = 0;
	nullChain[1] = 0;
	for (int c = 0; c < SPR_CHANNELS; c++) {
		write[c][0] = 0;
		write[c][1] = 0;
	}
	for (int c = 0; c < SPR_TOTAL; c++) {
		chainStart[c] = (c >= SPR_FIRST_CHANNEL && c < SPR_FIRST_CHANNEL + SPR_CHANNELS)
		              ? buf + (c - SPR_FIRST_CHANNEL) * SPR_CHAIN_WORDS
		              : nullChain;
	}

	return done;
}

// --- procedural sprite art ------------------------------------------------------------------

// Colour 0 is transparent; 1..3 are the channel's colour triple -- registers 29, 30 and 31,
// shared by sprite channels 6 and 7. See the palette in tiles.cpp.
static void setSpritePixel(UWORD* img, int x, int y, int color) {
	if (x < 0 || x >= BULLET_W || y < 0 || y >= BULLET_H) return;
	const UWORD bit = (UWORD)(0x8000 >> x);
	if (color & 1) img[y * 2]     |= bit; else img[y * 2]     &= (UWORD)~bit;
	if (color & 2) img[y * 2 + 1] |= bit; else img[y * 2 + 1] &= (UWORD)~bit;
}

static void sprDisc(UWORD* img, int cx, int cy, int r, int color) {
	const int r2 = r * r;
	for (int y = cy - r; y <= cy + r; y++)
		for (int x = cx - r; x <= cx + r; x++) {
			const int dx = x - cx, dy = y - cy;
			if (dx * dx + dy * dy <= r2) setSpritePixel(img, x, y, color);
		}
}

static void sprStroke(UWORD* img, int x0, int y0, int x1, int y1, int r, int color) {
	const int dx = x1 - x0, dy = y1 - y0;
	int steps = (dx < 0 ? -dx : dx);
	const int ady = (dy < 0 ? -dy : dy);
	if (ady > steps) steps = ady;
	if (steps == 0) steps = 1;
	for (int s = 0; s <= steps; s++)
		sprDisc(img, x0 + dx * s / steps, y0 + dy * s / steps, r, color);
}

// The same shot as the bob version: brass tail streaking back, cyan core, white head -- but
// three colours instead of five, because that is all a sprite carries.
void generateBulletSprites(UWORD* sheet) {
	for (int i = 0; i < SPR_SHEET_WORDS; i++) sheet[i] = 0;

	for (int d = 0; d < BOB_DIRECTIONS; d++) {
		UWORD* const img = sheet + d * SPR_IMAGE_WORDS;
		const int vx = dirX[d], vy = dirY[d];
		const int cx = 8, cy = 8;

		sprStroke(img, cx - vx * 6 / 16, cy - vy * 6 / 16, cx, cy, 1, 1);
		sprStroke(img, cx - vx * 3 / 16, cy - vy * 3 / 16,
		               cx + vx * 3 / 16, cy + vy * 3 / 16, 1, 2);
		sprDisc(img, cx + vx * 4 / 16, cy + vy * 4 / 16, 1, 3);
	}
}
