#include <stdio.h>
#include <math.h>
#include <err.h>
#include "SDL.h"
#include "sndlib.h"
#include "main.h"
#include "expr.h"
#include "parse.h"
#include "mutate.h"

#undef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#undef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))

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

float pan, leftamp, rightamp;

#define MAXAMPDB -10
#define AMPDBRANGE 40

#define DELAYSAMPS (RATE*33/1000)
float delaybin[2*DELAYSAMPS];

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

// returns 1 if success 0 if fail
static int addterrainbyexpr(expr_t *expr)
{
	if (numterrains == MAXTERRAINS)
		return 0;
	terrains[numterrains] = expr;
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
//todo delaysamps clear
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
	pan = 0.0;
	leftamp = rightamp = 1.0;

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
	{
		expr_t *mutated;
		mutated = copy_expr(terrains[tindex]);
		mutate(mutated);
		if (!addterrainbyexpr(mutated))
			free_expr(mutated);
		else
			tindex = numterrains-1;
	}
	else if (newbuttons & JOYBTN_2) // mutate in-place
		mutate(terrains[tindex]);
	else if (newbuttons & JOYBTN_3)
		tindex = (tindex+numterrains-1) % numterrains;
	else if (newbuttons & JOYBTN_4)
		tindex = (tindex+1) % numterrains;

	angleturn += ANGLE_ACCEL * rotation;
	amp = DBTORAT(MAXAMPDB - (lever + 1.0) / 2.0 * AMPDBRANGE);
	if (updn != 0.0) speed *= pow(2, updn / JOYTICKS);

	pan += lr * 10.0 / JOYTICKS;
	leftamp = cos(pan * M_PI/180.0) + sin(pan * M_PI/180.0);
	rightamp = cos(pan * M_PI/180.0) - sin(pan * M_PI/180.0);
}

int delaypos = 0;

void action_writesamples(int numframes)
{
	float anglerad; // angle in radians
	float d_angle; // angle turn in radians per sample frame
	float speedsamp; // speed in units per sample frame
	int ix;
	float f[2];
	float f0[2];

	anglerad = angledeg * M_PI / 180.0;
	d_angle = angleturn * M_PI / 180.0 / RATE;
	speedsamp = speed / RATE;

	for (ix = 0; ix < numframes; ix++)
	{
		anglerad += d_angle;
		x += cos(anglerad) * speedsamp;
		y += sin(anglerad) * speedsamp;

		f0[0] = f0[1] = eval_terrain_at(x, y);
		f0[0] *= leftamp;
		f0[1] *= rightamp;
		// TODO: gain etc?

		// delay
		f[0] = f0[0] + delaybin[2*delaypos];
		f[1] = f0[1] + delaybin[2*delaypos+1];
		delaybin[2*delaypos] = delaybin[2*delaypos]*0.33+f0[0];
		delaybin[2*delaypos+1] = delaybin[2*delaypos+1]*0.33+f0[1];
		delaypos++;
		if (delaypos >= DELAYSAMPS) delaypos = 0;

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
void action_dodisplay(SDL_Surface *disp, int w, int h, int lines, int magnify)
{
	float scrtoplanemul;
	float planeleft, planetop;
	int scrx, scry;
	float f;
	float intensity; // color intensity [0, 256)
	static int row = 0;
	int endrow = (row + lines) % h;
	SDL_Rect dst;

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
		intensity = f;

		dst.x = scrx*magnify;
		dst.y = scry*magnify;
		dst.w = magnify;
		dst.h = magnify;
		SDL_FillRect(disp, &dst, SDL_MapRGB(disp->format,
			(Uint8)(MIN(intensity, 128) - 128 * 2.0),
			0,
			MAX(intensity, 192)));
	}

	row = endrow;
}
