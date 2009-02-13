#include <stdlib.h>
#include <sys/types.h>
#include <poll.h>
#include <err.h>
#include "SDL.h"
#include "main.h"
#include "sndlib.h"
#include "action.h"

extern char *optarg;
extern int optind;
extern char *__progname;

#define BLKSZ 512 /* desired audio blocksize */
#define SCRWIDTH 600
#define SCRHEIGHT 600

static void usage(void)
{
	fprintf(stderr, "usage: %s [-f device]\n", __progname);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	u_int blocksize;
	struct pollfd pfd;
	int ch;
	int mustlock;
	const char *device = NULL;

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

	screen = SDL_SetVideoMode(SCRWIDTH, SCRHEIGHT, 16, SDL_SWSURFACE);
	if (screen == NULL)
		errx(1, "can't set video mode: %s", SDL_GetError());
	if (screen->format->BytesPerPixel != 2)
		errx(1, "set video mode is not 16-bit!");
	mustlock = SDL_MUSTLOCK(screen);
	action_init();

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

		// TODO: read input (sometimes?)
		// TODO: update display (sometimes?)
	}

	snd_closeaudio();
	return 0;
}
