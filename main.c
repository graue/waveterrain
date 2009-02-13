#include <stdlib.h>
#include <sys/types.h>
#include <poll.h>
#include <err.h>
#include "SDL.h"
#include "main.h"
#include "ctimer.h"
#include "sndlib.h"
#include "action.h"

extern char *optarg;
extern int optind;
extern char *__progname;

#define BLKSZ 512 /* desired audio blocksize */
#define SCRWIDTH 600
#define SCRHEIGHT 600

// Joystick update ticks per second; display is updated every other tick
#define JOYTICKS 100 /* = 10 ms */

// Joystick update tick length in microseconds (milliseconds * 1000)
#define JOYTICKLEN (1000000 / JOYTICKS)

// max rotate speed in +/- degrees per second
#define MAX_ROTATE_SPEED 360.0

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
	struct pollfd pfd;
	int ch;
	int tickspassed = 0;
	int ticks = 0;
	int mustlock;
	const char *device = NULL;
	u_int64_t lasttime, nowtime;

	while ((ch = getopt(argc, argv, "f:")) != -1)
	{
		if (ch == 'f') // specify alternate device
			device = optarg;
		else usage();
	}

	argc -= optind;
	argv += optind;
	if (argc > 0) usage();

	if ((blocksize = snd_openaudio(device, RATE, 2, 16, BLKSZ)) == 0)
		errx(1, "cannot open audio");

	pfd.fd = sndlib_fd;
	pfd.events = POLLOUT;

	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0)
		errx(1, "SDL init error: %s", SDL_GetError());
	atexit(SDL_Quit);
	joy = SDL_JoystickOpen(0);
	if (joy == NULL)
		errx(1, "SDL joystick open error: %s", SDL_GetError());
	numbuttons = SDL_JoystickNumButtons(joy);
	numaxes = SDL_JoystickNumAxes(joy);

	screen = SDL_SetVideoMode(SCRWIDTH, SCRHEIGHT, 16, SDL_SWSURFACE);
	if (screen == NULL)
		errx(1, "can't set video mode: %s", SDL_GetError());
	if (screen->format->BytesPerPixel != 2)
		errx(1, "set video mode is not 16-bit!");
	mustlock = SDL_MUSTLOCK(screen);
	action_init();
	lasttime = get_usecs();

	for (;;)
	{
		if (poll(&pfd, 1, 0) == -1
			|| pfd.revents & (POLLERR|POLLHUP|POLLNVAL))
		{
			err(1, "poll");
		}

		if (pfd.revents & POLLOUT)
		{
			int numsamps = snd_space();
			if (numsamps == 0 && !snd_writewillblock())
				numsamps = blocksize;
			action_writesamples(sndlib_fd, numsamps);
		}

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

		if (tickspassed & 1)
			action_dodisplay(screen, SCRWIDTH, SCRHEIGHT);
	}

quit:
	snd_closeaudio();
	return 0;
}

// returns 1 if time to quit, 0 otherwise
int take_joystick_input(SDL_Joystick *joy)
{
	// TODO
	return 0;
}
