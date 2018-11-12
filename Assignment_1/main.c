#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void printArray(int num_procs, int thisRank, int LocalNum, int thisArray[]){
    for (int p = 0; p < num_procs;p++){
        if (p == thisRank){
            printf("Rank %d: ", thisRank);
            for (int m = 0; m < LocalNum; m++) {
                printf("%d ", thisArray[m]);
            }
            printf("\n------------------------------------------------\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

void generate_rand_array(int arrayToSort[], int m){
    srand(1);
    for (int i = 0; i < m; i++) arrayToSort[i] = rand()%100000;
}

void _single_phase_sort(int* a, int index, int size)
{
    int temp;
    for (int i = index; i < size; i += 2)
        if (a[i-1] > a[i]) {
            temp = a[i];
            a[i] = a[i-1];
            a[i-1] = temp;
     }
}

void size_even_case(int rank, int num_procs, int subArray[], int phase, int size){
    int *Large = malloc(1*sizeof(int));
    int *TempLarge = malloc(1*sizeof(int));
    if (phase%2==0) {        
        _single_phase_sort(subArray, 1, size);
    }
    else{                   
        _single_phase_sort(subArray, 2, size);
        if (rank==0){
            if (num_procs > 1){
                //rank 0 only receive
                int *recTemp1 = malloc(1*sizeof(int));
                MPI_Recv(recTemp1,1,MPI_INT,1,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                //compare and send large one back to rank1
                if (recTemp1[0] < subArray[size-1]){
                    Large[0] = subArray[size-1];
                    subArray[size-1]=recTemp1[0];
                }
                else{
                    Large[0] = recTemp1[0];
                }
                MPI_Send(Large,1,MPI_INT,1,4,MPI_COMM_WORLD);
                free(recTemp1);
            }
        }
        else{
            //num send
            int *sendTemp1 = malloc(1*sizeof(int));
            sendTemp1[0] = subArray[0];
            MPI_Send(sendTemp1,1,MPI_INT,rank-1,rank+1,MPI_COMM_WORLD);
            free(sendTemp1);
            //num receive
            if (rank < num_procs-1){
                int *recTemp2 = malloc(1*sizeof(int));
                MPI_Recv(recTemp2,1,MPI_INT,rank+1,rank+2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                //compare and send back
                if (recTemp2[0] < subArray[size-1]){
                    Large[0] = subArray[size-1];
                    subArray[size-1]=recTemp2[0];
                }
                else{
                    Large[0] = recTemp2[0];
                }
                MPI_Send(Large,1,MPI_INT,rank+1,rank+4,MPI_COMM_WORLD);
                free(recTemp2);
            }
            MPI_Recv(TempLarge,1,MPI_INT,rank-1,rank+3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            subArray[0] = TempLarge[0];
        }
    }
    free(TempLarge);
    free(Large);
}

void size_odd_case(int rank, int num_procs, int subArray[], int phase, int size){
    int *Large = malloc(1*sizeof(int));
    int *TempLarge = malloc(1*sizeof(int));
    if (phase%2==0) { //even sort
        _single_phase_sort(subArray, rank%2+1, size);
        if (rank%2==0) {//even rank processor
            if (rank < num_procs-1) {
                //receive
                int *recTemp3 = malloc(1*sizeof(int));
                MPI_Recv(recTemp3,1,MPI_INT,rank+1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                //compare and send back
                if (recTemp3[0] < subArray[size-1]){
                    Large[0] = subArray[size-1];
                    subArray[size-1]=recTemp3[0];
                }
                else{
                    Large[0] = recTemp3[0];
                }
                MPI_Send(Large,1,MPI_INT,rank+1,rank+1,MPI_COMM_WORLD);
                free(recTemp3);
            }
        }
        else{//odd rank processor
            //send
            int *sendTemp3 = malloc(1*sizeof(int));
            sendTemp3[0] = subArray[0];
            MPI_Send(sendTemp3,1,MPI_INT,rank-1,0,MPI_COMM_WORLD);
            free(sendTemp3);
            //receive large one and compare
            MPI_Recv(TempLarge,1,MPI_INT,rank-1,rank,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            subArray[0]=TempLarge[0];
        }
    }
    else{ //odd sort
        _single_phase_sort(subArray, 2-rank%2, size);
        if (rank%2==1) {//odd rank processor
            if (rank<num_procs-1){
                //receive
                int *recTemp4 = malloc(1*sizeof(int));
                MPI_Recv(recTemp4,1,MPI_INT,rank+1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                //compare and send
                if (recTemp4[0]<subArray[size-1]){
                    Large[0] = subArray[size-1];
                    subArray[size-1]=recTemp4[0];
                }else{
                    Large[0] = recTemp4[0];
                }
                MPI_Send(Large,1,MPI_INT,rank+1,rank+1,MPI_COMM_WORLD);
                free(recTemp4);
            }
        }else{//even rank processor
            if (rank > 0){
                //send
                int *sendTemp4 = malloc(1*sizeof(int));
                sendTemp4[0] = subArray[0];
                MPI_Send(sendTemp4,1,MPI_INT,rank-1,1,MPI_COMM_WORLD);
                free(sendTemp4);
                //receive
                MPI_Recv(TempLarge,1,MPI_INT,rank-1,rank,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                subArray[0] = TempLarge[0];
            }
        }
    }
    free(TempLarge);
    free(Large);
}



int main(int argc, char **argv) {
    
    int rank, num_procs; 
    MPI_Status status;
   
    //init MPI
    MPI_Init(&argc,&argv);
    //get the number of processors you have 
    MPI_Comm_size(MPI_COMM_WORLD,&num_procs);    
    //get the rank of the processor where this code run
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    // number defined as # of total integers
    int number = atoi(argv[1]);
    // size defined as # of integers in the prosessor
    int size = number / num_procs;
    // arrayToSort is random target array we want to sort
    int *arrayToSort = malloc(number * sizeof(int));
    // array scattered in every processor
    int *subArray = malloc(size * sizeof(int));

    //generate arrayToSort with random number
    if (rank == 0) { //do this only in rank0 processor
        printf("Name: Jiahan Yu\nID:   116010274\n");
        generate_rand_array(arrayToSort, number);
        printf("\nOrigin array: \n");
        for (int i = 0; i < number; i++) {
            printf("%d ", arrayToSort[i]);
        }
        printf("\n------------------------------------------------\n");
    }

    //setting the parameters of MPI_Scatterv
    int *displs = malloc(num_procs * sizeof(int));
    for (int i = 0; i < num_procs; i++) displs[i] = i * (size);
    int *Send_count = malloc(num_procs * sizeof(int));
    for (int i = 0; i < num_procs - 1; i++) Send_count[i] = size;
    Send_count[num_procs - 1] = number - (size * (num_procs - 1));    //set size of last processor

    /* scatter the arrayToSort to every processor,
     * each processor receive Send_count[rank] # of nums, denoted as subArray
     */
    MPI_Scatterv(arrayToSort, Send_count, displs, MPI_INT, subArray, Send_count[rank], MPI_INT, 0, MPI_COMM_WORLD);

    //print out original subArray in processor
    if (rank==0){
        printf("\nBefore sorting\n");
        printf("------------------------------------------------\n");

    }
    printArray(num_procs, rank, Send_count[rank], subArray);
    
    /*init a timer*/
    double start_time, end_time;
    double sort_time;
    start_time = MPI_Wtime();

    //address the problem by cases, even/odd size
    if (size % 2 == 0) {       //even size
        for (int i=0;i<number;i++) {
            size_even_case(rank, num_procs, subArray, i, Send_count[rank]);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    } else {                   //odd size
        for (int i=0;i<number;i++) {
            size_odd_case(rank, num_procs, subArray, i, Send_count[rank]);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    end_time = MPI_Wtime();
    sort_time = end_time - start_time;

    //print out current subArrays in each processor
    if (rank==0){
        printf("\nAfter sorting\n");
        printf("------------------------------------------------\n");
    }
    printArray(num_procs, rank, Send_count[rank], subArray);

    //gather all subArrays
    MPI_Gatherv(subArray,Send_count[rank],MPI_INT,arrayToSort,Send_count,displs,MPI_INT,0,MPI_COMM_WORLD);

    /* The rank0 processor display the sorted array */
    if(rank == 0) {

        printf("\nSorted array :\n");
        for (int i=0;i<number;i++) {
            printf("%d; ",arrayToSort[i]); 
        }
        printf("\n\nSorted in %f s\n\n",sort_time);
    }

    //deloc MPI
    MPI_Finalize();
}







