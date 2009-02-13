#include <stdio.h>
#include <math.h>
#include <err.h>
#include "SDL.h"
#include "sndlib.h"
#include "main.h"
#include "expr.h"
#include "parse.h"

#define M_20_OVER_LN10 8.68588963806503655302257838
#define M_LN10_OVER_20 0.115129254649702284200899573
#define RATTODB(x) (log(x) * M_20_OVER_LN10)
#define DBTORAT(x) exp((x) * M_LN10_OVER_20)
#define DC_OFFSET 1.19209290E-07F /* <float.h>'s FLT_EPSILON */

expr_t *terrain;
float x, y; // current location of pickup over terrain
float angledeg; // angle, in degrees, that pickup is moving
float speed; // speed which pickup moves PER SECOND

static const char *terrain_expr_str = 
"(+ (+ (+ (sin (* (* 2 pi ) y))"
"         (sin (* (* 2 pi ) (* x 2)))"
"      (+ (sin (* (* 2 pi ) (* y 3))"
"         (sin (* (* 2 pi ) (* x 4)))))"
"   (+ (+ (sin (* (* 2 pi ) (* y 5)))"
"         (sin (* (* 2 pi ) (* x 6)))"
"      (+ (sin (* (* 2 pi ) (* y 7))"
"         (sin (* (* 2 pi ) (* x 8)))))))";

static float eval_terrain_at(float x, float y)
{
	return sin(evaluate(terrain, x, y, 0.0 /* time not used */));
}

FILE *audiodump;

void action_init(void)
{
	terrain = parse(terrain_expr_str, NULL);
	if (terrain == NULL)
		errx(1, "terrain string wouldn't parse");
	x = y = 0.0;
	angledeg = 0.0;
	speed = 500.0;

	audiodump = fopen("audiodump.f32", "wb");
}

void action_control(float rotation)
{
	angledeg += rotation;
	if (angledeg < -180.0) angledeg += 360.0;
	else if (angledeg >= 180.0) angledeg -= 360.0;
}

void action_writesamples(int numframes)
{
	float dx, dy; // x, y movement per sample frame
	int ix;
	float f[2];

	dx = cos(angledeg * M_PI / 180.0) * speed / RATE;
	dy = sin(angledeg * M_PI / 180.0) * speed / RATE;

	for (ix = 0; ix < numframes; ix++)
	{
		x += dx;
		y += dy;

		f[0] = f[1] = eval_terrain_at(x, y);
		// TODO: gain etc?

		snd_writesample(f[0] * 32768.0);
		snd_writesample(f[1] * 32768.0);

		if (audiodump != NULL)
			fwrite(f, sizeof f[0], 2, audiodump);
	}
}

// put pixel macro - assumes surface is already locked
#define PUTPIXEL16(surf,x,y,r,g,b) { \
	Uint32 color = SDL_MapRGB((surf)->format, (r), (g), (b)); \
	Uint16 *bufp = (Uint16 *)(surf)->pixels + (y)*(surf)->pitch/2 + (x); \
	*bufp = color; \
}

// surface should be locked (if needed) prior to calling this
void action_dodisplay(SDL_Surface *disp, int w, int h)
{
	// TODO
}
