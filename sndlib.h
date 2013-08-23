#ifdef SNDLIB_H
#error "multiple include"
#endif
#define SNDLIB_H

typedef void (SDLCALL *callback_t)(void *userdata, Uint8 *stream, int len);

int snd_closeaudio(void);
unsigned int snd_openaudio(callback_t callback, unsigned int samplerate,
	unsigned int channels, unsigned int precision,
	unsigned int blocksize);
