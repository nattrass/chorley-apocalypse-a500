// Chorley Apocalypse - Milestone 1: 4-way Scrolling Tilemap Engine (50Hz)
#define MUSIC

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include "system.h"
#include "copper.h"
#include "music.h"
#include "tiles.h"
#include "map.h"
#include "display.h"
#include "keyboard.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

// Global variables
struct ExecBase *SysBase;
volatile struct Custom *custom;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

// Frame counter for profiling
volatile short frameCounter = 0;

__attribute__((always_inline)) inline short MouseLeft() {
	return !((*(volatile UBYTE*)0xbfe001) & 64);
}

static __attribute__((interrupt)) void interruptHandler() {
	// Acknowledge VBL only. ThePlayer owns EXTER for audio-DMA timing.
	custom->intreq = (1 << INTB_VERTB);
	custom->intreq = (1 << INTB_VERTB); // reset vbl req twice for a4000 bug

#ifdef MUSIC
	p61Music();
#endif

	frameCounter++;
}

int main() {
	SysBase = *((struct ExecBase**)4UL);
	custom = (struct Custom*)0xdff000;

	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
	if (!GfxBase) Exit(0);

	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase) Exit(0);

	// Start coordinates in the Flat Iron Market area
	int camX = 128;
	int camY = 128;
	int lastTileX = camX / TILE_SIZE;
	int lastTileY = camY / TILE_SIZE;

	// Pointers for dynamically allocated buffers in Chip & Slow RAM
	UBYTE* playfieldBuffer = NULL;
	UBYTE* tileSheet = NULL;
	UBYTE* hudBuffer = NULL;
	USHORT* copperLists[2] = { NULL, NULL };
	UBYTE* mapData = NULL;

	warpmode(1);

	// 1. Allocate Chip RAM buffers
	playfieldBuffer = (UBYTE*)AllocMem(PLAYFIELD_BYTES, MEMF_CHIP | MEMF_CLEAR);
	tileSheet       = (UBYTE*)AllocMem(TILESHEET_BYTES, MEMF_CHIP | MEMF_CLEAR);
	hudBuffer       = (UBYTE*)AllocMem(HUD_BYTES, MEMF_CHIP | MEMF_CLEAR);
	copperLists[0]  = (USHORT*)AllocMem(1024, MEMF_CHIP | MEMF_CLEAR);
	copperLists[1]  = (USHORT*)AllocMem(1024, MEMF_CHIP | MEMF_CLEAR);

	// 2. Allocate Slow/Fast RAM for 128x128 map
	mapData = (UBYTE*)AllocMem(MAP_BYTES, MEMF_PUBLIC | MEMF_CLEAR);

	if (!playfieldBuffer || !tileSheet || !hudBuffer || !copperLists[0] || !copperLists[1] || !mapData) {
		KPrintF("Memory allocation failed!\n");
		warpmode(0);
		Exit(0);
	}

	// 3. Procedural asset and map generation during warpmode (CPU only)
	generateTileSheet(tileSheet);
	generateMap(mapData);
	initHUD(hudBuffer);

#ifdef MUSIC
	if (p61Init(module) != 0) {
		KPrintF("p61Init failed!\n");
	}
#endif

	warpmode(0);

	// 4. Take over system from AmigaOS
	TakeSystem();
	WaitVbl();
	initKeyboard();

	// Enable Blitter DMA so initPlayfield can blit
	custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_BLITTER;

	// 5. Blitter initialization (fill initial double-wide playfield view)
	initPlayfield(playfieldBuffer, tileSheet, mapData, camX, camY);

	// Build initial copper lists
	buildCopperList(copperLists[0], camX, camY, playfieldBuffer, hudBuffer);
	buildCopperList(copperLists[1], camX, camY, playfieldBuffer, hudBuffer);

	// Initialize Copper and Raster DMA
	int copperIdx = 0;
	custom->cop1lc = (ULONG)copperLists[copperIdx];
	custom->copjmp1 = 0x7fff; // start copper
	custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER;

	// Install VBL interrupt handler
	SetInterruptHandler((APTR)interruptHandler);
#ifdef MUSIC
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_EXTER;
#else
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB;
#endif

	custom->intreq = 1 << INTB_VERTB;

	const int maxCamX = MAP_W * TILE_SIZE - SCREEN_W; // 2048 - 320 = 1728
	const int maxCamY = MAP_H * TILE_SIZE - VIEW_H;   // 2048 - 208 = 1840

	int idleCounter = 0;
	int autoAngle = 0;

	// 6. Main 50Hz Game Loop
	while (!MouseLeft() && !keyEscPressed()) {
		// Wait for raster line 0x10 to sync frame
		WaitLine(0x10);

		// Poll Keyboard (CIA-A Serial Port)
		pollKeyboard();

		// Read Joystick 1 (Port 2 on standard Amiga)
		UWORD joy = custom->joy1dat;
		bool joyRight = (joy & 0x0002) != 0;
		bool joyLeft  = (joy & 0x0200) != 0;
		bool joyDown  = (((joy >> 1) ^ joy) & 0x0001) != 0;
		bool joyUp    = (((joy >> 1) ^ joy) & 0x0100) != 0;
		bool joyFire  = !((*(volatile UBYTE*)0xbfe001) & 0x80);

		// Combine Joystick + Keyboard (Arrows / WASD / NumPad / Space)
		bool moveRight = joyRight || keyRightHeld();
		bool moveLeft  = joyLeft  || keyLeftHeld();
		bool moveDown  = joyDown  || keyDownHeld();
		bool moveUp    = joyUp    || keyUpHeld();
		bool speedBoost = joyFire || keyFireHeld();

		int speed = speedBoost ? 4 : 2; // Fast scroll when holding fire / space / shift

		if (moveRight || moveLeft || moveDown || moveUp) {
			idleCounter = 0;
			if (moveRight) camX += speed;
			if (moveLeft)  camX -= speed;
			if (moveDown)  camY += speed;
			if (moveUp)    camY -= speed;
		} else {
			idleCounter++;
			// Automated scenic tour across all 5 zones if no input
			if (idleCounter > 50) {
				autoAngle = (autoAngle + 1) & 511;
				// Smooth diagonal wandering trajectory
				if ((autoAngle >> 7) & 1) camX += 2; else camX -= 2;
				if ((autoAngle >> 6) & 1) camY += 2; else camY -= 2;
			}
		}

		// Clamp camera to map bounds
		if (camX < 0) camX = 0;
		if (camX > maxCamX) camX = maxCamX;
		if (camY < 0) camY = 0;
		if (camY > maxCamY) camY = maxCamY;

		// Check if camera crossed tile boundaries and blit new seams
		int curTileX = camX / TILE_SIZE;
		int curTileY = camY / TILE_SIZE;

		if (curTileX != lastTileX || curTileY != lastTileY) {
			updateTileSeams(lastTileX, lastTileY, curTileX, curTileY, mapData, tileSheet, playfieldBuffer);
			lastTileX = curTileX;
			lastTileY = curTileY;
		}

		// Build copper list for current frame in the back-buffer copper list
		copperIdx ^= 1;
		buildCopperList(copperLists[copperIdx], camX, camY, playfieldBuffer, hudBuffer);
		custom->cop1lc = (ULONG)copperLists[copperIdx];
	}

	// 7. Cleanup and exit cleanly
#ifdef MUSIC
	p61End();
#endif

	FreeMem(playfieldBuffer, PLAYFIELD_BYTES);
	FreeMem(tileSheet, TILESHEET_BYTES);
	FreeMem(hudBuffer, HUD_BYTES);
	FreeMem(copperLists[0], 1024);
	FreeMem(copperLists[1], 1024);
	FreeMem(mapData, MAP_BYTES);

	FreeSystem();

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);

	return 0;
}
