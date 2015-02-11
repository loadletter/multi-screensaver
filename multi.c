#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include "multi.xpm"

#define IMAGE_WIDTH 240
#define IMAGE_HEIGTH 192

#define SPRITE_WIDTH (IMAGE_WIDTH / 3)
#define SPRITE_HEIGTH (IMAGE_HEIGTH / 2)
#define SPRITE_NUMBER 6

typedef struct {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
} SpriteXY;

SpriteXY sprite_index[SPRITE_NUMBER] = {
	{0,					SPRITE_HEIGTH,	SPRITE_WIDTH,	SPRITE_HEIGTH},
	{SPRITE_WIDTH,		SPRITE_HEIGTH,	SPRITE_WIDTH,	SPRITE_HEIGTH},
	{SPRITE_WIDTH * 2,	SPRITE_HEIGTH,	SPRITE_WIDTH,	SPRITE_HEIGTH},

	{SPRITE_WIDTH * 2,	SPRITE_HEIGTH,	SPRITE_WIDTH,	SPRITE_HEIGTH},
	{SPRITE_WIDTH,		SPRITE_HEIGTH,	SPRITE_WIDTH,	SPRITE_HEIGTH},
	{0,					SPRITE_HEIGTH,	SPRITE_WIDTH,	SPRITE_HEIGTH},
};

#define SPR_X (sprite_index[current_sprite].x)
#define SPR_Y (blink_eyes ? 0 : sprite_index[current_sprite].y)
#define SPR_WIDTH (sprite_index[current_sprite].width)
#define SPR_HEIGHT (sprite_index[current_sprite].height)


Display *display;
GC gc;
Window window_id;
Pixmap pixmap;
int width;
int height;
int depth;
XImage *multi_img;
XImage *multi_clp;
XImage *background_img;



void get_dimensions()
{
	long dummy_id;
	int dummy;
	XGetGeometry(display, window_id, &dummy_id, &dummy, &dummy, &width, &height, &dummy, &depth);
}


int main()
{
	printf("Welcome!\n");
	display = XOpenDisplay(NULL);
	srand(time(NULL));
	
	char *xscreen_id = getenv("XSCREENSAVER_WINDOW");
	if(!xscreen_id)
	{
		printf("Couldn't find window from $XSCREENSAVER_WINDOW\n");
		exit(1);
	}
	window_id = strtol(xscreen_id, 0, 16);
	
	gc = XCreateGC(display, window_id, 0, NULL);
	
	/* Get window dimension */
	get_dimensions();	

	/* Capture background */
	background_img = XGetImage(display, window_id, 0, 0, width, height, AllPlanes, ZPixmap);
	if (!background_img)
	{
		printf ("Error reading background\n");
		exit (1);
	}
		
	/* Load multi bitmap and transparency from xpm data */
	if (XpmCreateImageFromData  (display, multi_xpm, &multi_img, &multi_clp, NULL))
	{
		printf ("Error reading image\n");
		exit (1);
	}
	
	/* Check for correctness */
	assert(multi_img->width = IMAGE_WIDTH);
	assert(multi_img->height = IMAGE_HEIGTH);
	assert(multi_clp->width = IMAGE_WIDTH);
	assert(multi_clp->height = IMAGE_HEIGTH);
	
	/* Put background */
	XPutImage(display, window_id, gc, background_img, 0, 0, 0, 0,
				background_img->width, background_img->height);

	while(1)
	{
		unsigned int x = random() % width;
		unsigned int y = random() % height;
		int blink_eyes = 0;
		int i;
		
		/* Multi cleans */
		for (i=0; i < (SPRITE_NUMBER * 6); i++)
		{
			/* Get current sprite, if beginning a new cycle maybe blink her eyes */
			int current_sprite = i % SPRITE_NUMBER;
			blink_eyes = (current_sprite == 0 ? (random() % SPRITE_NUMBER == 0) : blink_eyes);
			
			/* copy the transparent image into the pixmap */
			Pixmap multi_pix = XCreatePixmap(display, window_id, SPRITE_WIDTH, SPRITE_HEIGTH, multi_clp->depth);
			GC multi_gc = XCreateGC(display, multi_pix, 0, NULL);
			XPutImage(display, multi_pix, multi_gc, multi_clp, SPR_X, SPR_Y, 0, 0, SPR_WIDTH, SPR_HEIGHT);
			
			/* put multi transparent image in random places */
			XSetClipMask(display, gc, multi_pix);
			XSetClipOrigin(display, gc, x, y);
			XPutImage(display, window_id, gc, multi_img, SPR_X, SPR_Y, x, y, SPR_WIDTH, SPR_HEIGHT);
			
			usleep(1000 * 120);
			
			/* Remove this Multi */
			XPutImage(display, window_id, gc, background_img, x, y, x, y, SPRITE_WIDTH, SPRITE_HEIGTH);
		}
		
		/* once in a while, clear all */
		if (random() % 500 < 1)
		{
			XSetClipMask(display, gc, None);
			XClearWindow(display, window_id);
			XPutImage(display, window_id, gc, background_img, 0, 0, 0, 0, background_img->width, background_img->height);
		}
		
		usleep(1000);
	}
}
