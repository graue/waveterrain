#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <sys/time.h>
#include <err.h>
#include <assert.h>

#define DSP "/dev/audio"

static unsigned char *buf;
static u_int bufsize, bufused;
static u_int encoding, bytespersample, numchannels;
static double usecspersample;
static audio_info_t info;
static int fake = 0;

/*
 * File descriptor for the audio device we're writing to. -1 means audio
 * has not been initialized.
 */
int sndlib_fd = -1;

static FILE *dump = NULL;

void snd_stopdump(void)
{
	if (dump != NULL)
		fclose(dump);
	dump = NULL;
}

void snd_startdump(const char *filename)
{
	if (dump != NULL)
		fclose(dump);
	dump = fopen(filename, "wb");
}

/* Close audio output; return nonzero on success, 0 on failure. */
int snd_closeaudio(void)
{
	assert(sndlib_fd != -1);

	if (close(sndlib_fd) == -1)
		return 0;
	sndlib_fd = -1;

	free(buf);
	buf = NULL;
	snd_stopdump();

	return 1;
}

/*
 * Open audio output with the desired parameters. Return block size
 * in sample frames on success, 0 on failure.
 *
 * The block size, specified in sample frames, may be changed, but
 * everything else will be exactly as specified if the call succeeds.
 *
 * devname can be NULL to use the default device.
 *
 * blocksize can be passed as 0 to use the default size.
 */
u_int snd_openaudio(const char *devname, u_int samplerate, u_int channels,
	u_int precision, u_int blocksize)
{
	int status;

	assert(precision == 8 || precision == 16);
	assert(samplerate > 0);

	/* If audio is already open, close it first. */
	if (sndlib_fd != -1 && snd_closeaudio() == -1)
		return 0;

	if (devname == NULL) devname = DSP;
	fake = strcmp(devname, "/dev/null") == 0;

	AUDIO_INITINFO(&info);

	sndlib_fd = open(devname, O_WRONLY);
	if (sndlib_fd == -1) return 0;

	info.mode = AUMODE_PLAY;
	info.play.sample_rate = samplerate;
	info.play.channels = channels;
	info.play.precision = precision;
	info.play.encoding = fake ? AUDIO_ENCODING_SLINEAR_LE
		: AUDIO_ENCODING_SLINEAR;

	bytespersample = precision / 8;
	usecspersample = (double)1000000 / samplerate;

	/* Set the block size to "blocksize" sample frames. */
	info.blocksize = bytespersample * channels * blocksize;

	/* Make the buffer size low, in effect. */
	info.hiwat = 8;

	if (!fake && (status = ioctl(sndlib_fd, AUDIO_SETINFO, &info)) == -1)
	{
		snd_closeaudio();
		return 0;
	}

	if (!fake && (status = ioctl(sndlib_fd, AUDIO_GETINFO, &info)) == -1)
	{
		snd_closeaudio();
		return 0;
	}

	encoding = info.play.encoding;

	if (!fake && (info.play.sample_rate != samplerate
		|| info.play.channels != channels
		|| info.play.precision != precision
		|| (encoding != AUDIO_ENCODING_SLINEAR_LE
			&& encoding != AUDIO_ENCODING_SLINEAR_BE)))
	{
		/* Couldn't get the desired format. */
		snd_closeaudio();
		return 0;
	}

	bufsize = info.blocksize;

	if (bufsize % (bytespersample * channels) != 0
		|| (buf = malloc(bufsize)) == NULL)
	{
		snd_closeaudio();
		return 0;
	}

	bufused = 0;
	numchannels = channels;

	return bufsize / (bytespersample * channels);
}

/* Write a sample. Warning: This call may block! */
void snd_writesample(double smp)
{
	assert(sndlib_fd != -1);

	if (smp > 32767.0f)
		smp = 32767.0f;
	else if (smp < -32768.0f)
		smp = -32768.0f;

	if (bufused == bufsize)
	{
		char *p = buf;
		int bytesleft = bufsize, written;
		while (bytesleft > 0
			&& (written = write(sndlib_fd, p, bytesleft)) != -1)
		{
			bytesleft -= written;
			p += written;
		}
		bufused = 0;
		if (dump != NULL)
			fwrite(buf, 1, bufsize, dump);
	}

	if (bytespersample == 1)
		buf[bufused++] = (unsigned char)(int8_t)smp;
	else
	{
		int16_t conv;
		assert(bytespersample == 2);
		if (encoding == AUDIO_ENCODING_SLINEAR_LE)
			conv = (int16_t)htole16((u_int16_t)(int16_t)smp);
		else /* (encoding == AUDIO_ENCODING_SLINEAR_BE) */
			conv = (int16_t)htobe16((u_int16_t)(int16_t)smp);
		buf[bufused++] = *((unsigned char *)&conv);
		buf[bufused++] = *(((unsigned char *)&conv)+1);
	}
}

/* Returns 1 if writing a sample will block at the moment, 0 if not. */
int snd_writewillblock(void)
{
	struct pollfd pfd[1];
	int nfds;

	assert(sndlib_fd != -1);
	if (bufused < bufsize)
		return 0;

	pfd[0].fd = sndlib_fd;
	pfd[0].events = POLLOUT;

	nfds = poll(pfd, 1, 0);
	if (nfds < 1 || pfd[0].revents & (POLLERR|POLLHUP|POLLNVAL))
		return 1;

	return 0;
}

/*
 * Returns a number of sample frames you can write while being sure none
 * of the writes will block.
 */
int snd_space(void)
{
	return (bufsize - bufused) / (bytespersample * numchannels);
}
