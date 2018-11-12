/* compile:
 * $gcc MS_pthread_dynamic.c -o MS_pthread_dynamic -L /opt/X11/lib -l X11 -lpthread
 * execute:
 * $./MS_pthread_dynamic <numOfThreads> <xmin> <xmax> <ymin> <ymin> <XResn> <YResn> <enable/disable>
 * e.g. $./MS_pthread_dynamic 4 -2 2 -2 2 400 400 enable
 */

#include <X11/Xlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>//time measure
#include <math.h>//time calculate

typedef struct complexType{
    double real, imag;
}Compl;

pthread_mutex_t mutex, ptr_mutex;
pthread_cond_t threshold_cv;
Display *display;
GC gc;
Window window;          /* initialization for a window */
int screen;             /* which screen */
clock_t tt1,tt2;
double rscale, iscale, roffset, ioffset, rright, iright;
int width, height, able;
int ptr = 0;

void *mandelbrot (void *t) {
    while (ptr < width) {
        long id = (long) t;
        int i = ptr;
        int j;

        /* draw points */
        Compl z, c;
        int repeats;
        double temp, lengthsq;
        for(j=0; j < height; j++) {
            z.real = 0.0;
            z.imag = 0.0;
            c.real = (double)i / rscale + roffset; /* Theorem : If c belongs to M(Mandelbrot set), then |c| <= 2 */
            c.imag = (double)j / iscale + ioffset; /* So needs to scale the window */
            repeats = 0;        
            lengthsq = 0.0;     

            while (repeats < 100 && lengthsq < 4.0) { /* Theorem : If c belongs to M, then |Zn| <= 2. So Zn^2 <= 4 */
                temp = z.real*z.real - z.imag*z.imag + c.real;
                z.imag = 2*z.real*z.imag + c.imag;
                z.real = temp;
                lengthsq = z.real*z.real + z.imag*z.imag; 
                repeats++;
            }

            if(able == 0){
                usleep(1);
                pthread_mutex_lock(&mutex); 
                XSetForeground(display, gc, 1024*1024*(repeats%256));
                XDrawPoint(display, window, gc, i, j);
                pthread_mutex_unlock(&mutex);
            }
        }
        pthread_mutex_lock(&ptr_mutex);
        ptr++;
        pthread_mutex_unlock(&ptr_mutex);
    }
    pthread_exit(NULL);
}


int main (int argc, char **argv) {

    int NUM_THREADS = atoi(argv[1]);     /* number of threads*/
    roffset = atof(argv[2]);             /* xmin: left range of real-axis */
    rright = atof(argv[3]);              /* xmax: right range of real-axis */
    ioffset = atof(argv[4]);             /* ymin: lower range of imag-axis */
    iright = atof(argv[5]);              /* ymax: upper range of imag-axis */
    /* set window size */
    width = atoi(argv[6]);               /* number of points in x-axis */
    height = atoi(argv[7]);              /* number of points in y-axis */
    rscale = width/(rright - roffset);
    iscale = height/(iright - ioffset);
    able = strncmp(argv[8], "enable", 6);/* enable/disable Xwindow */

    /* open connection with the server */
    if(able==0){
        /* open connection with the server */ 
        display = XOpenDisplay(NULL);
        if(display == NULL) {
            fprintf(stderr, "cannot open display\n");
            return 0;
        }
        screen = DefaultScreen(display);

        /* set window position */
        int x = 0;
        int y = 0;

        /* border width in pixels */
        int border_width = 0;

        /* create window */
        window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width,
                        BlackPixel(display, screen), WhitePixel(display, screen));
        
        /* create graph */
        XGCValues values;
        long valuemask = 0;
        
        gc = XCreateGC(display, window, valuemask, &values);
        //XSetBackground (display, gc, WhitePixel (display, screen));
        XSetForeground (display, gc, BlackPixel (display, screen));
        XSetBackground(display, gc, 0X0000FF00);
        XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
        
        /* map(show) the window */
        XMapWindow(display, window);
        XSync(display, 0);
    }

    /* time start */
    tt1 = clock();

    pthread_t threads[NUM_THREADS];
    int *taskids[NUM_THREADS];
    int rc, i, j;
    long t;
    pthread_attr_t attr;
    /* Initialize mutex and condition variable */
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&ptr_mutex, NULL);
    pthread_cond_init(&threshold_cv, NULL);
    /* For portability, explicitly create threads in a joinable state. */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (t=0; t<NUM_THREADS; t++){
        rc = pthread_create(&threads[t], &attr, mandelbrot, (void *) t);
        if(rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    /* Wait for all threads to complete. */
    for (i=0; i<NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    /* display processing time */
    tt2 = clock();
    double period = (double)(tt2 - tt1)/CLOCKS_PER_SEC;
    printf("Total comp time: %f sec\n", period);

   
    FILE *log_file_p;
    char file_name[100];
    sprintf(file_name, "Pthread_dynamic_%d_processes_execution_time.txt", NUM_THREADS);
    log_file_p = fopen(file_name, "a");  // add a new log
    fprintf(log_file_p, "%f\n", period);
    fclose(log_file_p);


    if(able==0){
        XFlush(display);
        sleep(10);
    }

    /* Clean up and exit. */
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&ptr_mutex);
    pthread_cond_destroy(&threshold_cv);


    return 0;
}
