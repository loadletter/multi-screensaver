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

#define SPR_X (sprite_index[current_sprite].x)
#define SPR_Y (blink_eyes ? 0 : sprite_index[current_sprite].y)
#define SPR_WIDTH (sprite_index[current_sprite].width)
#define SPR_HEIGHT (sprite_index[current_sprite].height)

Display *display;
XWindowAttributes wa;
GC gc;
Window window_id;
Pixmap pixmap;
XImage *multi_img;
XImage *multi_clp;
XImage *background_img;

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
	XGetWindowAttributes(display, window_id, &wa);

	/* Capture background */
	background_img = XGetImage(display, window_id, 0, 0, wa.width, wa.height, AllPlanes, ZPixmap);
	if (!background_img)
	{
		printf ("Error reading background\n");
		exit (1);
	}
		
	/* Load multi bitmap and transparency from xpm data */
	if (XpmCreateImageFromData (display, multi_xpm, &multi_img, &multi_clp, NULL))
	{
		printf ("Error reading image\n");
		exit (1);
	}
	
	/* Check for correctness */
	assert(multi_img->width = IMAGE_WIDTH);
	assert(multi_img->height = IMAGE_HEIGHT);
	assert(multi_clp->width = IMAGE_WIDTH);
	assert(multi_clp->height = IMAGE_HEIGHT);
	
	/* Create double buffer */
	Pixmap double_buffer = XCreatePixmap(display, window_id, wa.width, wa.height, wa.depth);
	
	/* Put background */
	XPutImage(display, double_buffer, gc, background_img, 0, 0, 0, 0, background_img->width, background_img->height);
	
	/* TODO: add arc drawing and support for multiple multi */

	while(1)
	{
		unsigned int x = random() % wa.width;
		unsigned int y = random() % wa.height;
		int blink_eyes = 0;
		int i;
		
		/* Multi cleans */
		for (i=0; i < (SPRITE_NUMBER * 6); i++)
		{
			/* Get current sprite, if beginning a new cycle maybe blink her eyes */
			int current_sprite = i % SPRITE_NUMBER;
			blink_eyes = (current_sprite == 0 ? (random() % SPRITE_NUMBER == 0) : blink_eyes);
			
			/* copy the transparent image into the pixmap */
			Pixmap multi_pix = XCreatePixmap(display, double_buffer, SPRITE_WIDTH, SPRITE_HEIGHT, multi_clp->depth);
			GC multi_gc = XCreateGC(display, multi_pix, 0, NULL);
			XPutImage(display, multi_pix, multi_gc, multi_clp, SPR_X, SPR_Y, 0, 0, SPR_WIDTH, SPR_HEIGHT);
			
			/* put multi transparent image into buffer */
			XSetClipMask(display, gc, multi_pix);
			XSetClipOrigin(display, gc, x, y);
			XPutImage(display, double_buffer, gc, multi_img, SPR_X, SPR_Y, x, y, SPR_WIDTH, SPR_HEIGHT);
			
			/* copy from buffer to window */
			XSetClipMask(display, gc, None);
			XCopyArea(display, double_buffer, window_id, gc, 0, 0, wa.width, wa.height, 0, 0);
			
			usleep(1000 * 120);
			
			/* Remove this Multi */
			XPutImage(display, double_buffer, gc, background_img, x, y, x, y, SPRITE_WIDTH, SPRITE_HEIGHT);
		}
		
		/* once in a while, clear all */
		if (random() % 500 < 1)
		{
			XSetClipMask(display, gc, None);
			XPutImage(display, double_buffer, gc, background_img, 0, 0, 0, 0, background_img->width, background_img->height);
			XCopyArea(display, double_buffer, window_id, gc, 0, 0, wa.width, wa.height, 0, 0);
		}
		
		usleep(1000);
	}
}
