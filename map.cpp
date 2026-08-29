#include "map.h"

void generateMap(UBYTE* map) {
	// Base layer: Cobblestone pavement across entire map
	for (int y = 0; y < MAP_H; y++) {
		for (int x = 0; x < MAP_W; x++) {
			map[y * MAP_W + x] = ((x + y) & 1) ? 0 : 1;
		}
	}

	// 1. Outer boundary walls (solid perimeter)
	for (int i = 0; i < MAP_W; i++) {
		map[0 * MAP_W + i] = 4; // Top wall
		map[1 * MAP_W + i] = 2;
		map[(MAP_H - 2) * MAP_W + i] = 2;
		map[(MAP_H - 1) * MAP_W + i] = 4; // Bottom wall
	}
	for (int i = 0; i < MAP_H; i++) {
		map[i * MAP_W + 0] = 4; // Left wall
		map[i * MAP_W + 1] = 2;
		map[i * MAP_W + (MAP_W - 2)] = 2;
		map[i * MAP_W + (MAP_W - 1)] = 4; // Right wall
	}

	// 2. Zone 1: Flat Iron Market Square (Rows 4..38, Cols 4..50)
	// Stalls arranged in market rows
	for (int row = 8; row <= 34; row += 6) {
		for (int col = 8; col <= 46; col += 6) {
			// 2x2 Market stall cluster
			map[row * MAP_W + col] = 8;
			map[row * MAP_W + (col + 1)] = 9;
			map[(row + 1) * MAP_W + col] = 8;
			map[(row + 1) * MAP_W + (col + 1)] = 9;

			// Hazard markings around stalls
			map[(row - 1) * MAP_W + col] = 14;
			map[(row + 2) * MAP_W + col] = 14;
		}
	}
	// St Laurence parish archway & stone monument in plaza
	map[6 * MAP_W + 26] = 26; map[6 * MAP_W + 27] = 27;
	map[7 * MAP_W + 26] = 4;  map[7 * MAP_W + 27] = 4;
	map[8 * MAP_W + 26] = 5;  map[8 * MAP_W + 27] = 5;
	map[10 * MAP_W + 26] = 30; // Inactive node switch

	// 3. Zone 2: Leeds & Liverpool Canal (Rows 42..54 across width)
	for (int y = 42; y <= 54; y++) {
		for (int x = 2; x < MAP_W - 2; x++) {
			if (y == 42 || y == 54) {
				map[y * MAP_W + x] = 5; // Stone canal wall / towpath edge
			} else if (y == 43 || y == 53) {
				map[y * MAP_W + x] = 12; // Towpath grass
			} else {
				map[y * MAP_W + x] = ((x + y) % 3 == 0) ? 10 : 11; // Canal water
			}
		}
	}

	// Canal Bridges at Cols 18..24, Cols 56..62, Cols 94..100
	const int bridgeCols[3] = { 20, 58, 96 };
	for (int b = 0; b < 3; b++) {
		int bc = bridgeCols[b];
		for (int y = 42; y <= 54; y++) {
			for (int x = bc - 2; x <= bc + 2; x++) {
				if (x == bc - 2 || x == bc + 2) {
					map[y * MAP_W + x] = 14; // Hazard bridge barrier
				} else {
					map[y * MAP_W + x] = 6; // Metal bridge decking
				}
			}
		}
	}

	// 4. Zone 3: Botany Bay Mill & Boiler Complex (Rows 58..92, Cols 4..68)
	// Outer Mill Walls
	for (int x = 6; x <= 66; x++) {
		map[58 * MAP_W + x] = (x % 5 == 0) ? 3 : 2; // North mill wall
		map[92 * MAP_W + x] = 2;                     // South mill wall
	}
	for (int y = 58; y <= 92; y++) {
		map[y * MAP_W + 6] = 2;                      // West mill wall
		map[y * MAP_W + 66] = (y % 6 == 0) ? 3 : 2; // East mill wall
	}

	// Mill Interior: Metal plate flooring
	for (int y = 59; y < 92; y++) {
		for (int x = 7; x < 66; x++) {
			map[y * MAP_W + x] = ((x + y) & 2) ? 6 : 7;
		}
	}

	// Boiler house & machinery rooms inside mill
	for (int x = 12; x <= 28; x += 4) {
		for (int y = 64; y <= 76; y += 4) {
			map[y * MAP_W + x] = 21; // Riveted boiler
			map[y * MAP_W + (x + 1)] = 16; // Brass engine
			map[(y + 1) * MAP_W + x] = 17; // Cogs
			map[(y + 1) * MAP_W + (x + 1)] = 22; // Steam vent
		}
	}

	// Steam Pipe Network running through mill
	for (int x = 8; x <= 64; x++) {
		map[80 * MAP_W + x] = 18; // Horizontal pipe
	}
	for (int y = 60; y <= 90; y++) {
		map[y * MAP_W + 36] = 19; // Vertical pipe
	}
	map[80 * MAP_W + 36] = 20; // Pipe junction

	// Mill doors and entrances
	map[58 * MAP_W + 20] = 6; map[58 * MAP_W + 21] = 6; // North entrance
	map[92 * MAP_W + 36] = 6; map[92 * MAP_W + 37] = 6; // South exit
	map[72 * MAP_W + 50] = 31; // Active node beacon inside mill!

	// 5. Zone 4: Vertical Railway Corridor (Cols 76..86, Rows 4..124)
	for (int y = 4; y < 124; y++) {
		for (int x = 76; x <= 86; x++) {
			map[y * MAP_W + x] = 23; // Ballast gravel
		}
		map[y * MAP_W + 79] = 25; // Track 1
		map[y * MAP_W + 83] = 25; // Track 2
	}

	// 6. Zone 5: Moor & Winter Hill Approach (Rows 95..124, Cols 4..124)
	for (int y = 95; y < 124; y++) {
		for (int x = 4; x < 124; x++) {
			if (x >= 76 && x <= 86) continue; // Keep railway clear

			int noise = (x * 19 + y * 37) & 7;
			if (noise < 3) {
				map[y * MAP_W + x] = 12; // Towpath turf / heather
			} else if (noise < 6) {
				map[y * MAP_W + x] = 13; // Dark bog mud
			} else {
				map[y * MAP_W + x] = 11; // Standing bog water pool
			}
		}
	}

	// Winter Hill Transmitter Mast Base & Broadcast Signal Grid (Rows 105..118, Cols 95..118)
	for (int y = 105; y <= 118; y++) {
		for (int x = 95; x <= 118; x++) {
			map[y * MAP_W + x] = ((x + y) & 1) ? 28 : 29; // Glowing broadcast circuit floor
		}
	}
	// Mast Pillars
	map[105 * MAP_W + 95] = 4;
	map[105 * MAP_W + 118] = 4;
	map[118 * MAP_W + 95] = 4;
	map[118 * MAP_W + 118] = 4;
	map[111 * MAP_W + 106] = 31; // Master broadcast node beacon!
}
