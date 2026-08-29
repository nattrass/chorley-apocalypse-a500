#include "tiles.h"

// 32 OCS colors matching DESIGN.md Section 8
// Greys (8), Brick & Rust (6), Moor Greens & Water (6), Brass (4), Signal (4), UI (4)
const UWORD gamePalette[32] = {
	0x0112, // 0: Dark night/charcoal
	0x0223, // 1: Dark slate
	0x0445, // 2: Shadow stone
	0x0667, // 3: Mid stone grey
	0x0889, // 4: Light stone grey
	0x0aab, // 5: Pale stone
	0x0ccd, // 6: Highlight grey
	0x0eee, // 7: Bright white/grey
	0x0311, // 8: Dark terracotta
	0x0621, // 9: Dark brick red
	0x0932, // 10: Medium brick red
	0x0c53, // 11: Warm brick orange
	0x0742, // 12: Dark rust iron
	0x0a63, // 13: Light rust orange
	0x0121, // 14: Dark bog green
	0x0242, // 15: Moor grass green
	0x0363, // 16: Moss green
	0x0584, // 17: Olive field green
	0x0134, // 18: Dark canal water
	0x0267, // 19: Canal water teal
	0x0541, // 20: Tarnished brass
	0x0862, // 21: Mid brass
	0x0ba3, // 22: Bright polished brass
	0x0ed5, // 23: Brass highlight
	0x0047, // 24: Deep signal cyan
	0x00cf, // 25: Vibrant broadcast cyan
	0x0705, // 26: Deep signal magenta
	0x0e1b, // 27: Vibrant neon magenta
	0x0b22, // 28: Health / danger red
	0x0eb2, // 29: Gold / pickup yellow
	0x039e, // 30: Shield / power blue
	0x0fff  // 31: Pure white text
};

// 3x5 font for numbers 0-9
static const UBYTE font3x5[10][5] = {
	{ 0b111, 0b101, 0b101, 0b101, 0b111 }, // 0
	{ 0b010, 0b110, 0b010, 0b010, 0b111 }, // 1
	{ 0b111, 0b001, 0b111, 0b100, 0b111 }, // 2
	{ 0b111, 0b001, 0b111, 0b001, 0b111 }, // 3
	{ 0b101, 0b101, 0b111, 0b001, 0b001 }, // 4
	{ 0b111, 0b100, 0b111, 0b001, 0b111 }, // 5
	{ 0b111, 0b100, 0b111, 0b101, 0b111 }, // 6
	{ 0b111, 0b001, 0b010, 0b010, 0b010 }, // 7
	{ 0b111, 0b101, 0b111, 0b101, 0b111 }, // 8
	{ 0b111, 0b101, 0b111, 0b001, 0b111 }  // 9
};

// 8x8 font for ASCII 32..90 (Space .. 'Z')
static const UBYTE font8x8[59][8] = {
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, // 32 ' '
	{ 0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00 }, // 33 '!'
	{ 0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00 }, // 34 '"'
	{ 0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00 }, // 35 '#'
	{ 0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00 }, // 36 '$'
	{ 0x00,0x63,0x66,0x0c,0x18,0x33,0x63,0x00 }, // 37 '%'
	{ 0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0x00 }, // 38 '&'
	{ 0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00 }, // 39 '''
	{ 0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00 }, // 40 '('
	{ 0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00 }, // 41 ')'
	{ 0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00 }, // 42 '*'
	{ 0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00 }, // 43 '+'
	{ 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30 }, // 44 ','
	{ 0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00 }, // 45 '-'
	{ 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00 }, // 46 '.'
	{ 0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00 }, // 47 '/'
	{ 0x3c,0x66,0x6e,0x76,0x66,0x66,0x3c,0x00 }, // 48 '0'
	{ 0x18,0x38,0x18,0x18,0x18,0x18,0x7e,0x00 }, // 49 '1'
	{ 0x3c,0x66,0x06,0x0c,0x18,0x30,0x7e,0x00 }, // 50 '2'
	{ 0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0x00 }, // 51 '3'
	{ 0x0c,0x1c,0x34,0x64,0x7e,0x04,0x04,0x00 }, // 52 '4'
	{ 0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0x00 }, // 53 '5'
	{ 0x1c,0x30,0x60,0x7c,0x66,0x66,0x3c,0x00 }, // 54 '6'
	{ 0x7e,0x06,0x0c,0x18,0x30,0x30,0x30,0x00 }, // 55 '7'
	{ 0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00 }, // 56 '8'
	{ 0x3c,0x66,0x66,0x3e,0x06,0x0c,0x38,0x00 }, // 57 '9'
	{ 0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00 }, // 58 ':'
	{ 0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30 }, // 59 ';'
	{ 0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x00 }, // 60 '<'
	{ 0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00 }, // 61 '='
	{ 0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00 }, // 62 '>'
	{ 0x3c,0x66,0x0c,0x18,0x18,0x00,0x18,0x00 }, // 63 '?'
	{ 0x3c,0x66,0x6e,0x6e,0x60,0x62,0x3c,0x00 }, // 64 '@'
	{ 0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00 }, // 65 'A'
	{ 0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00 }, // 66 'B'
	{ 0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0x00 }, // 67 'C'
	{ 0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00 }, // 68 'D'
	{ 0x7e,0x60,0x60,0x7c,0x60,0x60,0x7e,0x00 }, // 69 'E'
	{ 0x7e,0x60,0x60,0x7c,0x60,0x60,0x60,0x00 }, // 70 'F'
	{ 0x3c,0x66,0x60,0x6e,0x66,0x66,0x3a,0x00 }, // 71 'G'
	{ 0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00 }, // 72 'H'
	{ 0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00 }, // 73 'I'
	{ 0x0e,0x06,0x06,0x06,0x66,0x66,0x3c,0x00 }, // 74 'J'
	{ 0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00 }, // 75 'K'
	{ 0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0x00 }, // 76 'L'
	{ 0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00 }, // 77 'M'
	{ 0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0x00 }, // 78 'N'
	{ 0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00 }, // 79 'O'
	{ 0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00 }, // 80 'P'
	{ 0x3c,0x66,0x66,0x66,0x6e,0x3c,0x0e,0x00 }, // 81 'Q'
	{ 0x7c,0x66,0x66,0x7c,0x78,0x6c,0x66,0x00 }, // 82 'R'
	{ 0x3c,0x66,0x60,0x3c,0x06,0x66,0x3c,0x00 }, // 83 'S'
	{ 0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00 }, // 84 'T'
	{ 0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00 }, // 85 'U'
	{ 0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00 }, // 86 'V'
	{ 0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00 }, // 87 'W'
	{ 0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00 }, // 88 'X'
	{ 0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00 }, // 89 'Y'
	{ 0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00 }  // 90 'Z'
};

static void setTilePixel(UBYTE* tileSheet, int tileIdx, int x, int y, int colorIndex) {
	// Each tile has 16 rows. Each row has 5 plane words (10 bytes).
	UBYTE* rowPtr = tileSheet + tileIdx * TILE_BYTES + y * (BITPLANES * 2);
	int byteOffset = x >> 3;
	UBYTE bitMask = 1 << (7 - (x & 7));

	for (int p = 0; p < BITPLANES; p++) {
		UBYTE* pByte = rowPtr + (p * 2) + byteOffset;
		if ((colorIndex >> p) & 1)
			*pByte |= bitMask;
		else
			*pByte &= ~bitMask;
	}
}

void drawTextPlanar(UBYTE* buffer, int lineBytes, int numPlanes, int startX, int startY, const char* text, int color) {
	int cx = startX;
	int cy = startY;

	while (*text) {
		char c = *text++;
		if (c == '\n') {
			cx = startX;
			cy += 8;
			continue;
		}
		if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
		if (c < 32 || c > 90) c = ' ';

		const UBYTE* glyph = font8x8[c - 32];
		for (int row = 0; row < 8; row++) {
			UBYTE bits = glyph[row];
			UBYTE* linePtr = buffer + (cy + row) * lineBytes;
			for (int col = 0; col < 8; col++) {
				if (bits & (0x80 >> col)) {
					int px = cx + col;
					int byteOff = px >> 3;
					UBYTE mask = 1 << (7 - (px & 7));
					for (int p = 0; p < numPlanes; p++) {
						UBYTE* pByte = linePtr + (p * (lineBytes / numPlanes)) + byteOff;
						if ((color >> p) & 1)
							*pByte |= mask;
						else
							*pByte &= ~mask;
					}
				}
			}
		}
		cx += 8;
	}
}

static void drawDigit3x5(UBYTE* tileSheet, int tileIdx, int digit, int ox, int oy, int color) {
	if (digit < 0 || digit > 9) return;
	for (int y = 0; y < 5; y++) {
		UBYTE row = font3x5[digit][y];
		for (int x = 0; x < 3; x++) {
			if (row & (1 << (2 - x))) {
				setTilePixel(tileSheet, tileIdx, ox + x, oy + y, color);
			}
		}
	}
}

void generateTileSheet(UBYTE* tileSheet) {
	memclr(tileSheet, TILESHEET_BYTES);

	for (int t = 0; t < 32; t++) {
		// 1. Fill base texture according to tile type
		for (int y = 0; y < 16; y++) {
			for (int x = 0; x < 16; x++) {
				int c = 1; // default dark slate

				switch (t) {
				case 0: // Cobblestone 1 (Flat Iron street)
					c = ((x + (y & ~3)) % 4 == 0 || y % 4 == 0) ? 2 : (((x ^ y) & 2) ? 4 : 3);
					break;
				case 1: // Cobblestone 2 (Dark alley)
					c = ((x + y) % 3 == 0) ? 1 : (((x * y) & 4) ? 3 : 2);
					break;
				case 2: // Red Brick Wall
					if (y % 4 == 0) c = 1; // mortar
					else {
						int rowShift = (y / 4) % 2 == 0 ? 0 : 4;
						c = ((x + rowShift) % 8 == 0) ? 1 : 10;
					}
					break;
				case 3: // Dark Brick with Vent
					if (y % 4 == 0) c = 1;
					else c = (y >= 6 && y <= 10 && x >= 4 && x <= 12) ? 0 : 9;
					break;
				case 4: // Stone Pillar / Ashlar Wall
					c = (x == 0 || x == 15 || y == 0 || y == 15) ? 2 : (((x & 7) == 0 || (y & 7) == 0) ? 3 : 5);
					break;
				case 5: // Stone Flagstones
					c = (x % 8 == 0 || y % 8 == 0) ? 2 : (((x + y) & 1) ? 4 : 5);
					break;
				case 6: // Diamond Metal Plate
					c = ((x + y) % 4 == 0 || (x - y + 16) % 4 == 0) ? 6 : 3;
					break;
				case 7: // Metal Floor Grate
					c = (x % 2 == 0 && y % 2 == 0) ? 0 : 4;
					break;
				case 8: // Market Stall Wood Planking
					c = (x % 4 == 0) ? 8 : (((y == 2 || y == 13) && x % 4 == 2) ? 12 : 11);
					break;
				case 9: // Dark Weathered Wood
					c = (x % 4 == 0) ? 0 : (((x + y * 2) & 3) ? 8 : 9);
					break;
				case 10: // Canal Water 1 (Ripples)
					c = (y % 4 == 0 && (x & 3) < 2) ? 19 : 18;
					break;
				case 11: // Deep Canal Water
					c = (y % 6 == 0 && (x & 7) < 3) ? 19 : 14;
					break;
				case 12: // Towpath Grass
					c = ((x * 7 + y * 13) & 3) ? 16 : 15;
					break;
				case 13: // Moor Mud / Bog
					c = ((x * 11 + y * 5) & 3) ? 14 : 15;
					break;
				case 14: // Hazard Stripes (Yellow / Black diagonal)
					c = ((x + y) % 6 < 3) ? 29 : 0;
					break;
				case 15: // Warning Border / Secondary Hazard
					c = ((x - y + 16) % 6 < 3) ? 28 : 12;
					break;
				case 16: // Polished Brass Panel
					c = (x == 1 || x == 14 || y == 1 || y == 14) ? 23 : 21;
					if ((x == 3 || x == 12) && (y == 3 || y == 12)) c = 0; // bolt
					break;
				case 17: // Cogs / Clockwork Gears
					{
						int dx = x - 7; int dy = y - 7;
						int dist2 = dx * dx + dy * dy;
						if (dist2 <= 4) c = 0;
						else if (dist2 <= 25) c = 22;
						else if ((x + y) % 3 == 0 && dist2 <= 45) c = 20;
						else c = 2;
					}
					break;
				case 18: // Horizontal Steam Pipe
					if (y >= 4 && y <= 11) c = (y == 5) ? 6 : (y == 10 ? 1 : 4);
					else c = 2;
					break;
				case 19: // Vertical Steam Pipe
					if (x >= 4 && x <= 11) c = (x == 5) ? 6 : (x == 10 ? 1 : 4);
					else c = 2;
					break;
				case 20: // Pipe Cross Junction
					if ((x >= 4 && x <= 11) || (y >= 4 && y <= 11)) c = 5;
					else c = 2;
					break;
				case 21: // Riveted Boiler Iron
					c = 12;
					if (x == 0 || y == 0) c = 13;
					if (x == 15 || y == 15) c = 8;
					if ((x == 2 || x == 13) && (y == 2 || y == 13)) c = 7;
					break;
				case 22: // Steam Vent Grate
					{
						int dx = x - 7; int dy = y - 7;
						int dist2 = dx * dx + dy * dy;
						if (dist2 <= 36) c = ((x + y) & 1) ? 0 : 13;
						else c = 3;
					}
					break;
				case 23: // Railway Ballast
					c = ((x * 17 + y * 29) & 3) == 0 ? 0 : (((x + y) & 2) ? 3 : 2);
					break;
				case 24: // Horizontal Railway Track
					if (y == 3 || y == 12) c = 7; // steel rail
					else if (x % 5 == 0) c = 8; // sleeper
					else c = 23; // ballast
					break;
				case 25: // Vertical Railway Track
					if (x == 3 || x == 12) c = 7;
					else if (y % 5 == 0) c = 8;
					else c = 23;
					break;
				case 26: // Stone Arch Left
					if (x >= (15 - y)) c = 5;
					else if (x >= (13 - y)) c = 7;
					else c = 0;
					break;
				case 27: // Stone Arch Right
					if (x <= y) c = 5;
					else if (x <= y + 2) c = 7;
					else c = 0;
					break;
				case 28: // Winter Hill Cyan Broadcast Circuit
					c = (x == 7 || y == 7 || (x == y && x > 3 && x < 12)) ? 25 : 0;
					break;
				case 29: // Winter Hill Magenta Broadcast Signal
					c = (x == 7 || y == 7 || ((x + y) == 15 && x > 3 && x < 12)) ? 27 : 0;
					break;
				case 30: // Inactive Node
					{
						int dx = x - 7; int dy = y - 7;
						int dist2 = dx * dx + dy * dy;
						if (dist2 <= 9) c = 2; // dark lamp
						else if (dist2 <= 25) c = 20; // brass rim
						else c = 6;
					}
					break;
				case 31: // Active Node (Glowing Beacon)
					{
						int dx = x - 7; int dy = y - 7;
						int dist2 = dx * dx + dy * dy;
						if (dist2 <= 4) c = 31; // bright white center
						else if (dist2 <= 16) c = 25; // glowing cyan aura
						else if (dist2 <= 25) c = 22; // brass mount
						else c = 6;
					}
					break;
				}

				setTilePixel(tileSheet, t, x, y, c);
			}
		}

		// 2. Distinct 1-pixel tile border (light top/left, dark bottom/right)
		// so wrap, seam, and alignment boundaries are unmistakable on screen
		for (int i = 0; i < 16; i++) {
			setTilePixel(tileSheet, t, i, 0, 7);  // Top highlight
			setTilePixel(tileSheet, t, 0, i, 7);  // Left highlight
			setTilePixel(tileSheet, t, i, 15, 0); // Bottom shadow
			setTilePixel(tileSheet, t, 15, i, 0); // Right shadow
		}

		// 3. Center badge box for 2-digit tile index
		for (int by = 4; by <= 10; by++) {
			for (int bx = 3; bx <= 12; bx++) {
				setTilePixel(tileSheet, t, bx, by, 0); // Solid black backing
			}
		}

		// 4. Draw 2-digit tile number (00..31) in high contrast white
		int tens = t / 10;
		int ones = t % 10;
		drawDigit3x5(tileSheet, t, tens, 4, 5, 31);
		drawDigit3x5(tileSheet, t, ones, 8, 5, 31);
	}
}
