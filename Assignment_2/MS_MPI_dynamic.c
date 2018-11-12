

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <time.h>//time measure
#include <math.h>//time calculate

#define terminate_tag 0
#define data_tag 1
#define result_tag 2

typedef struct complextype
{
	double real, imag;
} Compl;

typedef struct commtype
{
	int j, *repeats;
} Comm;

int main(int argc, char *argv[])
{
	
	Display *display;
	GC gc;
	Window window;        	/* initialization for a window */
	int screen;           	/* which screen */

	int able = strncmp(argv[8], "enable", 6);
	if( able==0){
		/* open connection with the server */ 
		display = XOpenDisplay(NULL);
		if(display == NULL) {
			fprintf(stderr, "cannot open display\n");
			return 0;
		}
		screen = DefaultScreen(display);
	}

	//const int n = atoi(argv[1]);      		/* number of threads, doesn't make sense in MPI */
	double roffset = atof(argv[2]);  			/* left range of real-axis */
	double rright = atof(argv[3]);  			/* right range of real-axis */
	double ioffset = atof(argv[4]);  			/* lower range of imag-axis */
	double iright = atof(argv[5]); 				/* upper range of imag-axis */
	/* set window size */
	int width = atoi(argv[6]);    				/* number of points in x-axis */
	int height = atoi(argv[7]);   				/* number of points in y-axis */
	double rscale = width/(rright - roffset);   /* horizontal unit scale */
	double iscale = height/(iright - ioffset);  /* vertical unit scale */

	/* MPI Init */
	int rank, size;         
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size); //num of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); //process id

	//time measure
	clock_t tt1, tt2;
	tt1 = clock();

	if(rank==0 && able==0){
		/* set window position */
		int x = 0;
		int y = 0;

		/* border width in pixels */
		int border_width = 0;

		/* create window */
		window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width, BlackPixel(display, screen), WhitePixel(display, screen));
		
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
	
	/* draw points */
	int i, k;
	MPI_Barrier( MPI_COMM_WORLD );
	if(size == 1){
		Compl z, c;
		int repeats;		/* num of iterations */
		double lengthsq;	/* the length of the vector */
		double temp;
		int i, j;
		for(i=0; i<width; i++) {
			for(j=0; j<height; j++) {
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

				/* Draw points */
				XSetForeground (display, gc,  1024 * 1024 * (repeats % 256));		
				XDrawPoint (display, window, gc, i, j);
			}
		}
	}
	else if(rank==0){
		int working = 0;  //determine how many processes are working currently
		int row = 0;
		struct commtype remote;
		int remote_row, *remote_repeats;
		MPI_Status status;
		remote_repeats = (int *)malloc(sizeof(int)*width);
		
		for(k=1; k<size; k++){
			MPI_Send(&row, 1, MPI_INT, k, data_tag, MPI_COMM_WORLD);
			working++;
			row++;
		}
		do{
			// receive message from any source
			MPI_Recv(&remote_row, 1, MPI_INT, MPI_ANY_SOURCE, result_tag, MPI_COMM_WORLD, &status);
			MPI_Recv(remote_repeats, width, MPI_INT, status.MPI_SOURCE, result_tag, MPI_COMM_WORLD, &status);
			working--;

			// send reply back to sender of the message received above
			if(row < height){
				MPI_Send(&row, 1, MPI_INT, status.MPI_SOURCE, data_tag, MPI_COMM_WORLD);
				working++;
				row++;
			}else //send termination message
				MPI_Send(&row, 1, MPI_INT, status.MPI_SOURCE, terminate_tag, MPI_COMM_WORLD);
		
			/* Draw points */
			if(able == 0){
				for(i=0;i<width;i++){
					XSetForeground (display, gc,  1024 * 1024 * (remote_repeats[i] % 256));		
					XDrawPoint (display, window, gc, i, remote_row);
				}
			}
		}while(working>0);
	}
	else{
		int row;
		int repeats;		/* num of iterations */
		double lengthsq;	/* the length of the vector */
		double temp;
		Compl z, c;
		struct commtype self;
		MPI_Status status;
		self.repeats = (int *)malloc(sizeof(int)*width);
		MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		while(status.MPI_TAG == data_tag){
			self.j = row;
			c.imag = (double)row / iscale + ioffset; 
			for(i=0; i<width; i++) {
				z.real = 0.0;
				z.imag = 0.0;
				c.real = (double)i / rscale + roffset; /* Theorem : If c belongs to M(Mandelbrot set), then |c| <= 2 */
				
				repeats = 0;
				lengthsq = 0.0;

				while(repeats < 100 && lengthsq < 4.0) { /* Theorem : If c belongs to M, then |Zn| <= 2. So Zn^2 <= 4 */
					temp = z.real*z.real - z.imag*z.imag + c.real;
					z.imag = 2*z.real*z.imag + c.imag;
					z.real = temp;
					lengthsq = z.real*z.real + z.imag*z.imag; 
					repeats++;
				}
				self.repeats[i] = repeats;
				//MPI_Barrier( MPI_COMM_WORLD );
			}
			
			MPI_Send(&row, 1, MPI_INT, 0, result_tag, MPI_COMM_WORLD);
			MPI_Send(self.repeats, width, MPI_INT, 0, result_tag, MPI_COMM_WORLD);
			MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		}
	}
	
    if(rank==0){
    	/* display processing time */
	    tt2 = clock();
	    printf("Total comp time: %f sec\n", (double)(tt2 - tt1)/CLOCKS_PER_SEC);
		if (able==0){
			XFlush(display);
			sleep(5);
		}
	}
	
	/* MPI dealloc */
    MPI_Finalize();
	return 0;
}