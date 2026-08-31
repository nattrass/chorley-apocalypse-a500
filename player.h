#pragma once
#ifndef CHORLEY_PLAYER_H
#define CHORLEY_PLAYER_H

#include "support/gcc8_c_support.h"
#include "game/gamedefs.h"
#include "bob.h"
#include <exec/types.h>

// The player, the bullet pool and the camera. Positions are world pixels in 1/16ths, which is
// enough to keep an eight-way walk from drifting: the diagonals move 11/16 of the straight
// speed on each axis, and rounding that every frame in whole pixels would visibly bias.

#define FP_SHIFT        4                       // 1/16 pixel fixed point
#define FP_ONE          (1 << FP_SHIFT)

#define PLAYER_SPEED    4                       // Mill-Hand Speed stat: literal pixels/frame
#define PLAYER_ANIM_RATE 6                      // frames per walk frame

#define BULLET_SPEED    6                       // pixels/frame
#define BULLET_LIFE     60                      // frames
#define FIRE_INTERVAL   4                       // frames between shots while fire is held

// Camera dead zone: the player can move this far from centre before the camera follows, so
// small steps and the walk animation do not shake the whole screen.
#define DEADZONE_W      80
#define DEADZONE_H      56

// The M2 gate wants ~24 bobs live. Holding fire gets close, but not reliably and not with them
// all on screen, so T spawns a screen-filling grid of 32x32 bobs that is exactly the budget in
// DESIGN.md section 3: STRESS_BOBS plus the player.
#define STRESS_BOBS     23

struct PlayerInput {
	bool up, down, left, right, fire;
};

void entitiesInit(int startWorldX, int startWorldY);

// Step 1 of the frame: put back the background under everything drawn last frame.
void entitiesRestore(const RenderCtx* ctx);

// Step 2: move the player, latch the fire direction, run the bullets.
void entitiesUpdate(const PlayerInput* in, short frame);

// Step 2b: follow the player, dead zone first, then clamped to the map.
void cameraFollow(int* camX, int* camY);

// Step 4: draw everything, recording what to restore next frame. Returns the number of bobs
// actually blitted, which is the number the profiler should be told about.
int entitiesDraw(const RenderCtx* ctx, const UBYTE* playerSheet, const UBYTE* bulletSheet, short frame);

void entitiesToggleStress();
bool entitiesStressOn();

int playerX();          // world pixels, top-left of the bob
int playerY();

#endif // CHORLEY_PLAYER_H
