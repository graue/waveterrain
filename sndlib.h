#ifdef SNDLIB_H
#error "multiple include"
#endif
#define SNDLIB_H

int snd_closeaudio(void);
unsigned int snd_openaudio(const char *devname, unsigned int samplerate,
	unsigned int channels, unsigned int precision,
	unsigned int blocksize);
void snd_writesample(double smp);
int snd_writewillblock(void);
int snd_space(void);
void snd_startdump(const char *filename);
void snd_stopdump(void);

extern int sndlib_fd;
