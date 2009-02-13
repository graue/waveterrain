#include <math.h>
#include "SDL.h"
#include "expr.h"
#include "parse.h"

#define M_20_OVER_LN10 8.68588963806503655302257838
#define M_LN10_OVER_20 0.115129254649702284200899573
#define RATTODB(x) (log(x) * M_20_OVER_LN10)
#define DBTORAT(x) exp((x) * M_LN10_OVER_20)
#define DC_OFFSET 1.19209290E-07F /* <float.h>'s FLT_EPSILON */

void action_init(void)
{
	// TODO: init expressions etc.
}

void action_control(float rotation)
{
	// TODO
}

void action_writesamples(int fd, int numframes)
{
	// TODO
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
