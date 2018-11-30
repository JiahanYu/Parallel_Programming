#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

#define X_RESN 800
#define Y_RESN 800
#define ALL_TAG 101 /* transmit the info of all bodies */
#define NEW_TAG 102 /* transmit the new info of bodies */
#define TER_TAG 103 /* transmit the terminate info from master to slave */

const double G = 1;
const double dt = 0.1;
const int N = 800;
const int iteration = 100;
const double rid = 1e2;
struct timespec ref_time;

typedef struct {
    double x;
    double y;
} Vec2;

// typedef struct {
//     Vec2 p;
//     Vec2 v;
//     double m;
// } point;
struct point {
    double x,y;
    double vx,vy;
    double m;
};

//struct point body[N];


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

void draw_all_points(struct point *body, int white) {
    int i;
    if(white) {
        for(i=0; i<N; i++) {
            XSetForeground(display, gc, WhitePixel(display, screen));
            XDrawArc(display, win, gc, body[i].x-1, body[i].y-1, 2, 2, 0, 360*64);
        }
        XFlush(display);
    }
    else {
        XClearArea(display, win, 0, 0, X_RESN, Y_RESN, 0);
    }
}

void init_nbody(struct point *body) {
    int i;
    for(i=0; i<N; i++) {
        body[i].m = rand()%5 + 5;
        body[i].vx = 0; body[i].vy = 0;
        body[i].x = rand()%800;
        body[i].y = rand()%800;
    }
}

Vec2 Force_getForce(struct point* body, int idx1, int idx2 ) {
    Vec2 vec;
    Vec2 direction;
    direction.x = ( body[idx2].x - body[idx1].x );
    direction.y = ( body[idx2].y - body[idx1].y );

    double norm = sqrt(pow(direction.x,2) + pow(direction.y,2));  
    if ( norm < 1e-2 ) norm = 1e-2;
    double mag = body[idx1].m * body[idx2].m / pow( norm, 3 );
    vec.x = direction.x * mag;
    vec.y = direction.y * mag;

    return vec;
}

void reflect(struct point *body, int i){
    double tempv,tempp;
    if (body[i].x <= 0) {
        tempv = - body[i].vx;
        body[i].vx = tempv;
        body[i].x = 1;
    }
    if (body[i].x >= X_RESN) {
        tempv = - body[i].vx;
        body[i].vx = tempv;
        body[i].x = X_RESN - 1;
    }
    if (body[i].y <= 0) {
        tempv = - body[i].vy;
        body[i].vy = tempv;
        body[i].y = 1;
    }
    if (body[i].y >= Y_RESN) {
        tempv = - body[i].vy;
        body[i].vy = tempv;
        body[i].y = Y_RESN - 1;
    }
}

void setStartEnd(int size,int rank,int oriStart,int oriEnd,int *istart,int *icount){
    int ilength,iblock,irem,iend,i;
    ilength=oriEnd-oriStart+1;
    iblock=(int)ilength/size;
    irem=ilength-iblock*size;
    if(ilength>0){
        if(rank<irem){
            *istart=oriStart+rank*(iblock+1);
            iend=*istart+iblock;
        }else{
            *istart=oriStart+rank*iblock+irem;
            iend=*istart+iblock-1;
        }
    }else{
        *istart=1;
        iend=0;
    }
    *icount=iend-*istart+1;
}

void calculateNewInfo(struct point *oldBody,int start,int count){
    int i,j;

    /* calculate new velocity of each body*/
    for(i=0;i<count;i++){
        Vec2 F;
        F.x = F.y = 0;
        for(j=0;j<N;j++){
            if ( i != j ) {
                Vec2 force = Force_getForce(oldBody, i, j);
                F.x += force.x;
                F.y += force.y;
            }
        }
        oldBody[start+i].vx += F.x / oldBody[i].m * dt;
        oldBody[start+i].vy += F.y / oldBody[i].m * dt;
    }
    for (i=0;i<count;i++){
        oldBody[start+i].x += oldBody[start+i].vx * dt;
        oldBody[start+i].y += oldBody[start+i].vy * dt;
        reflect(oldBody,i);
    }
}

int main(int argc, char* argv[]) {
    clock_gettime( CLOCK_REALTIME, &ref_time );

    /* MPI Init */
    int size=0,rank=0;
    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    /* define Body type */
    MPI_Datatype Body_type;
    int blocklen[6]={1,1,1,1,1,1};
    MPI_Aint disp[6];//={0,8,16,24,32};
    disp[0]=0;
    disp[1]=sizeof(double);
    disp[2]=2*sizeof(double);
    disp[3]=3*sizeof(double);
    disp[4]=4*sizeof(double);
    disp[5]=sizeof(struct point);
    MPI_Datatype type[6]={MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_UB};
    MPI_Type_create_struct(5,blocklen,disp,type,&Body_type);
    MPI_Type_commit(&Body_type);

    /* initializing index for partition */
    int istart=0,icount=0;
    setStartEnd(size,rank,0,N-1,&istart,&icount);

    /* N-Body variables */
    int i,j,k;
    struct point body[N]; /* record the properties of all bodies */

    /* variables for MPI_Isend and MPI_Irecv*/
    MPI_Request req;
    MPI_Status stat;

    if (rank == 0) {
        init_windows();
        srand((unsigned)time(NULL));
        init_nbody(body);
        draw_all_points(body, 1);
    }

    if (rank==0){
        /* start to simulate N-body */
        for(k=0;k<iteration;k++){
            draw_all_points(body, 0);
            /* send body data to each slave */
            for(i=1;i<size;i++){
                MPI_Isend(body,N,Body_type,i,ALL_TAG,MPI_COMM_WORLD,&req);
                MPI_Wait(&req,&stat);
            }
            /* calculate new info of bodies in their own partition */
            setStartEnd(size,0,0,N-1,&istart,&icount);
            calculateNewInfo(body,istart,icount);
            /* receive other parts from each slave */
            for(i=1;i<size;i++){
                setStartEnd(size,i,0,N-1,&istart,&icount);
                MPI_Irecv(&body[istart],icount,Body_type,i,NEW_TAG,MPI_COMM_WORLD,&req);
                MPI_Wait(&req,&stat);
            }
            draw_all_points(body, 1);
        }
        /* tell slave to terminate */
        for(i=1;i<size;i++){
            MPI_Isend(body,N,Body_type,i,TER_TAG,MPI_COMM_WORLD,&req);
            MPI_Wait(&req,&stat);
        }

    } else {
        /* receive the info from master */
        MPI_Irecv(body,N,Body_type,0,MPI_ANY_TAG,MPI_COMM_WORLD,&req);
        MPI_Wait(&req,&stat);
        while(stat.MPI_TAG==ALL_TAG){
            /* calculate new info of bodies in their own partition */
            calculateNewInfo(body,istart,icount);
            /* send new info to master */
            MPI_Isend(&body[istart],icount,Body_type,0,NEW_TAG,MPI_COMM_WORLD,&req);
            MPI_Wait(&req,&stat);
            /* receive the next info from master */
            MPI_Irecv(body,N,Body_type,0,MPI_ANY_TAG,MPI_COMM_WORLD,&req);
            MPI_Wait(&req,&stat);
        }
    }

    if (rank == 0){
        struct timespec now;
        clock_gettime( CLOCK_REALTIME, &now );
        double elapsed_millis = ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
        printf( "elapsed_millis: %lf ms.\n", elapsed_millis );
    }
    MPI_Finalize();

    return 0;
}
