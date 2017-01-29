#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>

#include "multi.xpm"

#ifdef USE_XSCREENSAVER
#include "xscreensave.h"
#endif

#define IMAGE_WIDTH 240
#define IMAGE_HEIGHT 384
/* Sprites */
#define SPRITE_WIDTH (IMAGE_WIDTH / 3)
#define SPRITE_HEIGHT (IMAGE_HEIGHT / 4)
#define SPRITE_NUMBER 6
#define SPRITE_REPEAT 10
#define MAX_MULTI 12
/* Get sprite coordinates, switch to lower sprites if blinking */
#define SPR_X (ani_seq[current_sprite])
#define SPR_Y ((st->multi[m]->blink ? 0 : SPRITE_HEIGHT) + (st->multi[m]->reverse ? (SPRITE_HEIGHT * 2) : 0))
#define SPR_WIDTH (SPRITE_WIDTH)
#define SPR_HEIGHT (SPRITE_HEIGHT)
/* Calculate offset and radius */
#define CIRCLE_Y ((st->multi[m]->y) + (SPRITE_HEIGHT - 4) - (st->multi[m]->circle_size / 2))
#define CIRCLE_X ((st->multi[m]->x) + (st->multi[m]->reverse ? (SPRITE_WIDTH - 15) : 15) - (st->multi[m]->circle_size / 2))

typedef struct {
	unsigned int x;
	unsigned int y;
	unsigned int circle_size;
	int blink;
	int reverse;
	int i;
	XImage *img;
	XImage *clp;
	Pixmap pix;
	GC gc;
} MultiState;

typedef struct {
	unsigned int number;
	MultiState *multi[MAX_MULTI];
	Display *display;
	Window window;
	XWindowAttributes wa;
	GC gc;
	XImage *bg_img;
	Pixmap double_buf;
	Pixmap bg_pix;	
} SaverState;

/* Animation sequence */
int ani_seq[SPRITE_NUMBER] = {0, SPRITE_WIDTH, SPRITE_WIDTH * 2, SPRITE_WIDTH * 2, SPRITE_WIDTH, 0};

static SaverState *screen_init(unsigned int multi_number, char dont_getimage)
{
	printf("Welcome!\n");
	srandom(time(NULL));
	
	SaverState *st = malloc(sizeof(SaverState));
	if(!st)
	{
		printf("Couldn't malloc saverstate\n");
		exit(1);
	}
	st->display = XOpenDisplay(NULL);
	st->number = multi_number;
	
	/* Get window id from env variable */
	char *xscreen_id = getenv("XSCREENSAVER_WINDOW");
	if(!xscreen_id)
	{
		printf("Couldn't find window from $XSCREENSAVER_WINDOW\n");
		exit(1);
	}
	st->window = strtol(xscreen_id, 0, 16);
	
	if (!dont_getimage)
	{
#ifdef USE_XSCREENSAVER
	grab_screen_image(xscreen_id);
#endif
	}

	st->gc = XCreateGC(st->display, st->window, 0, NULL);
	
	/* Set drawing color */
	XSetForeground(st->display, st->gc, XBlackPixel(st->display, 0));
	
	/* Get window dimension */
	XGetWindowAttributes(st->display, st->window, &(st->wa));

	/* Capture background */
	st->bg_img = XGetImage(st->display, st->window, 0, 0, st->wa.width, st->wa.height, AllPlanes, ZPixmap);
	if (!st->bg_img)
	{
		printf("Error reading background\n");
		exit(1);
	}

	/* Create double buffer */
	st->double_buf = XCreatePixmap(st->display, st->window, st->wa.width, st->wa.height, st->wa.depth);

	/* Create pixmap to store background with holes */
	st->bg_pix = XCreatePixmap(st->display, st->double_buf, st->wa.width, st->wa.height, st->wa.depth);
	XPutImage(st->display, st->bg_pix, st->gc, st->bg_img, 0, 0, 0, 0, st->bg_img->width, st->bg_img->height);
	XCopyArea(st->display, st->bg_pix, st->double_buf, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
	
	/* Load multi bitmap and transparency from xpm data */
	XImage *multi_img;
	XImage *multi_clp;
	unsigned int i;
	if (XpmCreateImageFromData(st->display, multi_xpm, &multi_img, &multi_clp, NULL))
	{
		printf("Error reading image\n");
		exit(1);
	}
	for (i=0; i<multi_number; i++)
	{
		st->multi[i] = malloc(sizeof(MultiState));
		if (!st->multi[i])
		{
			printf("Couldn't malloc multistate %d\n", i);
			exit(1);
		}
		st->multi[i]->img = multi_img;
		st->multi[i]->clp = multi_clp;
		st->multi[i]->pix = XCreatePixmap(st->display, st->double_buf, SPRITE_WIDTH, SPRITE_HEIGHT, st->multi[i]->clp->depth);
		st->multi[i]->gc = XCreateGC(st->display, st->multi[i]->pix, 0, NULL);
		st->multi[i]->i = 0;
		st->multi[i]->circle_size = 0;
	}
	
	/* Check for correctness */
	assert(multi_img->width = IMAGE_WIDTH);	assert(multi_img->height = IMAGE_HEIGHT);
	
	return st;
}

static void run_cycle(SaverState *st)
{
	unsigned int m;
	for (m=0; m<(st->number); m++)
	{
		/* Reinitilize if completed a cycle */
		if (st->multi[m]->i >= (SPRITE_NUMBER * SPRITE_REPEAT) || st->multi[m]->circle_size == 0)
		{
			/* If there are more than one Multi start them at random so that they behave differently
			 * (minus the first which is privileged) */
			if (st->number > 1 && st->multi[m]->circle_size == 0 && m != 0 && (random() % (SPRITE_REPEAT * 2) != 0))
				continue;
			st->multi[m]->i = 0;
			st->multi[m]->x = random() % st->wa.width;
			st->multi[m]->y = random() % st->wa.height;
			st->multi[m]->blink = 0;
			st->multi[m]->reverse = (random() % 2 == 0) ? (random() % 2 == 0) : st->multi[m]->reverse;
			st->multi[m]->circle_size = 20;
		}
		
		/* Get current sprite, if beginning a new cycle maybe blink her eyes */
		int current_sprite = st->multi[m]->i % SPRITE_NUMBER;
		st->multi[m]->blink = (current_sprite == 0 ? (random() % SPRITE_REPEAT == 0) : st->multi[m]->blink);
	}

	for (m=0; m<(st->number); m++)
	{
		if (st->multi[m]->circle_size == 0)
			continue;
		/* Draw arc */
		XFillArc(st->display, st->bg_pix, st->gc, CIRCLE_X, CIRCLE_Y, st->multi[m]->circle_size, st->multi[m]->circle_size, 0, 360 * 64);
		st->multi[m]->circle_size += 4;
	}
	
	for (m=0; m<(st->number); m++)
	{
		if (st->multi[m]->circle_size == 0)
			continue;
		/* Copy the transparent image into the pixmap */
		int current_sprite = st->multi[m]->i % SPRITE_NUMBER;
		XPutImage(st->display, st->multi[m]->pix, st->multi[m]->gc, st->multi[m]->clp, SPR_X, SPR_Y, 0, 0, SPR_WIDTH, SPR_HEIGHT);
		
		/* Set mask and put multi transparent image into buffer */
		XSetClipMask(st->display, st->gc, st->multi[m]->pix);
		XSetClipOrigin(st->display, st->gc, st->multi[m]->x, st->multi[m]->y);
		XPutImage(st->display, st->double_buf, st->gc, st->multi[m]->img, SPR_X, SPR_Y, st->multi[m]->x, st->multi[m]->y, SPR_WIDTH, SPR_HEIGHT);
		st->multi[m]->i += 2;
	}
	
	/* Copy from buffer to window */
	XSetClipMask(st->display, st->gc, None);
	XCopyArea(st->display, st->double_buf, st->window, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
	
	/* Restore background pixmap to buffer */
	XCopyArea(st->display, st->bg_pix, st->double_buf, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
}

static void screen_revert(SaverState *st)
{
	/* Overwrite background with original background */
	XSetClipMask(st->display, st->gc, None);
	XPutImage(st->display, st->bg_pix, st->gc, st->bg_img, 0, 0, 0, 0, st->bg_img->width, st->bg_img->height);
	XCopyArea(st->display, st->bg_pix, st->double_buf, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
}

int main(int argc, char **argv)
{
	unsigned int multi_number = 1;
	char reset = 0;
	char dont_getimage = 0;
	int c;
	while ((c = getopt (argc, argv, "drn:")) != -1)
		switch(c)
		{
			case 'r':
				reset = 1;
				break;
			case 'd':
				dont_getimage = 1;
				break;
			case 'n':
				multi_number = atoi(optarg);
				multi_number = !(multi_number > 0 && multi_number <= MAX_MULTI) ? 1 : multi_number;
				break;
			case '?':
				if (optopt == 'n')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				fprintf(stderr, "Usage: %s [-n NUM] [-r]\n", argv[0]);
				fprintf(stderr, "-n NUM: sets the number of multi, must be between 1 and %i\n", MAX_MULTI);
				fprintf(stderr, "-r: randomly restore the screen so multi has more to work\n");
#ifdef USE_XSCREENSAVER
				fprintf(stderr, "-d: dont invoke xscreensaver-getimage for the background\n");
#endif
				exit(1);
			default:
				abort();
		}
	
	SaverState *st = screen_init(multi_number, dont_getimage);

	while(1)
	{
		/* Multi mops */
		run_cycle(st);
		
		/* Sleep */
		usleep(1000 * 130);
		
		/* Once in a while, clear all */
		if (reset && random() % 5000 < 1)
		{
			screen_revert(st);
		}
	}
}
