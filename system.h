#ifndef SYSTEM_H
#define SYSTEM_H

void TakeSystem();
void FreeSystem();
void SetInterruptHandler(void* interrupt);
void* GetInterruptHandler();
void WaitVbl();
void WaitLine(unsigned short line);
__attribute__((always_inline)) inline void WaitBlt();

extern struct View *ActiView;

#endif // SYSTEM_H
