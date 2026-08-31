#include "player.h"
#include "map.h"

// --- state ----------------------------------------------------------------------------------

struct Player {
	int   x, y;             // world position of the bob top-left, 1/16 pixel
	UBYTE facing;           // 0..7, the direction the gun points
	UBYTE animFrame;
	UBYTE animTimer;
	UBYTE fireTimer;
	bool  firePrev;
};

struct Bullet {
	int   x, y;             // 1/16 pixel
	UBYTE dir;
	UBYTE life;
	bool  active;
};

static Player  player;
static Bullet  bullets[MAX_BULLETS];
static bool    stressOn;

// Everything drawn last frame, waiting to be restored. Bounded by what can be drawn in one
// frame: the player, the whole bullet pool, and the stress grid.
#define MAX_DRAWN (1 + MAX_BULLETS + STRESS_BOBS)
static BobRect drawn[MAX_DRAWN];
static int     drawnCount;

static const int MAP_PIXEL_W = MAP_W * TILE_SIZE;
static const int MAP_PIXEL_H = MAP_H * TILE_SIZE;

// --- helpers --------------------------------------------------------------------------------

// The eight-way direction the stick is asking for, or -1 for centred.
static int stickDirection(const PlayerInput* in) {
	if (in->up    && !in->down) {
		if (in->right && !in->left) return 1;
		if (in->left  && !in->right) return 7;
		return 0;
	}
	if (in->down  && !in->up) {
		if (in->right && !in->left) return 3;
		if (in->left  && !in->right) return 5;
		return 4;
	}
	if (in->right && !in->left) return 2;
	if (in->left  && !in->right) return 6;
	return -1;
}

static void spawnBullet() {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (bullets[i].active) continue;
		const int d = player.facing;
		// Out of the barrel, not out of the middle of the bob.
		bullets[i].x = player.x + ((BOB_W - BULLET_W) / 2) * FP_ONE + dirX[d] * 12 * FP_ONE / 16;
		bullets[i].y = player.y + ((BOB_H - BULLET_H) / 2) * FP_ONE + dirY[d] * 12 * FP_ONE / 16;
		bullets[i].dir    = (UBYTE)d;
		bullets[i].life   = BULLET_LIFE;
		bullets[i].active = true;
		return;
	}
}

// --- interface ------------------------------------------------------------------------------

void entitiesInit(int startWorldX, int startWorldY) {
	player.x         = startWorldX * FP_ONE;
	player.y         = startWorldY * FP_ONE;
	player.facing    = 4;       // facing south, out of the screen
	player.animFrame = 0;
	player.animTimer = 0;
	player.fireTimer = 0;
	player.firePrev  = false;

	for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;

	stressOn   = false;
	drawnCount = 0;
}

void entitiesRestore(const RenderCtx* ctx) {
	for (int i = 0; i < drawnCount; i++) bobRestore(ctx, &drawn[i]);
	drawnCount = 0;
}

void entitiesUpdate(const PlayerInput* in, short frame) {
	(void)frame;

	const int moveDir = stickDirection(in);

	// Fire direction is latched on the press and held there for as long as the button is: that
	// is the Chaos Engine feel, walk one way while shooting another. With the button up the gun
	// simply follows the walk.
	if (in->fire) {
		if (!player.firePrev && moveDir >= 0) player.facing = (UBYTE)moveDir;
	} else if (moveDir >= 0) {
		player.facing = (UBYTE)moveDir;
	}
	player.firePrev = in->fire;

	if (moveDir >= 0) {
		const int step = PLAYER_SPEED * FP_ONE;
		player.x += dirX[moveDir] * step / 16;
		player.y += dirY[moveDir] * step / 16;

		if (++player.animTimer >= PLAYER_ANIM_RATE) {
			player.animTimer = 0;
			player.animFrame ^= 1;
		}
	} else {
		player.animTimer = 0;
		player.animFrame = 0;
	}

	// Map edges. Walls arrive with the attribute layer in M3.
	const int maxX = (MAP_PIXEL_W - BOB_W) * FP_ONE;
	const int maxY = (MAP_PIXEL_H - BOB_H) * FP_ONE;
	if (player.x < 0) player.x = 0; else if (player.x > maxX) player.x = maxX;
	if (player.y < 0) player.y = 0; else if (player.y > maxY) player.y = maxY;

	if (in->fire && player.fireTimer == 0) {
		spawnBullet();
		player.fireTimer = FIRE_INTERVAL;
	}
	if (player.fireTimer) player.fireTimer--;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!bullets[i].active) continue;
		const int d = bullets[i].dir;
		bullets[i].x += dirX[d] * (BULLET_SPEED * FP_ONE) / 16;
		bullets[i].y += dirY[d] * (BULLET_SPEED * FP_ONE) / 16;

		const int px = bullets[i].x >> FP_SHIFT;
		const int py = bullets[i].y >> FP_SHIFT;
		if (--bullets[i].life == 0 ||
		    px < -BULLET_W || px > MAP_PIXEL_W || py < -BULLET_H || py > MAP_PIXEL_H)
			bullets[i].active = false;
	}
}

void cameraFollow(int* camX, int* camY) {
	const int cx = (player.x >> FP_SHIFT) + BOB_W / 2;
	const int cy = (player.y >> FP_SHIFT) + BOB_H / 2;

	const int left  = SCREEN_W / 2 - DEADZONE_W / 2;
	const int right = SCREEN_W / 2 + DEADZONE_W / 2;
	const int top   = VIEW_H   / 2 - DEADZONE_H / 2;
	const int bot   = VIEW_H   / 2 + DEADZONE_H / 2;

	int x = *camX, y = *camY;
	if (cx - x < left)  x = cx - left;
	if (cx - x > right) x = cx - right;
	if (cy - y < top)   y = cy - top;
	if (cy - y > bot)   y = cy - bot;

	const int maxCamX = MAP_PIXEL_W - SCREEN_W;
	const int maxCamY = MAP_PIXEL_H - VIEW_H;
	if (x < 0) x = 0; else if (x > maxCamX) x = maxCamX;
	if (y < 0) y = 0; else if (y > maxCamY) y = maxCamY;

	*camX = x;
	*camY = y;
}

// Nothing off screen is worth a blit, and bobDraw would clip it away anyway.
__attribute__((always_inline)) static inline bool onScreen(const RenderCtx* ctx, int wx, int wy, int w, int h) {
	return wx + w > ctx->camX && wx < ctx->camX + SCREEN_W &&
	       wy + h > ctx->camY && wy < ctx->camY + VIEW_H;
}

int entitiesDraw(const RenderCtx* ctx, const UBYTE* playerSheet, const UBYTE* bulletSheet, short frame) {
	int count = 0;

	// Stress grid first, so it sits under everything that matters.
	if (stressOn) {
		for (int i = 0; i < STRESS_BOBS; i++) {
			const int col = i % 6;
			const int row = i / 6;
			// A triangle wave per bob, phase-shifted, so every one of them moves every frame and
			// the restore path gets the same work it would from a room full of enemies.
			const int t   = (frame + i * 7) & 63;
			const int osc = ((t < 32) ? t : 63 - t) / 2 - 8;

			const int wx = ctx->camX + 6 + col * 50 + osc;
			const int wy = ctx->camY + 4 + row * 46 - osc;
			if (!onScreen(ctx, wx, wy, BOB_W, BOB_H)) continue;

			const UBYTE* f = playerSheet + ((i & 7) * BOB_ANIM_FRAMES + (t >> 5)) * BOB_FRAME_BYTES;
			if (bobDraw(ctx, f, BOB_WORDS, BOB_H, wx, wy, &drawn[drawnCount])) {
				drawnCount++;
				count++;
			}
		}
	}

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!bullets[i].active) continue;
		const int wx = bullets[i].x >> FP_SHIFT;
		const int wy = bullets[i].y >> FP_SHIFT;
		if (!onScreen(ctx, wx, wy, BULLET_W, BULLET_H)) continue;

		const UBYTE* f = bulletSheet + bullets[i].dir * BULLET_FRAME_BYTES;
		if (bobDraw(ctx, f, BULLET_WORDS, BULLET_H, wx, wy, &drawn[drawnCount])) {
			drawnCount++;
			count++;
		}
	}

	{
		const int wx = player.x >> FP_SHIFT;
		const int wy = player.y >> FP_SHIFT;
		const UBYTE* f = playerSheet + (player.facing * BOB_ANIM_FRAMES + player.animFrame) * BOB_FRAME_BYTES;
		if (bobDraw(ctx, f, BOB_WORDS, BOB_H, wx, wy, &drawn[drawnCount])) {
			drawnCount++;
			count++;
		}
	}

	return count;
}

void entitiesToggleStress() { stressOn = !stressOn; }
bool entitiesStressOn()     { return stressOn; }

int playerX() { return player.x >> FP_SHIFT; }
int playerY() { return player.y >> FP_SHIFT; }
