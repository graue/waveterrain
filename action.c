#include <stdio.h>
#include <math.h>
#include <err.h>
#include "SDL.h"
#include "sndlib.h"
#include "main.h"
#include "expr.h"
#include "parse.h"
#include "mutate.h"

#define M_20_OVER_LN10 8.68588963806503655302257838
#define M_LN10_OVER_20 0.115129254649702284200899573
#define RATTODB(x) (log(x) * M_20_OVER_LN10)
#define DBTORAT(x) exp((x) * M_LN10_OVER_20)
#define DC_OFFSET 1.19209290E-07F /* <float.h>'s FLT_EPSILON */

#define MAXTERRAINS 100
expr_t *terrains[MAXTERRAINS];
int numterrains = 0;
int tindex = 0; // current terrain

float x, y; // current location of pickup over terrain
float angledeg; // angle, in degrees, that pickup is moving
float angleturn; // current angle turn speed in signed degrees/sec
float speed; // speed which pickup moves PER SECOND
float amp; // overall gain (multiplier, not dB)

#define MAXAMPDB -10
#define AMPDBRANGE 40

// returns 1 if success 0 if fail
static int addterrain(const char *expr_str, int *skipped)
{
	if (numterrains == MAXTERRAINS)
		return 0;
	terrains[numterrains] = parse(expr_str, skipped);
	if (terrains[numterrains] == NULL)
		return 0;
	numterrains++;
	return 1;
}

char terrainexprbuf[102400];

// returns values in [-1, 1]
static float eval_terrain_at(float x, float y)
{
	float f;

	f = amp * evaluate(terrains[tindex], x, y, 0.0 /* time not used */);

	// clip
	if (f < -1.0) f = -1.0;
	else if (f > 1.0) f = 1.0;

	return f;
}

FILE *audiodump;

void action_init(void)
{
	int numskipped;
	const char *p;
	FILE *fp;

	memset(terrainexprbuf, 0, sizeof terrainexprbuf);
	fp = fopen("terrains.txt", "r");
	if (fp == NULL)
		err(1, "fopen terrains.txt");
	fread(terrainexprbuf, 1, sizeof terrainexprbuf - 1, fp);
	fclose(fp);
	p = terrainexprbuf;

	while (addterrain(p, &numskipped))
		p += numskipped;

	if (numterrains == 0)
		errx(1, "no valid terrains");

	fprintf(stderr, "loaded %d terrains\n", numterrains);

	tindex = 0;
	x = y = 0.0;
	angledeg = 0.0;
	angleturn = 360.0;
	speed = 500.0;
	amp = DBTORAT(MAXAMPDB - AMPDBRANGE);

	audiodump = fopen("audiodump.f32", "wb");
}

// max angle turn change in degrees/sec/sec
#define ANGLE_ACCEL 10.0

// each axis is [-1.0, 1.0]
void action_control(float rotation, float lever, float updn, float lr,
	int buttons)
{
	static int lastbuttons = 0;
	int newbuttons;

	newbuttons = buttons & (~lastbuttons);
	lastbuttons = buttons;

	if (newbuttons & JOYBTN_1)
		mutate(terrains[tindex]);
	if (newbuttons & JOYBTN_3)
		tindex = (tindex+numterrains-1) % numterrains;
	if (newbuttons & JOYBTN_4)
		tindex = (tindex+1) % numterrains;

	angleturn += ANGLE_ACCEL * rotation;
	amp = DBTORAT(MAXAMPDB - (lever + 1.0) / 2.0 * AMPDBRANGE);
	if (updn != 0.0) speed *= pow(2, updn);
}

void action_writesamples(int numframes)
{
	float anglerad; // angle in radians
	float d_angle; // angle turn in radians per sample frame
	float speedsamp; // speed in units per sample frame
	int ix;
	float f[2];

	anglerad = angledeg * M_PI / 180.0;
	d_angle = angleturn * M_PI / 180.0 / RATE;
	speedsamp = speed / RATE;

	for (ix = 0; ix < numframes; ix++)
	{
		anglerad += d_angle;
		x += cos(anglerad) * speedsamp;
		y += sin(anglerad) * speedsamp;

		f[0] = f[1] = eval_terrain_at(x, y);
		// TODO: gain etc?

		snd_writesample(f[0] * 32768.0);
		snd_writesample(f[1] * 32768.0);

		if (audiodump != NULL)
			fwrite(f, sizeof f[0], 2, audiodump);
	}

	angledeg = anglerad * 180.0 / M_PI;
}

// put pixel macro - assumes surface is already locked
#define PUTPIXEL16(surf,x,y,r,g,b) { \
	Uint32 color = SDL_MapRGB((surf)->format, (r), (g), (b)); \
	Uint16 *bufp = (Uint16 *)(surf)->pixels + (y)*(surf)->pitch/2 + (x); \
	*bufp = color; \
}

// zoom such that this many seconds of straight vertical/horizontal travel
// (whichever dimension is smaller)
// are visible at once
#define ZOOM_SECS 10

// surface should be locked (if needed) prior to calling this
void action_dodisplay(SDL_Surface *disp, int w, int h, int lines)
{
	float scrtoplanemul;
	float planeleft, planetop;
	int scrx, scry;
	float f;
	Uint8 intensity; // color intensity 0-255
	static int row = 0;
	int endrow = (row + lines) % h;

	scrtoplanemul = ZOOM_SECS * speed / (w < h ? w : h);
	planeleft = x - (0.5*w / scrtoplanemul);
	planetop = y - (0.5*h / scrtoplanemul);

	for (scrx = 0; scrx < w; scrx++)
	for (scry = row; scry != endrow; scry = (scry+1) % h)
	{
		f = eval_terrain_at(planeleft + scrx*scrtoplanemul,
			planetop + scry*scrtoplanemul);

		f += 1.0;
		f *= 255.9 / 2.0;
		intensity = (Uint8)f;

		PUTPIXEL16(disp, scrx, scry, intensity, intensity, intensity);
	}

	row = endrow;
}
