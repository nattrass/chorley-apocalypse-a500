// Host stand-in for <exec/types.h>, for the native unit-test build only.
//
// The game sources are compiled unmodified against this, so the types have to match the Amiga
// ones where it matters: UBYTE/UWORD widths, and sizeof(APTR) == 4 (copper.h computes bitplane
// pointer register offsets from it).
//
// One deliberate deviation: ULONG is pointer-sized here, not 32-bit. copper.h casts a host
// pointer to ULONG, which on a 64-bit host is a compile error for a 32-bit type. The copper
// list therefore ends up holding the low 32 bits of a host address -- which is what
// tests/test_display.cpp checks against.

#pragma once
#ifndef CHORLEY_TEST_EXEC_TYPES_H
#define CHORLEY_TEST_EXEC_TYPES_H

typedef unsigned char  UBYTE;
typedef signed char    BYTE;
typedef unsigned short UWORD;
typedef short          WORD;
typedef unsigned short USHORT;
typedef short          SHORT;
typedef __UINTPTR_TYPE__ ULONG;
typedef __INTPTR_TYPE__  LONG;
typedef short          BOOL;
typedef char           TEXT;
typedef const char*    CONST_STRPTR;
typedef char*          STRPTR;

#define TRUE  1
#define FALSE 0

// Amiga chip address of a host buffer. Defined in tests/shim/chipmem.cpp.
unsigned int chipAddr(const void* p);

// A 32-bit Amiga pointer. Every custom-chip pointer register is one of these, so the register
// block keeps the hardware layout on a 64-bit host. Constructing one from a host pointer maps
// it into the fake chip RAM arena and aborts if the buffer is not chip memory -- which is the
// real constraint the blitter imposes, made loud.
struct APTR {
	unsigned int addr;

	APTR() : addr(0) {}
	APTR(const void* p) : addr(chipAddr(p)) {}

	void operator=(const APTR& o) volatile { addr = o.addr; }
	void operator=(const APTR& o)          { addr = o.addr; }
};

static_assert(sizeof(APTR) == 4, "APTR must be 4 bytes or the custom register layout shifts");
static_assert(sizeof(UWORD) == 2, "UWORD must be 16 bits");
static_assert(sizeof(UBYTE) == 1, "UBYTE must be 8 bits");

#endif
