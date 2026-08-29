#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

extern struct ExecBase *SysBase;
extern volatile struct Custom *custom;
extern struct GfxBase *GfxBase;

// System state backup
static UWORD SystemInts;
static UWORD SystemDMA;
static UWORD SystemADKCON;
static volatile APTR VBR = 0;
static APTR SystemIrq;

struct View *ActiView;

static APTR GetVBR(void) {
	APTR vbr = 0;
	UWORD getvbr[] = { 0x4e7a, 0x0801, 0x4e73 }; // MOVEC.L VBR,D0 RTE

	if (SysBase->AttnFlags & AFF_68010)
		vbr = (APTR)Supervisor((ULONG (*)())getvbr);

	return vbr;
}

void SetInterruptHandler(APTR interrupt) {
	*(volatile APTR*)(((UBYTE*)VBR) + 0x6c) = interrupt;
}

APTR GetInterruptHandler() {
	return *(volatile APTR*)(((UBYTE*)VBR) + 0x6c);
}

void WaitVbl() {
	debug_start_idle();
	while (1) {
		volatile ULONG vpos = *(volatile ULONG*)0xDFF004;
		vpos &= 0x1ff00;
		if (vpos != (311 << 8))
			break;
	}
	while (1) {
		volatile ULONG vpos = *(volatile ULONG*)0xDFF004;
		vpos &= 0x1ff00;
		if (vpos == (311 << 8))
			break;
	}
	debug_stop_idle();
}

void WaitLine(USHORT line) {
	while (1) {
		volatile ULONG vpos = *(volatile ULONG*)0xDFF004;
		if (((vpos >> 8) & 511) == line)
			break;
	}
}

__attribute__((always_inline)) inline void WaitBlt() {
	UWORD tst = *(volatile UWORD*)&custom->dmaconr; //for compatibility a1000
	(void)tst;
	while (*(volatile UWORD*)&custom->dmaconr & (1 << 14)) {} //blitter busy wait
}

void TakeSystem() {
	Forbid();
	//Save current interrupts and DMA settings so we can restore them upon exit. 
	SystemADKCON = custom->adkconr;
	SystemInts = custom->intenar;
	SystemDMA = custom->dmaconr;
	ActiView = GfxBase->ActiView; //store current view

	LoadView(0);
	WaitTOF();
	WaitTOF();

	WaitVbl();
	WaitVbl();

	OwnBlitter();
	WaitBlt();
	Disable();

	custom->intena = 0x7fff;//disable all interrupts
	custom->intreq = 0x7fff;//Clear any interrupts that were pending

	custom->dmacon = 0x7fff;//Clear all DMA channels

	//set all colors black
	for (int a = 0; a < 32; a++)
		custom->color[a] = 0;

	WaitVbl();
	WaitVbl();

	VBR = GetVBR();
	SystemIrq = GetInterruptHandler(); //store interrupt register
}

void FreeSystem() {
	WaitVbl();
	WaitBlt();
	custom->intena = 0x7fff;//disable all interrupts
	custom->intreq = 0x7fff;//Clear any interrupts that were pending
	custom->dmacon = 0x7fff;//Clear all DMA channels

	//restore interrupts
	SetInterruptHandler(SystemIrq);

	/*Restore system copper list(s). */
	custom->cop1lc = (ULONG)GfxBase->copinit;
	custom->cop2lc = (ULONG)GfxBase->LOFlist;
	custom->copjmp1 = 0x7fff; //start copper

	/*Restore all interrupts and DMA settings. */
	custom->intena = SystemInts | 0x8000;
	custom->dmacon = SystemDMA | 0x8000;
	custom->adkcon = SystemADKCON | 0x8000;

	WaitBlt();
	DisownBlitter();
	Enable();

	LoadView(ActiView);
	WaitTOF();
	WaitTOF();

	Permit();
}
