/* compile:
 * $gcc MS_pthread_static.c -o MS_pthread_static -L /opt/X11/lib -l X11 -lpthread
 * execute:
 * $./MS_pthread_static <numOfThreads> <xmin> <xmax> <ymin> <ymin> <XResn> <YResn> <enable/disable>
 * e.g. $./MS_pthread_static 4 -2 2 -2 2 400 400 enable
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

struct thread_data
{
   int thread_id;
   int inii;
   int fini;
};

pthread_mutex_t mutex;
Display *display;
GC gc;
Window window;          /* initialization for a window */
int screen;             /* which screen */
clock_t tt1,tt2;
double rscale, iscale, roffset, ioffset, rright, iright;
int width, height, able;

void *mandelbrot (void *threadarg) {
    int xmin, xmax;
    long t;
    struct thread_data *my_data;

    my_data = (struct  thread_data *) threadarg;
    t = my_data->thread_id;
    xmin = my_data->inii;
    xmax = my_data->fini;

    Compl z, c;
    int repeats;        /* num of iterations */
    double lengthsq;    /* the length of the vector */
    double temp;
    int i,j;

    pthread_mutex_lock(&mutex);

    /* Calculate points */
    for (i = xmin; i < xmax; i++){
        for(j=0; j < height; j++) {

            z.real = z.imag = 0.0;
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
                XSetForeground(display, gc, 1024*1024*(repeats%256));
                XDrawPoint(display, window, gc, i, j);
            }

        }

    }

    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);

}


int main (int argc, char **argv) {

    int NUM_THREADS = atoi(argv[1]);         /* number of threads*/
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
    struct thread_data thread_data_array[NUM_THREADS];

    /* open connection with the server */
    if(able==0){
        /* open connection with the server */ 
        display = XOpenDisplay(NULL);
        if(display == NULL) {
            fprintf(stderr, "cannot open display\n");
            exit(-1);
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

    /* assign length */
    int inii = 0;
    int fini = width;
    int coresize = 0;
    coresize = width / NUM_THREADS;
    if((width%NUM_THREADS) != 0) coresize++;
   
    pthread_t threads[NUM_THREADS];
    int *taskids[NUM_THREADS];
    int rc, i, j;
    long t;
    pthread_attr_t attr;
    /* Initialize mutex and condition variable */
    pthread_mutex_init(&mutex, NULL);
    /* For portability, explicitly create threads in a joinable state. */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    for (t=0; t<NUM_THREADS; t++){
        thread_data_array[t].thread_id = t;
        thread_data_array[t].inii = coresize * t;
        if(t < NUM_THREADS-1){
            thread_data_array[t].fini = coresize * (t+1); 
        }else {
            thread_data_array[t].fini = width;
        }
        rc = pthread_create(&threads[t], &attr, mandelbrot, (void *) &thread_data_array[t]);
        if(rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }

    }

    /* Wait for all threads to complete. */
    for ( i=0; i<NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    /* display processing time */
    tt2 = clock();
    double period = (double)(tt2 - tt1)/CLOCKS_PER_SEC;
    printf("Total comp time: %f sec\n", period);

    /* save execution time log */
    FILE *log_file_p;
    char file_name[100];
    sprintf(file_name, "Pthread_static_%d_processes_execution_time.txt", NUM_THREADS);
    log_file_p = fopen(file_name, "a");  // add a new log
    fprintf(log_file_p, "%f\n", period);
    fclose(log_file_p);

    /* Draw points */    
    if(able == 0){
        XFlush(display);
        sleep(10);        
    }

    /* Clean up and exit */
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&mutex);

    return 0;
}