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

#define IMAGE_WIDTH 240
#define IMAGE_HEIGHT 192

#define SPRITE_WIDTH (IMAGE_WIDTH / 3)
#define SPRITE_HEIGHT (IMAGE_HEIGHT / 2)
#define SPRITE_NUMBER 6
#define SPRITE_REPEAT 6

typedef struct {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
} SpriteXY;

SpriteXY sprite_index[SPRITE_NUMBER] = {
	{0,					SPRITE_HEIGHT,	SPRITE_WIDTH,	SPRITE_HEIGHT},
	{SPRITE_WIDTH,		SPRITE_HEIGHT,	SPRITE_WIDTH,	SPRITE_HEIGHT},
	{SPRITE_WIDTH * 2,	SPRITE_HEIGHT,	SPRITE_WIDTH,	SPRITE_HEIGHT},

	{SPRITE_WIDTH * 2,	SPRITE_HEIGHT,	SPRITE_WIDTH,	SPRITE_HEIGHT},
	{SPRITE_WIDTH,		SPRITE_HEIGHT,	SPRITE_WIDTH,	SPRITE_HEIGHT},
	{0,					SPRITE_HEIGHT,	SPRITE_WIDTH,	SPRITE_HEIGHT},
};

/* Get sprite coordinates, switch to lower sprites if blinking */
#define SPR_X (sprite_index[current_sprite].x)
#define SPR_Y (st->multi[m]->blink ? 0 : sprite_index[current_sprite].y)
#define SPR_WIDTH (sprite_index[current_sprite].width)
#define SPR_HEIGHT (sprite_index[current_sprite].height)
/* Calculate offset and radius */
#define CIRCLE_Y ((st->multi[m]->y) + (SPRITE_HEIGHT - 4) - (st->multi[m]->circle_size / 2))
#define CIRCLE_X ((st->multi[m]->x) + (15) - (st->multi[m]->circle_size / 2))


typedef struct {
	unsigned int x;
	unsigned int y;
	unsigned int circle_size;
	int blink;
	int i;
	XImage *img;
	XImage *clp;
	Pixmap pix;
	GC gc;
} MultiState;

typedef struct {
	unsigned char number;
	MultiState *multi[10];
	Display *display;
	Window window;
	XWindowAttributes wa;
	GC gc;
	XImage *bg_img;
	Pixmap double_buf;
	Pixmap bg_pix;	
} SaverState;


static SaverState *screen_init(unsigned char multi_number)
{
	printf("Welcome!\n");
	srand(time(NULL));
	
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
	
	st->gc = XCreateGC(st->display, st->window, 0, NULL);
	
	/* Set drawing color */
	XSetForeground(st->display, st->gc, XBlackPixel(st->display, 0));
	
	/* Get window dimension */
	XGetWindowAttributes(st->display, st->window, &(st->wa));

	/* Capture background */
	/* TODO: maybe DefaultRootWindow(display) instead of window_id */
	st->bg_img = XGetImage(st->display, st->window, 0, 0, st->wa.width, st->wa.height, AllPlanes, ZPixmap);
	if (!st->bg_img)
	{
		printf ("Error reading background\n");
		exit (1);
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
	int i;
	if (XpmCreateImageFromData(st->display, multi_xpm, &multi_img, &multi_clp, NULL))
	{
		printf ("Error reading image\n");
		exit (1);
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
	int m;
	for (m=0; m<(st->number); m++)
	{
		/* Reinitilize if completed a cycle */
		if (st->multi[m]->i >= (SPRITE_NUMBER * SPRITE_REPEAT) || st->multi[m]->circle_size == 0)
		{
			/* If there are more than one Multis start them at random so that they behave differently
			 * (minus the first which is privileged) */
			if (st->number > 1 && st->multi[m]->circle_size == 0 && m != 0 && (random() % (SPRITE_REPEAT * 2) != 0))
				continue;
			st->multi[m]->i = 0;
			st->multi[m]->x = random() % st->wa.width;
			st->multi[m]->y = random() % st->wa.height;
			st->multi[m]->blink = 0;
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
		st->multi[m]->circle_size += 2;
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
		st->multi[m]->i += 1;
	}
	
	/* Copy from buffer to window */
	XSetClipMask(st->display, st->gc, None);
	XCopyArea(st->display, st->double_buf, st->window, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
	
	/* Restore background pixmap to buffer */
	XCopyArea(st->display, st->bg_pix, st->double_buf, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
}

static void screen_revert(SaverState *st)
{
	XSetClipMask(st->display, st->gc, None);
	XPutImage(st->display, st->bg_pix, st->gc, st->bg_img, 0, 0, 0, 0, st->bg_img->width, st->bg_img->height);
	XCopyArea(st->display, st->bg_pix, st->double_buf, st->gc, 0, 0, st->wa.width, st->wa.height, 0, 0);
}

int main()
{
	SaverState *st = screen_init(4);
	
	/* TODO:
	 * find out how to set dontClearRoot for xscreensaver
	 * copy some macros from xscreensaver (screenhack.h, screenhacki.h ) to build as an xsceensaver module
	 * copy some settings stuff (XrmGetResource from resources.c) */

	while(1)
	{
		/* Multi cleans */
		run_cycle(st);
		
		/* Sleep */
		usleep(1000 * 120);
		
		/* Once in a while, clear all */
		if (random() % 5000 < 1)
		{
			screen_revert(st);
		}
	}
}
