void action_init(void);
void action_writesamples(int numframes);
void SDLCALL action_audiocallback(void *userdata, unsigned char *stream,
	int len);
void action_control(float rotation, float lever, float updn, float lr,
	int buttons);
void action_dodisplay(SDL_Surface *disp, int w, int h, int lines, int magnify);
