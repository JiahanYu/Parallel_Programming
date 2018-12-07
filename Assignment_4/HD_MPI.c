#include <mpi.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <time.h>

#define NIL (0)
#define WORKTAG 21
#define QUITTAG 86    /* tag message: "Master wishes you to quit"*/
#define ROOMWIDTH 200
#define ROOMHEIGHT 200
#define FIREPLACETEMP 100
#define WALLTEMP 20
#define DEBUG 1
#define EPSILON 0.01

enum BOOLEAN {FALSE=0,TRUE = 1};
unsigned long color[20] = {0X6666CC,0X8A2BE2,0X7B68EE,0X6495ED,0X1E90FF,0X87CEFA,0X00FFFF,
					0X40E0D0,0X7FFFAA,0X3CB371,0X2E8B57,0X556B2F,0X808000,
					0XBDB76B,0XF0E68C,0XFFD700,0XFF6347,0XFF4500,0XF08080,
					0XB22222};

/* function prototype */
void InitGraphics();
void InitializeWorkspace();
void DoDistribution(int numprocs, int currentrank);
void DrawGraphics();
void PartitionData(int numprocs);
void CalculateSlaveValues();

int myrank;
int size;
int global_term;

Display* dpy;
Window w;
GC gc;

struct timespec ref_time;

unsigned long temperature_to_color_pixel(double t) {
	int idx = t/5;
	return color[idx];
}

int main(int argc, char* argv[]) {
    clock_gettime( CLOCK_REALTIME, &ref_time );

	/* init mpi */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	global_term = FALSE;

	if(myrank == 0) { /* master */
		//gettimeofday(&starttime,(struct timezone *) 0);
		InitGraphics();
		PartitionData(size);
	}
	else DoDistribution(size, myrank); /* do the slave work */

    if (myrank == 0){
        struct timespec now;
        clock_gettime( CLOCK_REALTIME, &now );
        double elapsed_millis = ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
        printf( "elapsed_millis: %lf ms.\n", elapsed_millis );
    }

	MPI_Finalize();
 	return(0);
}


void InitGraphics() {
	/* create a display object */
	/* create a Graphics context object */
	/* reserve two colors for use */
	Display* newdpy = XOpenDisplay(NIL);
	int blackColor = BlackPixel(newdpy, DefaultScreen(newdpy));
	int whiteColor = WhitePixel(newdpy, DefaultScreen(newdpy));
	dpy = newdpy;

	/* create a simple window, h200 x w100, black bg */
	w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, ROOMWIDTH, ROOMHEIGHT,
						 0, blackColor, blackColor);
	XSelectInput(dpy, w, StructureNotifyMask);
	XMapWindow(dpy, w);
	/* initialize GC */
	gc = XCreateGC(dpy, w, 0, NIL);
	/* set the forecolor */
	XSetForeground(dpy, gc, whiteColor);

	for(;;) {
		XEvent e;
		XNextEvent(dpy, &e);
		if (e.type == MapNotify) break;
		if (global_term == TRUE) return;
	}
	XFlush(dpy);
	XSetForeground(dpy, gc, 0X87CEFA);
}


void DoDistribution(int numprocs, int currentrank) {
	int iteration=0;
	int limit = 18663;
	int terminate = FALSE;
	int gterminate = FALSE;
	int i,j;
	MPI_Status status;
	MPI_Request req;
	unsigned long startingpoint = 0;
	long rowsize = ROOMHEIGHT / (numprocs - 1);

	/* represent ghost points */
	double habove[ROOMWIDTH] = {20.0}; 
	double hbelow[ROOMWIDTH] = {20.0}; 
	double gabove[ROOMWIDTH] = {20.0}; 
	double gbelow[ROOMWIDTH] = {20.0}; 
	
	double h[numprocs][ROOMWIDTH][rowsize]; 	/* double */
	int    converged[ROOMWIDTH][rowsize]; 		/* int: 0 or 1 */
    double oldvalues[ROOMWIDTH][rowsize]; 		/* double */
    double g[numprocs-1][ROOMWIDTH][rowsize]; 	/* double */

	/* init values for running */
	for(i = 0;i < ROOMWIDTH; i++){
	 	for(j = 0;j < rowsize; j++) {
		 	converged[i][j] = 0; 	/* initially not converged */
		 	oldvalues[i][j] = 20.0;
		 	h[currentrank][i][j] = 20.0;
		 	g[currentrank][i][j] = 20.0;
	 	}
 	}

 	while(terminate == FALSE) {

 		/*set up fireplace and walls */
 		for(j = 0; j < rowsize; j++){	 		
	 		h[currentrank][0][j] = WALLTEMP;
	 		h[currentrank][ROOMWIDTH-1][j] = WALLTEMP;
	 	}

	 	if((currentrank == numprocs-1)){
	 		for(i = 0;i < ROOMWIDTH;i++)
	 			h[currentrank][i][rowsize-1] = WALLTEMP;
	 	}
	 			
		if(currentrank == 1) {
			for(i = (ROOMWIDTH*(3.0/10.0));i < (ROOMWIDTH * (7.0/10.0)); i++)
 				h[currentrank][i][0] = FIREPLACETEMP;
	 	 	for(i = 0;i < (ROOMWIDTH * (3.0/10.0));i++) 
	 	 		h[currentrank][i][0] = WALLTEMP;
	 	 	for(i = (ROOMWIDTH*(7.0/10.0)); i <ROOMWIDTH; i++) 
	 	 		h[currentrank][i][0] = WALLTEMP;
 		}

 		/* Modified Jacobi Iterative method */
		for(i = 1;i < ROOMWIDTH - 1 ;i++) {
			for(j = 0;j < rowsize;j++) {
 				if(j == 0 && currentrank > 1) 
 					g[currentrank][i][j] = 0.25 * (h[currentrank][i-1][j] + 
 						h[currentrank][i+1][j] + hbelow[i] + h[currentrank][i][j+1]);
 				else if(j == (rowsize-1) && currentrank < (numprocs-1)) 
 					g[currentrank][i][j] = 0.25 * (h[currentrank][i-1][j] + 
 						h[currentrank][i+1][j] + h[currentrank][i][j-1] + habove[i]);
 				else {
 					if(j!=(rowsize-1) && j!=0){
 						g[currentrank][i][j] = 0.25 * (h[currentrank][i-1][j] + 
 							h[currentrank][i+1][j] + h[currentrank][i][j-1] + h[currentrank][i][j+1]);
 					}
 				}

 				/* check convergence */
				if(fabs(g[currentrank][i][j] - oldvalues[i][j])< 0.01) 
				 	converged[i][j] = 1;

				if (j == 0 && currentrank == 1) converged[i][j] = 1;
				else if(j == (rowsize-1) && currentrank == (numprocs-1)) converged[i][j] = 1;

				/* store calculated values for next convergence checking */
				oldvalues[i][j] = g[currentrank][i][j];
			}
		}

		for(j=0;j<ROOMHEIGHT-1;j++){
			converged[0][j] = converged[ROOMWIDTH-1][j] = 1;
		}

		/* save off computed values */
		for(i = 0;i < ROOMWIDTH;i++){
			for(j = 0;j < rowsize;j++){
				h[currentrank][i][j]=g[currentrank][i][j];
			}
		}
		
		if (iteration % 100 == 0)
			MPI_Send(&g[currentrank], ROOMWIDTH*rowsize, MPI_DOUBLE, 0, WORKTAG, MPI_COMM_WORLD);
 		
 		if(currentrank % 2==0) { /* even numbered node */
			if(currentrank < numprocs-1) {
				for(i=0; i < ROOMWIDTH; i++)
 					gbelow[i]=g[currentrank][i][rowsize-1];
 				MPI_Send(&gbelow, ROOMWIDTH, MPI_DOUBLE, currentrank + 1, WORKTAG, MPI_COMM_WORLD);
				MPI_Recv(&habove, ROOMWIDTH, MPI_DOUBLE, currentrank + 1, WORKTAG, MPI_COMM_WORLD, &status);
			}
			/*send ghost points off*/
			if(currentrank > 1) {
				for(i=0;i < ROOMWIDTH; i++)
 					gabove[i] = g[currentrank][i][0];
					MPI_Send(&gabove, ROOMWIDTH, MPI_DOUBLE, currentrank-1, WORKTAG, MPI_COMM_WORLD);
					MPI_Recv(&hbelow, ROOMWIDTH, MPI_DOUBLE, currentrank-1, WORKTAG, MPI_COMM_WORLD, &status);
 			}
 		}
		else { /* odd numbered node */
			if(currentrank>1) {
				MPI_Recv(&hbelow, ROOMWIDTH, MPI_DOUBLE, currentrank-1, WORKTAG, MPI_COMM_WORLD, &status);
				for(i=0;i<ROOMWIDTH;i++)
		 			gbelow[i]=g[currentrank][i][0];
		 		MPI_Send(&gbelow, ROOMWIDTH, MPI_DOUBLE, currentrank-1, WORKTAG, MPI_COMM_WORLD);
		 	}
		 	if(currentrank<numprocs-1) {
		 		for(i=0;i<ROOMWIDTH;i++) 
		 			gabove[i]=g[currentrank][i][rowsize-1];
		 		MPI_Recv(&habove,ROOMWIDTH,MPI_DOUBLE, currentrank+1, WORKTAG, MPI_COMM_WORLD,&status);
				MPI_Send(&gabove,ROOMWIDTH, MPI_DOUBLE, currentrank+1,WORKTAG, MPI_COMM_WORLD);
		 	}
		}

		/* check if each pixel has converged */
		terminate = TRUE;
 		for(i = 0;i < ROOMWIDTH;i++) {
 			for(j = 0;j < rowsize;j++) {
 				if(converged[i][j] == FALSE)
 					terminate = FALSE;
			}
 		}

 		/* if itself is converge */
		if(terminate==TRUE) {
			/* this node tell the host i'm done*/
			MPI_Send(&terminate, 1, MPI_INT, 0, QUITTAG, MPI_COMM_WORLD);
			/* this node want to know whether his siblings are done or not */
			MPI_Recv(&terminate, 1, MPI_INT, 0, QUITTAG, MPI_COMM_WORLD, &status);
		}

 		iteration++;
 		if(iteration > limit) terminate = TRUE;

 	} 

 	terminate = FALSE;
 	gterminate = FALSE;
 	/* SEND TO MASTER */
 	MPI_Send(&g[currentrank], ROOMWIDTH*rowsize, MPI_DOUBLE, 0, QUITTAG, MPI_COMM_WORLD);

	while(terminate==FALSE) {
 		MPI_Recv(&gterminate, 1, MPI_INT, 0, WORKTAG, MPI_COMM_WORLD, &status);
 		if(gterminate==TRUE) terminate = TRUE;
 	}

}


void PartitionData(int numprocs) {
	int i,j;
	int terminate=0;
	int count = numprocs-1; /* number of working slaves*/
	int quitsig = 1;
	int globalconvergence = 0;
	int constint = 0;
	MPI_Status status;
	unsigned long rowsize = ROOMHEIGHT/(numprocs-1);
	int converged[60]={0};
	double h[ROOMWIDTH][rowsize];
	unsigned long lastproc = 0;

	while (terminate == FALSE) {
 		MPI_Recv(&h, rowsize*ROOMWIDTH, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if(status.MPI_TAG == QUITTAG) { /* indicate convergence */
			converged[status.MPI_SOURCE] = 1;
			count--;
		} else {
			for(i = 0;i < ROOMWIDTH;i++) {
 				for(j = 0;j < rowsize;j++) {
 					XSetForeground(dpy, gc, temperature_to_color_pixel(h[i][j]));
					/* draw points */
					XDrawPoint(dpy, w, gc, i, j + ((status.MPI_SOURCE-1)*rowsize));
				}
			}
			XFlush(dpy);
		}
		if (count == 0) {
			globalconvergence = 1;
			/* check exists any one not converged */
			for (i=1; i < numprocs-1; i++) {
	 			if (converged[i] == 0) globalconvergence = 0;
	 			/* reset for next calculation */
	 			converged[i] = 0;
	 		} 		
	 		MPI_Send(&globalconvergence, 1, MPI_INT, i, QUITTAG, MPI_COMM_WORLD);
	 		/* reset the counter */
	 		count = numprocs-1;
	 	}
	 	if (globalconvergence == 1){
	 		for(i=1; i < numprocs; i++) {
	 			MPI_Send(&quitsig, 1, MPI_INT, i, WORKTAG, MPI_COMM_WORLD);
	 			quitsig = 1;
	 		}
	 		global_term = TRUE;
	 		terminate = TRUE;
	 		return;
	 	}
	}

}





