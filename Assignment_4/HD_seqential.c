#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#define ROOM_TEMP 20
#define FIRE_TEMP 100
#define legal(x, n) ( (x)>=0 && (x)<(n) )

typedef struct TemperatureField
{
	int x, y;
	double **t;
	double *storage;
} TemperatureField;

int iteration,x,y;
TemperatureField *field, *tempField, *swapField;
int terminate;
double error;
double EPSILON;

int dx[4] = {0, -1, 0, 1};
int dy[4] = {1, 0, -1, 0};
unsigned long color[20] = {0X6666CC,0X8A2BE2,0X7B68EE,0X6495ED,0X1E90FF,0X87CEFA,0X00FFFF,
					0X40E0D0,0X7FFFAA,0X3CB371,0X2E8B57,0X556B2F,0X808000,
					0XBDB76B,0XF0E68C,0XFFD700,0XFF6347,0XFF4500,0XF08080,
					0XB22222};

Window win;                            	   /* initialization for a window */
unsigned int width, height,                /* window size */
	       border_width,                   /* border width in pixels */
	       idth, display_height,           /* size of screen */
	       screen;                         /* which screen */

char *window_name = "Temperature Simulation", *display_name = NULL;
GC gc;
unsigned long valuemask = 0;
XGCValues values;
Display *display;
XSizeHints size_hints;
Pixmap bitmap;
FILE *fp, *fopen ();	
Colormap default_cmap;

int min(int x, int y){ if (x<y) return x; return y; }

void deleteField(TemperatureField *field) {
	free(field->t);
	free(field->storage);
}

void newField(TemperatureField *field, int x, int y, int sourceX, int sourceY)
{
	TemperatureField temp = *field;
	field->storage = malloc( sizeof(double) * x * y );
	field->t = malloc( sizeof(double*) * x );
	field->x = x;
	field->y = y;
	int i, j;
	for (i=0; i<x; ++i)
		field->t[i] = &field->storage[i*y];
	if (sourceX)
	{
		double scaleFactorX = (double)sourceX/x;
		double scaleFactorY = (double)sourceY/y;
		for (i=0; i<x; ++i)
			for (j=0; j<y; ++j)
				field->t[i][j] = temp.t[(int)(i*scaleFactorX)][(int)(j*scaleFactorY)];
		deleteField(&temp);
	}
	else memset(field->storage, 0, sizeof(double)*x*y);
}

void initField(TemperatureField *field)
{
	int i, j;
	for (i=0; i<field->x; ++i)
		for (j=0; j<field->y; ++j)
			field->t[i][j] = 20.0f;
}

void refreshField(TemperatureField *field, int initX, int initY, int thisX, int thisY, int allX, int allY)
{
	int j;
	for (j=allY*3/10; j<allY*7/10; ++j)
	    if (legal(-initX, thisX)&&legal(j-initY, thisY))
			field->t[-initX][j-initY] = FIRE_TEMP;
}

double temperature_iterate(TemperatureField *field)
{
	double err;
	int i, j, d;
	for (i=0; i<field->x; ++i)
		for (j=0; j<field->y; ++j)
		{
			tempField->t[i][j] = 0;
			for (d=0; d<4; ++d)
				if ( legal(i+dx[d], field->x) && legal(j+dy[d], field->y) )
					tempField->t[i][j] += field->t[i+dx[d]][j+dy[d]];
				else
					tempField->t[i][j] += ROOM_TEMP;
			tempField->t[i][j] /= 4;
			if (i) //not fire place
				err += fabs(tempField->t[i][j] - field->t[i][j]);
		}
	refreshField(field, 0, 0, field->x, field->y, field->x, field->y);
	return err;
		// for (j=0; j<field->y; ++j)
		// {
		// 	int cnt = 0;
		// 	tempField->t[i][j] = 0;
		// 	for (d=0; d<4; ++d)
		// 		if ( legal(i+dx[d], field->x) && legal(j+dy[d], field->y) )
		// 		{
		// 			tempField->t[i][j] += field->t[i+dx[d]][j+dy[d]];
		// 			++cnt;
		// 		}
		// 	tempField->t[i][j] /= cnt;
		// }
	//tempField->t[0][0] = 100.0f;
}

unsigned long temperature_to_color_pixel(double t) {
	int idx = t/5;
	return color[idx];
}

void XWindow_Init(TemperatureField *field)
{    
    XSetWindowAttributes attr[1];           
    /* connect to Xserver */
    if (  (display = XOpenDisplay (display_name)) == NULL ) {
        fprintf (stderr, "drawon: cannot connect to X server %s\n", XDisplayName (display_name) );
        exit (-1);
    }     
    /* get screen size */
    screen = DefaultScreen (display);
    /* set window size */
    //XFlush (display);
    width = field->y;
    height = field->x;
    /* create opaque window */
    border_width = 4;
    win = XCreateSimpleWindow (display, RootWindow (display, screen),
                            width, height, width, height, border_width, 
                            BlackPixel (display, screen), WhitePixel (display, screen));
    size_hints.flags = USPosition|USSize;
    size_hints.x = 0;
    size_hints.y = 0;
    size_hints.width = width;
    size_hints.height = height;
    size_hints.min_width = 300;
    size_hints.min_height = 300;   
    XSetNormalHints (display, win, &size_hints);
    XStoreName(display, win, window_name);
    /* create graphics context */
    gc = XCreateGC (display, win, valuemask, &values);
	default_cmap = DefaultColormap(display, screen);
    XSetBackground (display, gc, WhitePixel (display, screen));
    XSetForeground (display, gc, BlackPixel (display, screen));
    XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
    attr[0].backing_store = Always;
    attr[0].backing_planes = 1;
    attr[0].backing_pixel = BlackPixel(display, screen);
    XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);
    XMapWindow (display, win);
    XSync(display, 0); 
}

void XRedraw(TemperatureField *field) {
    int i, j;
    for (int i=0; i<field->x; ++i)
        for (int j=0; j<field->y; ++j){
			XSetForeground(display, gc, temperature_to_color_pixel(field->t[i][j]));
	        XDrawPoint (display, win, gc, j, i);
	    }
    XFlush(display);
}

int main(int argc, char **argv)
{
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (argc<4)
    {
	    printf("Usage: %s x y iteration epsilon\n", argv[0]);
    }
    sscanf(argv[1], "%d", &x);
    sscanf(argv[2], "%d", &y);
    sscanf(argv[3], "%d", &iteration);
    sscanf(argv[4], "%lf", &EPSILON);

    field = malloc(sizeof(TemperatureField));
    tempField = malloc(sizeof(TemperatureField));
    terminate = 0;
    field->x = y;
    field->y = x;


    newField(field, x, y,0,0);
    newField(tempField, x, y,0,0);
    initField(field);
    XWindow_Init(field);

    int iter;
    for (iter=0; iter<iteration; iter++)
    {
		error = temperature_iterate(field);
		if ((error<EPSILON) && iter!=0){
			printf("Finished. iteration=%d, error=%lf\n", iter, error);
			break;
		}
		swapField = field;
		field = tempField;
		tempField = swapField;
		XRedraw(field);
    }

	clock_gettime(CLOCK_MONOTONIC, &finish);
	double time_elapsed_s = (finish.tv_sec-start.tv_sec) + (finish.tv_nsec - start.tv_nsec)/1000000000;
    printf("elapsed time %lfs\n", time_elapsed_s);

    deleteField(field);
    deleteField(tempField);

    return 0;
}
