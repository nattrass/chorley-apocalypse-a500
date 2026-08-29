#include "keyboard.h"
#include <hardware/custom.h>

extern volatile struct Custom *custom;

alignas(4) static bool keyStates[128];
static bool escPressed = false;

void initKeyboard() {
	for (int i = 0; i < 128; i++) {
		keyStates[i] = false;
	}
	escPressed = false;

	// Set CIA-A CRA bit 6 to 0 (Serial Port Input mode)
	*(volatile UBYTE*)0xbfee01 &= ~0x40;

	// Clear any pending state in CIA-A ICR and SDR
	volatile UBYTE dummy = *(volatile UBYTE*)0xbfed01;
	(void)dummy;
	dummy = *(volatile UBYTE*)0xbfec01;
	(void)dummy;
}

void pollKeyboard() {
	// Check if CIA-A Serial Port interrupt is pending (bit 3 / 0x08 of ICR)
	if (*(volatile UBYTE*)0xbfed01 & 0x08) {
		UBYTE sdr = *(volatile UBYTE*)0xbfec01;

		// Handshake pulse to keyboard controller (pull SP line low for ~100us)
		*(volatile UBYTE*)0xbfee01 |= 0x40; // Output mode pulls line low
		for (volatile int i = 0; i < 120; i++) {} // ~100us delay on 7MHz 68000
		*(volatile UBYTE*)0xbfee01 &= ~0x40; // Input mode releases line

		// Decode Amiga raw scancode: inverted, rotated right by 1
		UBYTE raw = ~sdr;
		UBYTE keyCode = (raw >> 1) | ((raw & 1) << 7);
		bool isRelease = (keyCode & 0x80) != 0;
		UBYTE keyIndex = keyCode & 0x7f;

		keyStates[keyIndex] = !isRelease;

		if (keyIndex == KEY_ESC && !isRelease) {
			escPressed = true;
		}

		// Clear PORT interrupt in INTREQ
		custom->intreq = 1 << 3; // INTB_PORTS
	}
}

bool isKeyDown(UBYTE rawKeyCode) {
	if (rawKeyCode < 128)
		return keyStates[rawKeyCode];
	return false;
}

bool keyLeftHeld() {
	return keyStates[KEY_LEFT] || keyStates[KEY_A] || keyStates[0x2d]; // 0x2d = NumPad 4
}

bool keyRightHeld() {
	return keyStates[KEY_RIGHT] || keyStates[KEY_D] || keyStates[0x2f]; // 0x2f = NumPad 6
}

bool keyUpHeld() {
	return keyStates[KEY_UP] || keyStates[KEY_W] || keyStates[0x3d]; // 0x3d = NumPad 8
}

bool keyDownHeld() {
	return keyStates[KEY_DOWN] || keyStates[KEY_S] || keyStates[0x1e]; // 0x1e = NumPad 2
}

bool keyFireHeld() {
	return keyStates[KEY_SPACE] || keyStates[KEY_LSHIFT] || keyStates[KEY_RSHIFT] ||
	       keyStates[KEY_RETURN] || keyStates[0x64] || keyStates[0x65] || keyStates[0x0f];
}

bool keyEscPressed() {
	return escPressed || keyStates[KEY_ESC];
}
