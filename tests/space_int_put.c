/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <shmem.h>
#include <shmemx.h>

#define SIZE 10
#define ITER 10

int main(int argc, char *argv[])
{
    int mype, errs = 0;

    shmem_init();
    mype = shmem_my_pe();

    shmemx_space_config_t space_config;
    shmemx_space_t space;

    space_config.sheap_size = 1 << 20;
    space_config.num_contexts = 0;
    shmemx_space_create(space_config, &space);
    shmemx_space_attach(space);

    int *src = shmemx_space_malloc(space, SIZE * ITER * sizeof(int));
    int *dst = shmemx_space_malloc(space, SIZE * ITER * sizeof(int));

    int i, x;
    for (i = 0; i < SIZE * ITER; i++) {
        src[i] = mype + i;
        dst[i] = 0;
    }
    shmem_barrier_all();

    for (x = 0; x < ITER; x++) {
        int off = x * SIZE;
        if (mype == 0) {
            shmem_int_put(&dst[off], &src[off], SIZE, 1);
            shmem_quiet();
        }
    }

    shmem_barrier_all();

    if (mype == 1) {
        for (i = 0; i < SIZE * ITER; i++) {
            if (dst[i] != i) {
                fprintf(stderr, "Excepted %d at dst[%d], but %d\n", i, i, dst[i]);
                fflush(stderr);
                errs++;
            }
        }
    }

    shmem_free(dst);
    shmem_free(src);

    shmemx_space_detach(space);
    shmem_finalize();

    if (mype == 1 && errs == 0) {
        fprintf(stdout, "Passed\n");
        fflush(stderr);
    }
    return 0;
}
