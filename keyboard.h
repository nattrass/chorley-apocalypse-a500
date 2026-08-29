#pragma once
#ifndef CHORLEY_KEYBOARD_H
#define CHORLEY_KEYBOARD_H

#include "support/gcc8_c_support.h"
#include <exec/types.h>

// Initialize keyboard hardware on CIA-A
void initKeyboard();

// Poll keyboard state from CIA-A SDR
void pollKeyboard();

// Check if a specific raw key is currently held down
bool isKeyDown(UBYTE rawKeyCode);

// Directional and action helpers emulating joystick
bool keyLeftHeld();
bool keyRightHeld();
bool keyUpHeld();
bool keyDownHeld();
bool keyFireHeld();
bool keyEscPressed();

// Key code definitions
#define KEY_ESC        0x45
#define KEY_SPACE      0x40
#define KEY_RETURN     0x44
#define KEY_UP         0x4c
#define KEY_DOWN       0x4d
#define KEY_RIGHT      0x4e
#define KEY_LEFT       0x4f
#define KEY_W          0x11
#define KEY_A          0x20
#define KEY_S          0x21
#define KEY_D          0x22
#define KEY_LSHIFT     0x60
#define KEY_RSHIFT     0x61

#endif // CHORLEY_KEYBOARD_H
