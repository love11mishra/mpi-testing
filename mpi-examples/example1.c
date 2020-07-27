#include <crest.h>
#include <mpi/mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])

{
  int numprocs, myrank;

  CREST_int(numprocs);

  CR_MPI_Init(&argc, &argv);
  CR_MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  CR_MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  if (myrank == 0) {
    // sends its rank to all others
    for (int i = 1; i < numprocs; i++) {
      CR_MPI_Send(&i, 1, MPI_INT, i, i + 5, MPI_COMM_WORLD);
    }

  } else {
    MPI_Status status;
    int recv_buffer = 99;  // some random value
    int err = CR_MPI_Recv(&recv_buffer, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if (err == MPI_SUCCESS) {
      printf("[rank]: %d Recieved %d form [rank]: %d\n", myrank, recv_buffer, status.MPI_SOURCE);
    }
  }

  CR_MPI_Finalize();
  return 0;
}
