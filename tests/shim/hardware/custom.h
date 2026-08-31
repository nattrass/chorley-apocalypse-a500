// Host stand-in for <hardware/custom.h>: the custom chip register block at the real offsets,
// with a software blitter behind it.
//
// Register writes from the game are plain stores into this struct. bltsize is the exception:
// writing it starts a blit on real hardware, so here it is a type whose assignment operator
// runs the emulation immediately. Everything a blit needs is already in the struct by then,
// which is exactly the ordering the hardware requires too.
//
// The offsets are asserted below rather than trusted: copper.h builds copper instructions out
// of offsetof(struct Custom, ...), so a wrong offset would silently make every copper-list
// test meaningless.

#pragma once
#ifndef CHORLEY_TEST_HARDWARE_CUSTOM_H
#define CHORLEY_TEST_HARDWARE_CUSTOM_H

#include <exec/types.h>

// Runs the blit described by the current register contents. Defined in tests/shim/chipset.cpp.
void fakeBlitterRun(UWORD bltsize);

struct BltSizeReg {
	UWORD v;
	void operator=(UWORD size) volatile { v = size; fakeBlitterRun(size); }
};

struct Custom {
	UWORD      bltddat;                     // 0x000
	UWORD      dmaconr;                     // 0x002 -- always reads 0: the fake blitter is never busy
	UWORD      pad004[(0x040 - 0x004) / 2];
	UWORD      bltcon0;                     // 0x040
	UWORD      bltcon1;                     // 0x042
	UWORD      bltafwm;                     // 0x044
	UWORD      bltalwm;                     // 0x046
	APTR       bltcpt;                      // 0x048
	APTR       bltbpt;                      // 0x04c
	APTR       bltapt;                      // 0x050
	APTR       bltdpt;                      // 0x054
	BltSizeReg bltsize;                     // 0x058
	UWORD      pad05a[(0x060 - 0x05a) / 2];
	UWORD      bltcmod;                     // 0x060
	UWORD      bltbmod;                     // 0x062
	UWORD      bltamod;                     // 0x064
	UWORD      bltdmod;                     // 0x066
	UWORD      pad068[(0x08e - 0x068) / 2];
	UWORD      diwstrt;                     // 0x08e
	UWORD      diwstop;                     // 0x090
	UWORD      ddfstrt;                     // 0x092
	UWORD      ddfstop;                     // 0x094
	UWORD      dmacon;                      // 0x096
	UWORD      clxcon;                      // 0x098
	UWORD      intena;                      // 0x09a
	UWORD      intreq;                      // 0x09c
	UWORD      adkcon;                      // 0x09e
	UWORD      pad0a0[(0x0e0 - 0x0a0) / 2];
	APTR       bplpt[8];                    // 0x0e0
	UWORD      bplcon0;                     // 0x100
	UWORD      bplcon1;                     // 0x102
	UWORD      bplcon2;                     // 0x104
	UWORD      bplcon3;                     // 0x106
	UWORD      bpl1mod;                     // 0x108
	UWORD      bpl2mod;                     // 0x10a
	UWORD      pad10c[(0x180 - 0x10c) / 2];
	UWORD      color[32];                   // 0x180
	UWORD      pad1c0[(0x200 - 0x1c0) / 2];
};

#define CHORLEY_ASSERT_REG(name, off) \
	static_assert(__builtin_offsetof(struct Custom, name) == off, #name " is at the wrong offset")

CHORLEY_ASSERT_REG(dmaconr, 0x002);
CHORLEY_ASSERT_REG(bltcon0, 0x040);
CHORLEY_ASSERT_REG(bltcon1, 0x042);
CHORLEY_ASSERT_REG(bltafwm, 0x044);
CHORLEY_ASSERT_REG(bltalwm, 0x046);
CHORLEY_ASSERT_REG(bltcpt,  0x048);
CHORLEY_ASSERT_REG(bltbpt,  0x04c);
CHORLEY_ASSERT_REG(bltapt,  0x050);
CHORLEY_ASSERT_REG(bltdpt,  0x054);
CHORLEY_ASSERT_REG(bltsize, 0x058);
CHORLEY_ASSERT_REG(bltcmod, 0x060);
CHORLEY_ASSERT_REG(bltbmod, 0x062);
CHORLEY_ASSERT_REG(bltamod, 0x064);
CHORLEY_ASSERT_REG(bltdmod, 0x066);
CHORLEY_ASSERT_REG(diwstrt, 0x08e);
CHORLEY_ASSERT_REG(diwstop, 0x090);
CHORLEY_ASSERT_REG(ddfstrt, 0x092);
CHORLEY_ASSERT_REG(ddfstop, 0x094);
CHORLEY_ASSERT_REG(dmacon,  0x096);
CHORLEY_ASSERT_REG(intena,  0x09a);
CHORLEY_ASSERT_REG(intreq,  0x09c);
CHORLEY_ASSERT_REG(bplpt,   0x0e0);
CHORLEY_ASSERT_REG(bplcon0, 0x100);
CHORLEY_ASSERT_REG(bplcon1, 0x102);
CHORLEY_ASSERT_REG(bplcon2, 0x104);
CHORLEY_ASSERT_REG(bpl1mod, 0x108);
CHORLEY_ASSERT_REG(bpl2mod, 0x10a);
CHORLEY_ASSERT_REG(color,   0x180);

#undef CHORLEY_ASSERT_REG

static_assert(sizeof(struct Custom) == 0x200, "the custom register block is 512 bytes");

#endif
