//config
#define MUSIC

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include "system.h"
#include "copper.h"
#include "music.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

// Global variables needed by system.cpp
struct ExecBase *SysBase;
volatile struct Custom *custom;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

// Frame counter for profiling
volatile short frameCounter = 0;

// Asset data - placeholder palette
INCBIN(colors, "image.pal")

// Put copper2 into chip mem so we can use it without copying
const UWORD copper2[] __attribute__((section (".MEMF_CHIP"))) = {
	0xffff, 0xfffe // end copper list
};

__attribute__((always_inline)) inline short MouseLeft() {
	return !((*(volatile UBYTE*)0xbfe001) & 64);
}

static void Wait10() {
	WaitLine(0x10);
}

static __attribute__((interrupt)) void interruptHandler() {
	// This handler owns VBL only. P61 uses EXTER for audio-DMA timing, so do
	// not acknowledge it here or its level-6 player code will miss the event.
	custom->intreq = (1 << INTB_VERTB); custom->intreq = (1 << INTB_VERTB); // reset vbl req twice for a4000 bug

#ifdef MUSIC
	p61Music();
#endif

	// Increment frameCounter for profiling
	frameCounter++;
}

int main() {
	SysBase = *((struct ExecBase**)4UL);
	custom = (struct Custom*)0xdff000;

	// We will use the graphics library only to locate and restore the system copper list once we are through.
	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
	if (!GfxBase)
		Exit(0);

	// used for printing
	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase)
		Exit(0);

	warpmode(1);
	// Precalc stuff happens here during warpmode
#ifdef MUSIC
	if (p61Init(module) != 0)
		KPrintF("p61Init failed!\n");
#endif
	warpmode(0);

	TakeSystem();
	WaitVbl();

	USHORT* copper1 = (USHORT*)AllocMem(1024, MEMF_CHIP);
	USHORT* copPtr = copper1;

	// register graphics resources with WinUAE for nicer gfx debugger experience
	debug_register_palette(colors, "image.pal", 32, 0);
	debug_register_copperlist(copper1, "copper1", 1024, 0);
	debug_register_copperlist(copper2, "copper2", sizeof(copper2), 0);

	copPtr = screenScanDefault(copPtr);
	
	// enable bitplanes	
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0 << 10)/*dual pf*/ | (1 << 9)/*color*/ | ((5) << 12)/*num bitplanes*/;
	*copPtr++ = offsetof(struct Custom, bplcon1);	//scrolling
	*copPtr++ = 0; // scroll = 0 for now, black screen
	*copPtr++ = offsetof(struct Custom, bplcon2);	//playfied priority
	*copPtr++ = 1 << 6;//0x24;			//Sprites have priority over playfields

	const USHORT lineSize = 320 / 8;

	//set bitplane modulo
	*copPtr++ = offsetof(struct Custom, bpl1mod); //odd planes   1,3,5
	*copPtr++ = 4 * lineSize;
	*copPtr++ = offsetof(struct Custom, bpl2mod); //even  planes 2,4
	*copPtr++ = 4 * lineSize;

	// set bitplane pointers to NULL for black screen
	UBYTE* planes[5] = { 0, 0, 0, 0, 0 };
	copPtr = copSetPlanes(0, copPtr, (const UBYTE**)planes, 5);

	// set colors - all black for now
	for (int a = 0; a < 32; a++)
		copPtr = copSetColor(copPtr, a, 0);

	// jump to copper2
	*copPtr++ = offsetof(struct Custom, copjmp2);
	*copPtr++ = 0x7fff;

	custom->cop1lc = (ULONG)copper1;
	custom->cop2lc = (ULONG)copper2;
	custom->dmacon = DMAF_BLITTER;//disable blitter dma for copjmp bug
	custom->copjmp1 = 0x7fff; //start copper

	custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER;

	// Install interrupt handler
	SetInterruptHandler((APTR)interruptHandler);
#ifdef MUSIC
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_EXTER; // ThePlayer needs both VERTB and EXTER
#else
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB;
#endif

	custom->intreq = 1 << INTB_VERTB;

	// Main game loop
	while (!MouseLeft()) {
		Wait10();
		// Game logic happens here
	}

#ifdef MUSIC
	p61End();
#endif

	// Clean up
	FreeMem(copper1, 1024);
	FreeSystem();

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);
	
	return 0;
}
