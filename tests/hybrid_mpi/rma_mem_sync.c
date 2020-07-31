/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <shmem.h>
#include <mpi.h>


int *mpi_ranks = NULL;
int mype, npes;

void collect_mpi_ids(void)
{
    static long pSync[SHMEM_COLLECT_SYNC_SIZE];
    int i;
    mpi_ranks = shmem_malloc(sizeof(int) * npes);

    shmem_barrier_all();
    shmem_collect32(mpi_ranks, mype, 1, 0, 0, npes, pSync);

    for (i = 0; i < npes; i++)
        printf("PE %d's MPI rank is %d\n", i, mpi_ranks[i]);

    shmem_free(mpi_ranks);
}

#define ARRAY_SIZE 1024
int main(int argc, char *argv[])
{
    int mype, npes, rank;
    int *array = NULL, a;
    MPI_Win win;

    shmem_init();
    MPI_Init(&argc, &argv);

    mype = shmem_my_pe();
    npes = shmem_n_pes();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    array = shmem_malloc(ARRAY_SIZE * npes);

    /* create MPI window for the shmem symmetric heap */
    MPI_Win_create(&array, ARRAY_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

    shmem_int_put(&array[mype], &mype, 1, 0);
    shmem_barrier_all();        /* ensure all remote updates by shmem_put are completed on PE 0. */

    if (rank == 0) {
        MPI_Win_sync(win);
    }
    MPI_Win_fence(0, win);      /* ensure shmem updates are visible in both MPI public and private window copies */

    MPI_Get(&a, 1, MPI_INT, 0, mype, 1, MPI_INT, win);
    MPI_Win_flush(0, win);

    MPI_Win_unlock(0, win);
    MPI_Win_free(&win);

    shmem_free(array);

    shmem_finalize();
    MPI_Finalize();

    return 0;
}
