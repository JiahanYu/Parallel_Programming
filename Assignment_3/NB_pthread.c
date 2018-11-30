#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>

#define X_RESN 800
#define Y_RESN 800
int num_threads;
const double G = 1;
const double dt = 0.5;
const int N = 800;
const int iteration = 10000;
const double rid = 1e2;
struct timespec ref_time;
pthread_mutex_t mutex;
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

point body[N];
int count =0;

typedef struct {
  int num_threads;
  int dt;
  int tid;
} Params;



void reflect(point* body, int i){
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

void draw_all_points(point *body, int white) {
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
        body[i].m = rand()%5 + 5;
        body[i].v.x = 0; body[i].v.y = 0;
        body[i].p.x = rand()%800;
        body[i].p.y = rand()%800;
    }
}

Vec2 Force_getForce(int idx1, int idx2 ) {
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

void* move_bodies( void* t ) {
    int tid = (intptr_t)t;
    int i, j;

    if ( num_threads > N ) num_threads = N;
    if ( tid < num_threads ) {
        int start_idx = tid * ( N / num_threads );
        int end_idx = ( tid < num_threads - 1 ) ? start_idx + ( N / num_threads ) : N;
        for (i = start_idx; i < end_idx; i++) {
            Vec2 F;
            F.x = 0; F.y = 0;
            for (j = 0; j < N; j++) {
                if ( i != j ) {
                    Vec2 force = Force_getForce( i, j );
                    F.x += force.x;
                    F.y += force.y;
                }
            }
        //compute and update velocity
        body[i].v.x += F.x / body[i].m * dt;
        body[i].v.y += F.y / body[i].m * dt;
        }
    }
}


int main( int argc, char* argv[] ) {

    num_threads = atoi(argv[1]);
    srand((unsigned)time(NULL));
    init_windows();
    init_nbody(body);

    pthread_t* threads = malloc( sizeof( pthread_t ) * num_threads );

    int i, iter;
    draw_all_points(body, 1);
    clock_gettime( CLOCK_REALTIME, &ref_time );
    for ( iter = 0; iter < iteration; iter++) {
        draw_all_points(body, 0);
        //compute force/accerlation on ith body
        for (i = 0; i < num_threads; i++)
            pthread_create( &threads[i], NULL, move_bodies, (void*) (intptr_t)i );
        for (i = 0; i < num_threads; i++)
            pthread_join( threads[i], NULL );
        //compute and update position
        for(i=0; i<N; i++) {
            body[i].p.x += body[i].v.x * dt;
            body[i].p.y += body[i].v.y * dt;
            reflect(body,i);
        }
        draw_all_points(body, 1);
    }

    struct timespec now;
    clock_gettime( CLOCK_REALTIME, &now );
    double elapsed_millis = ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
    printf( "elapsed_millis: %lf ms.\n", elapsed_millis );
    pthread_exit(NULL);
    return 0;
}

