#include "support/gcc8_c_support.h"
#include <hardware/custom.h>

// Copper list helper functions

__attribute__((always_inline)) inline USHORT* copSetPlanes(UBYTE bplPtrStart, USHORT* copListEnd, const UBYTE **planes, int numPlanes) {
	for (USHORT i = 0; i < numPlanes; i++) {
		ULONG addr = (ULONG)planes[i];
		*copListEnd++ = offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR);
		*copListEnd++ = (UWORD)(addr >> 16);
		*copListEnd++ = offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR) + 2;
		*copListEnd++ = (UWORD)addr;
	}
	return copListEnd;
}

__attribute__((always_inline)) inline USHORT* copWaitXY(USHORT *copListEnd, USHORT x, USHORT i) {
	*copListEnd++ = (i << 8) | (x << 1) | 1;	//bit 1 means wait. waits for vertical position x<<8, first raster stop position outside the left 
	*copListEnd++ = 0xfffe;
	return copListEnd;
}

__attribute__((always_inline)) inline USHORT* copWaitY(USHORT* copListEnd, USHORT i) {
	*copListEnd++ = (i << 8) | 4 | 1;	//bit 1 means wait. waits for vertical position x<<8, first raster stop position outside the left 
	*copListEnd++ = 0xfffe;
	return copListEnd;
}

__attribute__((always_inline)) inline USHORT* copSetColor(USHORT* copListCurrent, USHORT index, USHORT color) {
	*copListCurrent++ = offsetof(struct Custom, color) + sizeof(UWORD) * index;
	*copListCurrent++ = color;
	return copListCurrent;
}

__attribute__((always_inline)) inline USHORT* screenScanDefault(USHORT* copListEnd) {
	const USHORT x = 129;
	const USHORT width = 320;
	const USHORT height = 256;
	const USHORT y = 44;
	const USHORT RES = 8; //8=lowres,4=hires
	USHORT xstop = x + width;
	USHORT ystop = y + height;
	USHORT fw = (x >> 1) - RES;

	*copListEnd++ = offsetof(struct Custom, ddfstrt);
	*copListEnd++ = fw;
	*copListEnd++ = offsetof(struct Custom, ddfstop);
	*copListEnd++ = fw + (((width >> 4) - 1) << 3);
	*copListEnd++ = offsetof(struct Custom, diwstrt);
	*copListEnd++ = x + (y << 8);
	*copListEnd++ = offsetof(struct Custom, diwstop);
	*copListEnd++ = (xstop - 256) + ((ystop - 256) << 8);
	return copListEnd;
}
