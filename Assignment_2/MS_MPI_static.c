

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <time.h>//time measure
#include <math.h>//time calculate

typedef struct complextype
{
	double real, imag;
} Compl;

int main(int argc, char *argv[])
{
	Display *display;
	GC gc;
	Window window;        	/* initialization for a window */
	int screen;           	/* which screen */

	int able = strncmp(argv[8], "enable", 6);	/* enable/disable Xwindow */
	if(able==0){
		/* open connection with the server */ 
		display = XOpenDisplay(NULL);
		if(display == NULL) {
			fprintf(stderr, "cannot open display\n");
			return 0;
		}
		screen = DefaultScreen(display);
	}

	//const int n = atoi(argv[1]);      		/* number of threads, doesn't make sense in MPI*/
	double roffset = atof(argv[2]);  			/* left range of real-axis */
	double rright = atof(argv[3]);  			/* right range of real-axis */
	double ioffset = atof(argv[4]);  			/* lower range of imag-axis */
	double iright = atof(argv[5]); 				/* upper range of imag-axis */
	/* set window size */
	int width = atoi(argv[6]);    				/* number of points in x-axis */
	int height = atoi(argv[7]);   				/* number of points in y-axis */
	double rscale = width/(rright - roffset);
	double iscale = height/(iright - ioffset);
	
	/* MPI Init */
	int rank, size;         
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size); //num of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); //process id

	/* time measure */
	clock_t tt1, tt2;
	tt1 = clock();

	//if window is enabled
	if(rank==0 && able==0){
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
		XSetForeground (display, gc, BlackPixel (display, screen));
		XSetBackground(display, gc, 0X0000FF00);
		XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
		
		/* map(show) the window */
		XMapWindow(display, window);
		XSync(display, 0);
	}
	
	/* assign length */
	int inii=0;
	int fini=width;
	int coresize=0;
	coresize = width / size;
	if(width%2 != 0) coresize++;
	inii = coresize * rank;
	if(fini > (inii + coresize))
		fini = (inii + coresize);

	/* draw points */
	Compl z, c;
	int *remote_i, *remote_repeats;
	int *self_repeats = (int *) malloc(sizeof(int) * height);
	
	if(rank==0){
		remote_i = (int *)malloc(sizeof(int) * size);
		remote_repeats = (int *)malloc(sizeof(int) * height * size);
		//remote = (struct commtype *)malloc(sizeof(int)*( height + 1 ) * size);
	}

	int i, j, k;	
	int repeats;        /* num of iterations */
	double temp;
	double lengthsq;	/* the length of the vector */
	MPI_Barrier( MPI_COMM_WORLD );

    /* Calculate points */
	for(i = inii; i < fini; i++) {
		for(j=0; j < height; j++) {
			z.real = 0.0;
			z.imag = 0.0;
			c.real = (double)i / rscale + roffset; /* Theorem : If c belongs to M(Mandelbrot set), then |c| <= 2 */
			c.imag = (double)j / iscale + ioffset; /* So needs to scale the window */
			repeats = 0;		
			lengthsq = 0.0; 	

			while(repeats < 100 && lengthsq < 4.0) { /* Theorem : If c belongs to M, then |Zn| <= 2. So Zn^2 <= 4 */
				temp = z.real*z.real - z.imag*z.imag + c.real;
				z.imag = 2*z.real*z.imag + c.imag;
				z.real = temp;
				lengthsq = z.real*z.real + z.imag*z.imag; 
				repeats++;
			}
			self_repeats[j] = repeats;
		}
		
		MPI_Gather( &i, 1 , MPI_INT, remote_i, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Gather( self_repeats, height, MPI_INT, remote_repeats, height, MPI_INT, 0, MPI_COMM_WORLD);
	
		/* Draw points */
		if(rank==0 && able==0){
			for(k=0;k<size;k++){
				for(j=0;j<height;j++){
					XSetForeground (display, gc,  1024 * 1024 * (remote_repeats[k*height+j] % 256));	
					XDrawPoint (display, window, gc, remote_i[k], j);
				}
			}
		}
	}

	/* dealloc memory */
	if(rank == 0){
    	/* display processing time */
	    tt2 = clock();
	    printf("Total comp time: %f sec\n", (double)(tt2 - tt1)/CLOCKS_PER_SEC);

		if(able==0){
			XFlush(display);
			sleep(5);
		}
		free((void *)remote_i);
		free((void *)remote_repeats);
	}
	free((void *)self_repeats);

	/* MPI dealloc */
    MPI_Finalize();

	return 0;
}