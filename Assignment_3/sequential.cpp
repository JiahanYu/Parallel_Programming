#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <unistd.h>
using namespace std;
#define X_RESN 800
#define Y_RESN 800
const double G = 1;
const double dt = 0.1;
const int N = 300;
const int iteration = 100000;
const double rid = 1e2;
struct timespec ref_time;

typedef struct {
    double x;
    double y;
} Vec2;

typedef struct {
    Vec2 p;
    Vec2 v;
    double m;
} point;

Window win;
unsigned int width, height, x, y, border_width, display_width, display_height, screen;
const char *window_name = "Nbody Simulation";
char *display_name = NULL;
GC gc;
unsigned long valuemask = 0;
XGCValues values;
Display *display;
XSizeHints size_hints;
Pixmap bitmap;
XSetWindowAttributes attr[1];


void connect_to_Xserver() {
    if((display = XOpenDisplay(display_name)) == NULL) {
        fprintf(stderr, "drawon: cannot connect to X server %s\n", XDisplayName(display_name));
        exit(-1);
    }
}
void init_windows() {
    connect_to_Xserver();
    screen = DefaultScreen(display);
    display_width = DisplayWidth(display, screen);
    display_height = DisplayHeight(display, screen);
    width = X_RESN;
    height = Y_RESN;
    x = 0;
    y = 0;
    border_width = 4;
    win = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width, BlackPixel(display, screen), BlackPixel(display, screen));
    size_hints.flags = USPosition|USSize;
    size_hints.x = x;
    size_hints.y = y;
    size_hints.width = width;
    size_hints.height = height;
    size_hints.min_width = 300;
    size_hints.min_height = 300;  
    XSetNormalHints(display, win, &size_hints);
    XStoreName(display, win, window_name);
    gc = XCreateGC(display, win, valuemask, &values);
    XSetBackground(display, gc, WhitePixel(display, screen));
    XSetForeground(display, gc, WhitePixel(display, screen));
    XSetLineAttributes(display, gc, 1, LineSolid, CapRound, JoinRound);
    attr[0].backing_store = Always;
    attr[0].backing_planes = 1;
    attr[0].backing_pixel = BlackPixel(display, screen);
    XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);
    XMapWindow(display, win);
    XSync(display, 0);
}

inline int randint(int lowerbound, int upperbound) {return rand()%(upperbound-lowerbound) + lowerbound;}
void draw_all_points(point *body, bool white) {
    int i;
    if(white) {
        for(i=0; i<N; i++) {
            XSetForeground(display, gc, WhitePixel(display, screen));
            XDrawArc(display, win, gc, body[i].p.x-1, body[i].p.y-1, 2, 2, 0, 360*64);
        }
        XFlush(display);
    }
    else {
        XClearArea(display, win, 0, 0, X_RESN, Y_RESN, 0);
    }
}

void init_nbody(point *body) {
    int i;
    for(i=0; i<N; i++) {
        body[i].m = randint(5, 10);
        body[i].v.x = 0; body[i].v.y = 0;
        body[i].p.x = randint(0, 800);
        body[i].p.y = randint(0, 800);
    }
}

Vec2 Force_getForce(point* body, int idx1, int idx2 ) {
    Vec2 vec;
    Vec2 direction;
    direction.x = ( body[idx2].p.x - body[idx1].p.x );
    direction.y = ( body[idx2].p.y - body[idx1].p.y );

    double norm = sqrt(pow(direction.x,2) + pow(direction.y,2));  
    if ( norm < 1e-2 ) norm = 1e-2;
    double mag = body[idx1].m * body[idx2].m / pow( norm, 3 );
    vec.x = direction.x * mag;
    vec.y = direction.y * mag;

    return vec;
}

void reflect(point *body, int i){
    double tempv,tempp;

    if (body[i].p.x <= 0) {
        tempv = - body[i].v.x;
        body[i].v.x = tempv;
        body[i].p.x = 1;
    }
    if (body[i].p.x >= X_RESN) {
        tempv = - body[i].v.x;
        body[i].v.x = tempv;
        body[i].p.x = X_RESN - 1;
    }

    if (body[i].p.y <= 0) {
        tempv = - body[i].v.y;
        body[i].v.y = tempv;
        body[i].p.y = 1;
    }
    if (body[i].p.y >= Y_RESN) {
        tempv = - body[i].v.y;
        body[i].v.y = tempv;
        body[i].p.y = Y_RESN - 1;
    }
}


int main() {
    srand((unsigned)time(NULL));
    init_windows();
    point body[N];
    init_nbody(body);

    draw_all_points(body, true);
    clock_gettime( CLOCK_REALTIME, &ref_time );

    int i,j,k;
    for(k=0; k<iteration; k++) {
        draw_all_points(body, false);
        for(i=0; i<N; i++){
            //compute force/accerlation on ith body
            Vec2 F;
            F.x = F.y = 0;
            for (j = 0; j < N; j++) {
                if ( i != j ) {
                    Vec2 force = Force_getForce(body, i, j);
                    F.x += force.x;
                    F.y += force.y;
                }
            }
            body[i].v.x += F.x / body[i].m * dt;
            body[i].v.y += F.y / body[i].m * dt;
        }
        for(i=0; i<N; i++) {
            body[i].p.x += body[i].v.x * dt;
            body[i].p.y += body[i].v.y * dt;
            reflect(body,i);
        }
        draw_all_points(body, true);
    }

    struct timespec now;
    clock_gettime( CLOCK_REALTIME, &now );
    double elapsed_millis = ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
    printf( "elapsed_millis: %lf ms.\n", elapsed_millis );

    return 0;
}
