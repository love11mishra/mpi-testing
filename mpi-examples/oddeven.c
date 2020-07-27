#include <crest.h>
#include <mpi/mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {

  int nprocs, rank;

  int num;
  int res;

  CREST_int(num);
  CREST_int(res);

  CR_MPI_Init(&argc, &argv);
  CR_MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  CR_MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    srand(time(NULL));
    num = rand() % 10;
    printf("num generated : %d\n", num);
    CR_MPI_Send(&num, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
    CR_MPI_Send(&num, 1, MPI_INT, 2, 2, MPI_COMM_WORLD);
  }

  if (rank == 1) {
    MPI_Status status;
    int err = CR_MPI_Recv(&num, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

    if (num % 2 == 0)
      res = num * 3;
    else
      res = num;

    err = CR_MPI_Send(&res, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
  }

  if (rank == 2) {
    MPI_Status status;
    int err = CR_MPI_Recv(&num, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &status);

    if (num % 2 != 0)
      res = num * 2;
    else
      res = num;

    err = CR_MPI_Send(&res, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
  }

  if (rank == 0) {
    MPI_Status status;
    int temp;
    int err = CR_MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("Recieved %d from P%d\n", temp, status.MPI_SOURCE);
    res = temp;

    err = CR_MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("Recieved %d from P%d\n", temp, status.MPI_SOURCE);

    res = (temp < res) ? res : temp;
    printf("Result : %d\n", res);
  }

  MPI_Finalize();
  return 0;
}
