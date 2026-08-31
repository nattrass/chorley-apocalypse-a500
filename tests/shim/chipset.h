// The fake Amiga chipset the unit tests run the real engine code against: a chip RAM arena and
// a software blitter driven by the custom registers.
//
// Chip RAM is modelled as one arena because the hardware constraint is real -- the blitter can
// only reach chip memory -- and modelling it turns "you passed the blitter a pointer into the
// wrong kind of RAM" from an A500-only crash into a test failure. Every buffer a test hands to
// the engine must come from chipAlloc().

#pragma once
#ifndef CHORLEY_TEST_CHIPSET_H
#define CHORLEY_TEST_CHIPSET_H

#include <exec/types.h>
#include <hardware/custom.h>
#include <stddef.h>

// --- chip RAM ------------------------------------------------------------------------------

void*        chipAlloc(size_t bytes);           // zeroed, word aligned
void         chipReset();                       // drop every allocation
size_t       chipUsed();
bool         chipContains(const void* p);
void*        chipHost(unsigned int amigaAddr);  // inverse of chipAddr()

// --- blitter -------------------------------------------------------------------------------

// Blits since the last chipResetBlitCount(). The engine's frame cost is counted in blits, so
// tests assert on this as well as on the pixels.
unsigned long chipBlitCount();
void          chipResetBlitCount();

// Big-endian word access, the way the chips see chip RAM, so tests read what an A500 would.
inline UWORD peekWord(const void* p) {
	const UBYTE* b = (const UBYTE*)p;
	return (UWORD)((b[0] << 8) | b[1]);
}
inline void pokeWord(void* p, UWORD v) {
	UBYTE* b = (UBYTE*)p;
	b[0] = (UBYTE)(v >> 8);
	b[1] = (UBYTE)v;
}

extern volatile struct Custom* custom;

#endif
