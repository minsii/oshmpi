/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <shmem.h>
#include <shmemx.h>

int main(int argc, char *argv[])
{
    int mype;

    shmem_init();
    mype = shmem_my_pe();

    shmemx_space_config_t space_config;
    shmemx_space_t space1, space2;

    space_config.sheap_size = 1 << 20;
    space_config.num_contexts = 0;
#ifdef USE_CUDA
    space_config.memkind = SHMEMX_SPACE_CUDA;
#endif

    shmemx_space_create(space_config, &space1);
    shmemx_space_create(space_config, &space2);

    shmemx_space_attach(space1);
    shmemx_space_attach(space2);

    int *sbuf = shmemx_space_malloc(space1, 8192);
    int *rbuf = shmemx_space_malloc(space1, 8192);

    shmem_free(sbuf);
    shmem_free(rbuf);

    shmemx_space_detach(space2);
    shmemx_space_detach(space1);

    shmemx_space_destroy(space1);
    shmemx_space_destroy(space2);
    shmem_finalize();

    if (mype == 0) {
        fprintf(stdout, "Passed\n");
        fflush(stderr);
    }
    return 0;
}
