/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <shmem.h>
#include <shmemx.h>
#include "oshmpi_impl.h"

void OSHMPI_space_initialize(void)
{
    OSHMPI_THREAD_INIT_CS(&OSHMPI_global.space_list.cs);
}

void OSHMPI_space_finalize(void)
{
    OSHMPI_THREAD_DESTROY_CS(&OSHMPI_global.space_list.cs);
}

/* Locally create a space with a symmetric heap allocated based on the user
 * specified configure options. */
void OSHMPI_space_create(shmemx_space_config_t space_config, OSHMPI_space_t ** space_ptr)
{
    OSHMPI_space_t *space = OSHMPIU_malloc(sizeof(OSHMPI_space_t));
    OSHMPI_ASSERT(space);
    space->win = MPI_WIN_NULL;

    /* Allocate internal heap. Note that heap may be allocated on device.
     * Thus, we need allocate heap and the space object separately. */
    space->heap_sz = OSHMPI_ALIGN(space_config.sheap_size, OSHMPI_global.page_sz);
    space->heap_base = OSHMPIU_malloc(space->heap_sz);
    OSHMPI_ASSERT(space->heap_base);

    /* Initialize memory pool per space */
    OSHMPIU_mempool_init(&space->mem_pool, space->heap_base, space->heap_sz, OSHMPI_global.page_sz);
    OSHMPI_THREAD_INIT_CS(&space->mem_pool_cs);

    OSHMPI_THREAD_ENTER_CS(&OSHMPI_global.space_list.cs);
    LL_PREPEND(OSHMPI_global.space_list.head, space);
    OSHMPI_global.space_list.nspaces++;
    OSHMPI_THREAD_EXIT_CS(&OSHMPI_global.space_list.cs);

    *space_ptr = (void *) space;
}

/* Locally destroy a space and free the associated symmetric heap. */
void OSHMPI_space_destroy(OSHMPI_space_t * space)
{
    OSHMPI_THREAD_ENTER_CS(&OSHMPI_global.space_list.cs);
    LL_DELETE(OSHMPI_global.space_list.head, space);
    OSHMPI_THREAD_EXIT_CS(&OSHMPI_global.space_list.cs);

    OSHMPIU_mempool_destroy(&space->mem_pool);
    OSHMPI_THREAD_DESTROY_CS(&space->mem_pool_cs);
    OSHMPIU_free(space->heap_base);
    OSHMPIU_free(space);

    OSHMPI_global.space_list.nspaces--;
}

/* Locally create a context associated with the space.
 * RMA/AMO and memory synchronization calls with a space_ctx will only access to the specific space. */
int OSHMPI_space_create_ctx(OSHMPI_space_t * space, long options, OSHMPI_ctx_t ** ctx_ptr)
{
    int shmem_errno = SHMEM_SUCCESS;

    if (space->win == MPI_WIN_NULL) {
        /* Cannot create window before collective attach */
        shmem_errno = SHMEM_NO_CTX;
    } else {
        OSHMPI_ctx_t *ctx = (OSHMPI_ctx_t *) OSHMPIU_malloc(sizeof(OSHMPI_ctx_t));
        ctx->win = space->win;  /* window was created at collective attach */
        *ctx_ptr = ctx;
    }

    return shmem_errno;
}

/* Collectively attach the space into the default team */
void OSHMPI_space_attach(OSHMPI_space_t * space)
{
    MPI_Info info = MPI_INFO_NULL;

    OSHMPI_ASSERT(space->win == MPI_WIN_NULL);  /* space should not be attached yet */

    OSHMPI_CALLMPI(MPI_Info_create(&info));
    OSHMPI_set_mpi_info_args(info);

    OSHMPI_CALLMPI(MPI_Win_create(space->heap_base, (MPI_Aint) space->heap_sz,
                                  1 /* disp_unit */ , info, OSHMPI_global.comm_world,
                                  &space->win));

    OSHMPI_CALLMPI(MPI_Win_lock_all(MPI_MODE_NOCHECK, space->win));
    OSHMPI_CALLMPI(MPI_Info_free(&info));

    OSHMPI_DBGMSG("space_attach space %p, base %p, size %ld, win 0x%x\n",
                  space, space->heap_base, space->heap_sz, space->win);
}

/* Collectively detach the space from the default team */
void OSHMPI_space_detach(OSHMPI_space_t * space)
{
    OSHMPI_ASSERT(space->win != MPI_WIN_NULL);  /* space should be already attached  */

    OSHMPI_CALLMPI(MPI_Win_unlock_all(space->win));
    OSHMPI_CALLMPI(MPI_Win_free(&space->win));
}

/* Collectively allocate a buffer from the space */
void *OSHMPI_space_malloc(OSHMPI_space_t * space, size_t size)
{
    void *ptr = NULL;

    OSHMPI_THREAD_ENTER_CS(&space->mem_pool_cs);
    ptr = OSHMPIU_mempool_alloc(&space->mem_pool, size);
    OSHMPI_THREAD_EXIT_CS(&space->mem_pool_cs);

    OSHMPI_DBGMSG("space_malloc from space %p, size %ld -> ptr %p, disp 0x%lx\n",
                  space, size, ptr, (MPI_Aint) ptr - (MPI_Aint) space->heap_base);
    OSHMPI_barrier_all();
    return ptr;
}

/* Collectively allocate a buffer from the space with byte alignment */
void *OSHMPI_space_align(OSHMPI_space_t * space, size_t alignment, size_t size)
{
    return OSHMPI_space_malloc(space, OSHMPI_ALIGN(size, alignment));
}

/* Collectively free a buffer from the space */
void OSHMPI_space_free(OSHMPI_space_t * space, void *ptr)
{
    OSHMPI_THREAD_ENTER_CS(&space->mem_pool_cs);
    OSHMPIU_mempool_free(&space->mem_pool, ptr);
    OSHMPI_THREAD_EXIT_CS(&space->mem_pool_cs);

    OSHMPI_DBGMSG("space_free from space %p, ptr %p, disp 0x%lx\n",
                  space, ptr, (MPI_Aint) ptr - (MPI_Aint) space->heap_base);
}
