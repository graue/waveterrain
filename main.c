/* Important fact: you hold buttons 5 and 6 to quit. */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <err.h>
#include "SDL.h"
#include "mt.h"
#include "main.h"
#include "ctimer.h"
#include "sndlib.h"
#include "action.h"

extern char *optarg;
extern int optind;
extern char *__progname;

#define BLKSZ 1024 /* desired audio blocksize */
#define VIRTWIDTH 100
#define VIRTHEIGHT 100
#define MAGNIFY 6
#define SCRWIDTH (VIRTWIDTH*MAGNIFY)
#define SCRHEIGHT (VIRTHEIGHT*MAGNIFY)
#define SCRLINES 5 /* lines to update each time */

// Joystick update tick length in microseconds (milliseconds * 1000)
#define JOYTICKLEN (1000000 / JOYTICKS)

static void usage(void)
{
	fprintf(stderr, "usage: %s [-f device]\n", __progname);
	exit(EXIT_FAILURE);
}

static int numbuttons, numaxes;
int take_joystick_input(SDL_Joystick *joy);

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Joystick *joy;
	u_int blocksize;
	int ch;
	int tickspassed = 0;
	int ticks = 0;
	int mustlock;
	u_int64_t lasttime, nowtime;

	while ((ch = getopt(argc, argv, "f:")) != -1)
	{
		if (ch != 'f') // specify alternate device, but not used
			usage();
	}

	argc -= optind;
	argv += optind;
	if (argc > 0) usage();

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0)
		errx(1, "SDL init error: %s", SDL_GetError());
	atexit(SDL_Quit);

	if ((blocksize = snd_openaudio(action_audiocallback, RATE, 2, 16,
		BLKSZ)) == 0) {
		errx(1, "cannot open audio");
	}

	joy = SDL_JoystickOpen(0);
	if (joy == NULL)
		errx(1, "SDL joystick open error: %s", SDL_GetError());
	numbuttons = SDL_JoystickNumButtons(joy);
	numaxes = SDL_JoystickNumAxes(joy);

	screen = SDL_SetVideoMode(SCRWIDTH, SCRHEIGHT, 16,
		SDL_SWSURFACE);
	if (screen == NULL)
		errx(1, "can't set video mode: %s", SDL_GetError());
	if (screen->format->BytesPerPixel != 2)
		errx(1, "set video mode is not 16-bit!");
	mustlock = SDL_MUSTLOCK(screen);
	action_init();
	lasttime = get_usecs();
	mt_init((u_int32_t)lasttime);
	SDL_PauseAudio(0);

	for (;;)
	{
		nowtime = get_usecs();
		for (ticks = 0; nowtime - lasttime > JOYTICKLEN;
			ticks++, lasttime += JOYTICKLEN)
			;

		if (ticks > 0)
			SDL_JoystickUpdate();
		tickspassed += ticks;

		while (ticks-- > 0)
			if (take_joystick_input(joy) != 0)
				goto quit;

		// Update the screen every other tick.
		if (tickspassed & 1 || ticks >= 2)
		{
			action_dodisplay(screen, VIRTWIDTH, VIRTHEIGHT,
				SCRLINES, MAGNIFY);
			SDL_UpdateRect(screen, 0, 0, 0, 0);
		}

		int64_t time_to_next_tick = lasttime + JOYTICKLEN
			- get_usecs();
		if (time_to_next_tick > 11000)
			SDL_Delay(time_to_next_tick / 1000 - 10);
	}

quit:
	snd_closeaudio();
	return 0;
}

// axes - XXX specific to my joystick
#define JOYAXIS_UPDN 1
#define JOYAXIS_LR 0
#define JOYAXIS_BASE 2
#define JOYAXIS_ROTATE 3
 
static int joybuttonstate(SDL_Joystick *joy)
{
	int state = 0;
	// XXX what if fewer than 6 buttons?
	if (SDL_JoystickGetButton(joy, 0)) state |= JOYBTN_1;
	if (SDL_JoystickGetButton(joy, 1)) state |= JOYBTN_2;
	if (SDL_JoystickGetButton(joy, 2)) state |= JOYBTN_3;
	if (SDL_JoystickGetButton(joy, 3)) state |= JOYBTN_4;
	if (SDL_JoystickGetButton(joy, 4)) state |= JOYBTN_5;
	if (SDL_JoystickGetButton(joy, 5)) state |= JOYBTN_6;
	return state;
}

// XXX may be specific to my Saitek joystick?
#define JOYAXIS_MIN -1.0
#define JOYAXIS_MAX (254.0 / 256.0) /* 0.9921875 */

static float scaledjoyaxis(SDL_Joystick *joy, int axis)
{
	float axval = SDL_JoystickGetAxis(joy, axis) / 32768.0;
	/* scale to [-1.0, 1.0] */
	if (axval < 0)
		axval = axval / JOYAXIS_MIN * -1.0;
	else
		axval = axval / JOYAXIS_MAX * 1.0;
	return axval;
}

// hold buttons 5 and 6 to quit
#define JOYBTN_QUIT (JOYBTN_5|JOYBTN_6)

// returns 1 if time to quit, 0 otherwise
int take_joystick_input(SDL_Joystick *joy)
{
	int buttons;

	buttons = joybuttonstate(joy);
	if (buttons == JOYBTN_QUIT)
		return 1;

	action_control(scaledjoyaxis(joy, JOYAXIS_ROTATE),
		scaledjoyaxis(joy, JOYAXIS_BASE),
		scaledjoyaxis(joy, JOYAXIS_UPDN),
		scaledjoyaxis(joy, JOYAXIS_LR),
		buttons);

	return 0;
}
