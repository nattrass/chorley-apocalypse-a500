#define MUSIC

#include "support/gcc8_c_support.h"
#include <hardware/custom.h>

#ifdef MUSIC

// Demo - Module Player - ThePlayer 6.1a: https://www.pouet.net/prod.php?which=19922
// The Player® 6.1A: Copyright © 1992-95 Jarno Paananen
// P61.testmod - Module by Skylord/Sector 7 
INCBIN(player, "player610.6.no_cia.bin")
INCBIN_CHIP(module, "testmod.p61")

int p61Init(const void* module) { // returns 0 if success, non-zero otherwise
	register volatile const void* _a0 ASM("a0") = module;
	register volatile const void* _a1 ASM("a1") = NULL;
	register volatile const void* _a2 ASM("a2") = NULL;
	register volatile const void* _a3 ASM("a3") = player;
	register                int   _d0 ASM("d0"); // return value
	__asm volatile (
		"movem.l %%d1-%%d7/%%a4-%%a6,-(%%sp)\n"
		"jsr 0(%%a3)\n"
		"movem.l (%%sp)+,%%d1-%%d7/%%a4-%%a6"
	: "=r" (_d0), "+rf"(_a0), "+rf"(_a1), "+rf"(_a2), "+rf"(_a3)
	:
	: "cc", "memory");
	return _d0;
}

void p61Music() {
	register volatile const void* _a3 ASM("a3") = player;
	register volatile const void* _a6 ASM("a6") = (void*)0xdff000;
	__asm volatile (
		"movem.l %%d0-%%d7/%%a0-%%a2/%%a4-%%a5,-(%%sp)\n"
		"jsr 4(%%a3)\n"
		"movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a2/%%a4-%%a5"
	: "+rf"(_a3), "+rf"(_a6)
	:
	: "cc", "memory");
}

void p61End() {
	register volatile const void* _a3 ASM("a3") = player;
	register volatile const void* _a6 ASM("a6") = (void*)0xdff000;
	__asm volatile (
		"movem.l %%d0-%%d1/%%a0-%%a1,-(%%sp)\n"
		"jsr 8(%%a3)\n"
		"movem.l (%%sp)+,%%d0-%%d1/%%a0-%%a1"
	: "+rf"(_a3), "+rf"(_a6)
	:
	: "cc", "memory");
}

#endif // MUSIC
