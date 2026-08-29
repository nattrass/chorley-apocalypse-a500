#pragma once
#ifndef CHORLEY_SYSTEM_H
#define CHORLEY_SYSTEM_H

#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <hardware/custom.h>

extern volatile struct Custom *custom;

void TakeSystem();
void FreeSystem();
void SetInterruptHandler(void* interrupt);
void* GetInterruptHandler();
void WaitVbl();
void WaitLine(unsigned short line);

__attribute__((always_inline)) inline void WaitBlt() {
	UWORD tst = *(volatile UWORD*)&custom->dmaconr; // for compatibility a1000
	(void)tst;
	while (*(volatile UWORD*)&custom->dmaconr & (1 << 14)) {} // blitter busy wait
}

extern struct View *ActiView;

#endif // CHORLEY_SYSTEM_H
