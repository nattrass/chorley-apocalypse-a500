#ifndef MUSIC_H
#define MUSIC_H

#include <exec/types.h>

#ifdef MUSIC
int p61Init(const void* module);
void p61Music();
void p61End();

extern const UBYTE module[];
#endif // MUSIC

#endif // MUSIC_H
