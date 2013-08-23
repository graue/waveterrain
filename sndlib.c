/* Originally an OpenBSD sound lib, now just a wrapper around SDL's annoying
 * SDL_OpenAudio() function, and for symmetry, its close function
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include "SDL.h"
#include "sndlib.h"
#include "xm.h"

int snd_closeaudio(void) {
	SDL_CloseAudio();
	return 1;
}

/* Returns the block size obtained. */
unsigned int snd_openaudio(callback_t callback, unsigned int samplerate,
	unsigned int channels, unsigned int precision,
	unsigned int blocksize) {
	SDL_AudioSpec *desired = xm(sizeof (SDL_AudioSpec), 1);
	SDL_AudioSpec *obtained = xm(sizeof (SDL_AudioSpec), 1);
	unsigned int actualblocksize;

	desired->freq = samplerate;
	assert(precision == 16);  // Better be 16, that's all we support.
	desired->format = AUDIO_S16;
	assert(channels == 2);  // Also the only value supported.
	desired->channels = 2;
	desired->samples = blocksize;
	desired->callback = callback;
	desired->userdata = NULL;

	if (SDL_OpenAudio(desired, obtained) < 0)
		errx(1, "Couldn't open audio: %s", SDL_GetError());

	if (obtained->channels != channels
		|| obtained->format != desired->format
		|| obtained->freq != desired->freq) {
		errx(1, "Desired %d channels, precision %d, rate %d, "
			"got %d channels, format %d, rate %d",
			channels, precision, samplerate,
			obtained->channels, obtained->format, obtained->freq);
	}
	free(desired);
	actualblocksize = obtained->samples;
	free(obtained);

	return actualblocksize;
}
