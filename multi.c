#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include "multi.xpm"

Display *display;
GC gc;
Window window_id;
Pixmap pixmap;
int width;
int height;
int depth;
XImage *multi_img;
XImage *background_img;



void get_dimensions()
{
	long dummy_id;
	int dummy;
	XGetGeometry(display, window_id, &dummy_id,
			  &dummy, &dummy, &width, &height, &dummy, &depth);
}

void xclear()
{
	XSetForeground(display, gc, XBlackPixel(display, 0));
	XFillRectangle(display, pixmap, gc, 0,0, width, height);
	XSetForeground(display, gc, XWhitePixel(display, 0));
}

void xcopy()
{
	XCopyArea(display, pixmap, window_id, gc, 0, 0, width, height, 0, 0);
	XFlush(display);
}

void color(char *color, XColor *result)
{
	XParseColor(display, DefaultColormap(display,0), color, result);
	XAllocColor(display, DefaultColormap(display,0), result);
}

int main()
{
	printf("Welcome!\n");
	display = XOpenDisplay(NULL);
	srand(time(NULL));
	char *xscreen_id = getenv("XSCREENSAVER_WINDOW");
	if(xscreen_id)
	{
		window_id = strtol(xscreen_id, 0, 16);
		gc = XCreateGC(display, window_id, 0, NULL);
	}
	else
	{
		printf("Couldn't find window from $XSCREENSAVER_WINDOW, creating one\n");
		window_id = RootWindow(display, 0);
		gc = XCreateGC(display, window_id, 0, NULL);
		window_id = XCreateSimpleWindow(display, window_id, 0, 0, 840, 640, 0, 0, WhitePixel(display, 0));
		XMapWindow(display, window_id);
	}
	//XSetForeground(display, gc, XWhitePixel(display, 0));
	
	/* Get window dimension */
	get_dimensions();

	/* Capture background */
	background_img = XGetImage(display, window_id, 0, 0, width, height, AllPlanes, ZPixmap);
	if (!background_img)
	{
		printf ("Error reading background\n");
		exit (1);
	}
		
	/* Load multi bitmap from xpm data */
	if (XpmCreateImageFromData  (display, multi_xpm, &multi_img, NULL, NULL))
	{
		printf ("Error reading image\n");
		exit (1);
	}
	
	/* Put background */
	XPutImage(display, window_id, gc, background_img, 0, 0, 0, 0,
				background_img->width, background_img->height);

	while(1)
	{
		/* put multi bitmap in random places */
		XPutImage(display, window_id, gc, multi_img, 0, 0,
				random() % width,
				random() % height,
				multi_img->width, multi_img->height);
		
		/* once in a while, clear all */
		if (random() % 500 < 1)
		{
			XClearWindow(display, window_id);
			XPutImage(display, window_id, gc, background_img, 0, 0, 0, 0,
				background_img->width, background_img->height);
		}
		
		usleep(1000);
		
	}
}
