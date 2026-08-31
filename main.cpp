// Chorley Apocalypse - Milestone 2: the player over the scrolling tilemap (50Hz)
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
#include "bob.h"
#include "player.h"

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

#define KEY_T 0x14

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

	// Start in the Flat Iron market square
	const int startX = 320;
	const int startY = 320;
	int camX = startX + BOB_W / 2 - SCREEN_W / 2;
	int camY = startY + BOB_H / 2 - VIEW_H / 2;
	if (camX < 0) camX = 0;
	if (camY < 0) camY = 0;
	int lastTileX = camX / TILE_SIZE;
	int lastTileY = camY / TILE_SIZE;

	// Pointers for dynamically allocated buffers in Chip & Slow RAM
	UBYTE* playfieldBuffer = NULL;
	UBYTE* tileSheet = NULL;
	UBYTE* hudBuffer = NULL;
	UBYTE* playerSheet = NULL;
	UBYTE* bulletSheet = NULL;
	USHORT* copperLists[2] = { NULL, NULL };
	UBYTE* mapData = NULL;

	warpmode(1);

	// 1. Allocate Chip RAM buffers. Everything the blitter or copper touches lives here.
	playfieldBuffer = (UBYTE*)AllocMem(PLAYFIELD_BYTES, MEMF_CHIP | MEMF_CLEAR);
	tileSheet       = (UBYTE*)AllocMem(TILESHEET_BYTES, MEMF_CHIP | MEMF_CLEAR);
	hudBuffer       = (UBYTE*)AllocMem(HUD_BYTES, MEMF_CHIP | MEMF_CLEAR);
	playerSheet     = (UBYTE*)AllocMem(CLASS_BOB_BYTES, MEMF_CHIP | MEMF_CLEAR);
	bulletSheet     = (UBYTE*)AllocMem(BULLET_SHEET_BYTES, MEMF_CHIP | MEMF_CLEAR);
	copperLists[0]  = (USHORT*)AllocMem(1024, MEMF_CHIP | MEMF_CLEAR);
	copperLists[1]  = (USHORT*)AllocMem(1024, MEMF_CHIP | MEMF_CLEAR);

	// 2. Allocate Slow/Fast RAM for 128x128 map
	mapData = (UBYTE*)AllocMem(MAP_BYTES, MEMF_PUBLIC | MEMF_CLEAR);

	if (!playfieldBuffer || !tileSheet || !hudBuffer || !playerSheet || !bulletSheet ||
	    !copperLists[0] || !copperLists[1] || !mapData) {
		KPrintF("Memory allocation failed!\n");
		warpmode(0);
		Exit(0);
	}

	// 3. Procedural asset and map generation during warpmode (CPU only)
	generateTileSheet(tileSheet);
	generateMap(mapData);
	generatePlayerBobs(playerSheet);
	generateBulletBobs(bulletSheet);
	initHUD(hudBuffer);
	entitiesInit(startX, startY);

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
	RenderCtx ctx;
	ctx.playfield = playfieldBuffer;
	ctx.tileSheet = tileSheet;
	ctx.map       = mapData;
	ctx.camX      = camX;
	ctx.camY      = camY;

	initPlayfield(playfieldBuffer, tileSheet, mapData, camX, camY);

	// Build initial copper lists
	buildCopperList(copperLists[0], camX, camY, playfieldBuffer, hudBuffer);
	buildCopperList(copperLists[1], camX, camY, playfieldBuffer, hudBuffer);

	debug_register_bitmap(playfieldBuffer, "playfield.bpl", PLAYFIELD_W, PLAYFIELD_H, BITPLANES, debug_resource_bitmap_interleaved);
	debug_register_palette(gamePalette, "playfield.pal", 32, 0);
	debug_register_copperlist(copperLists[0], "copper1", 1024, 0);
	// Width is the stored width, guard word included, or the debugger skews every row.
	debug_register_bitmap(playerSheet, "player.bpl", BOB_WORDS * 16,
	                      BOB_H * BOB_DIRECTIONS * BOB_ANIM_FRAMES, BITPLANES,
	                      debug_resource_bitmap_interleaved | debug_resource_bitmap_masked);

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

	bool stressKeyPrev = false;
	int  liveBobs = 0;
	int  peakLine = 0;
	int  peakBobs = 0;

	// 6. Main 50Hz Game Loop
	while (!MouseLeft() && !keyEscPressed()) {
		// Sync to the top of the frame, above the display window, and stay ahead of the beam.
		// The playfield is single-buffered, so every bob blit from here on is racing the raster
		// down the screen -- that race is what the M2 gate is judging.
		WaitLine(0x10);

		// 6a. Put the background back under last frame's bobs, before anything moves. The
		// records carry their own buffer slots, so this is independent of where the camera is.
		entitiesRestore(&ctx);

		// 6b. Input
		pollKeyboard();

		UWORD joy = custom->joy1dat;
		PlayerInput in;
		in.right = ((joy & 0x0002) != 0)                  || keyRightHeld();
		in.left  = ((joy & 0x0200) != 0)                  || keyLeftHeld();
		in.down  = ((((joy >> 1) ^ joy) & 0x0001) != 0)   || keyDownHeld();
		in.up    = ((((joy >> 1) ^ joy) & 0x0100) != 0)   || keyUpHeld();
		in.fire  = !((*(volatile UBYTE*)0xbfe001) & 0x80) || keyFireHeld();

		const bool stressKey = isKeyDown(KEY_T);
		if (stressKey && !stressKeyPrev) entitiesToggleStress();
		stressKeyPrev = stressKey;

		// 6c. Move the world
		entitiesUpdate(&in, frameCounter);
		cameraFollow(&camX, &camY);
		ctx.camX = camX;
		ctx.camY = camY;

		// 6d. Blit whatever tile columns and rows the camera just uncovered
		const int curTileX = camX / TILE_SIZE;
		const int curTileY = camY / TILE_SIZE;
		if (curTileX != lastTileX || curTileY != lastTileY) {
			updateTileSeams(lastTileX, lastTileY, curTileX, curTileY, mapData, tileSheet, playfieldBuffer);
			lastTileX = curTileX;
			lastTileY = curTileY;
		}

		// 6e. Draw the bobs and remember what to restore next frame
		liveBobs = entitiesDraw(&ctx, playerSheet, bulletSheet, frameCounter);

		// 6f. Hand the new camera position to the copper
		copperIdx ^= 1;
		buildCopperList(copperLists[copperIdx], camX, camY, playfieldBuffer, hudBuffer);
		custom->cop1lc = (ULONG)copperLists[copperIdx];

		// How far down the frame all of that got, blitter included. Past 312 is a dropped frame.
		// Sampled every frame and held at the worst of the last 16, because a peak that only
		// happens on a tile crossing is exactly the one worth seeing. Redrawing the digits is
		// per-pixel CPU work on the HUD bitmap, far too expensive to do at 50Hz, and it would
		// distort the number it is reporting.
		WaitBlt();
		const int rasterLine = (int)((*(volatile ULONG*)0xdff004 >> 8) & 511);
		if (rasterLine > peakLine) peakLine = rasterLine;
		if (liveBobs > peakBobs)  peakBobs = liveBobs;
		if ((frameCounter & 15) == 0) {
			hudSetCounters(hudBuffer, peakBobs, peakLine);
			peakLine = 0;
			peakBobs = 0;
		}
	}

	// 7. Cleanup and exit cleanly
#ifdef MUSIC
	p61End();
#endif

	FreeMem(playfieldBuffer, PLAYFIELD_BYTES);
	FreeMem(tileSheet, TILESHEET_BYTES);
	FreeMem(hudBuffer, HUD_BYTES);
	FreeMem(playerSheet, CLASS_BOB_BYTES);
	FreeMem(bulletSheet, BULLET_SHEET_BYTES);
	FreeMem(copperLists[0], 1024);
	FreeMem(copperLists[1], 1024);
	FreeMem(mapData, MAP_BYTES);

	FreeSystem();

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);

	return 0;
}
