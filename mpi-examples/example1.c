#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])

{
  int numprocs, myrank;
  MPI_Init( &argc, &argv );
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  if(myrank == 0){
      //sends its rank to all others
      for(int i = 1; i < numprocs; i++){
          MPI_Send(&myrank, 1, MPI_INT, i, i + 5, MPI_COMM_WORLD);
      }
  }
  else{
      MPI_Status status;
      int recv_buffer = 99; //some random value
      int err = MPI_Recv(&recv_buffer,1, MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD, &status);
      if(err == MPI_SUCCESS){
            printf("[rank]: %d Recieved %d form [rank]: %d\n",myrank,recv_buffer,status.MPI_SOURCE);
      }
  }
  MPI_Finalize();
  return 0;
 }
