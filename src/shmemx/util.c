/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <shmem.h>
#include <shmemx.h>
#include "oshmpi_impl.h"

OSHMPI_TIMER_EXTERN_DECL(iput_dtype_create);
OSHMPI_TIMER_EXTERN_DECL(iput_put_nbi);
OSHMPI_TIMER_EXTERN_DECL(iput_local_comp);
OSHMPI_TIMER_EXTERN_DECL(iput_dtype_free);
OSHMPI_TIMER_EXTERN_DECL(iput);
OSHMPI_TIMER_EXTERN_DECL(quiet);

void shmemx_print_timer(double factor, const char *prefix)
{
#ifdef OSHMPI_ENABLE_TIMER
    OSHMPI_PRINTF("[OSHMPI timer] %s", prefix);
    OSHMPI_PRINT_TIMER(iput_dtype_create, factor);
    OSHMPI_PRINT_TIMER(iput_put_nbi, factor);
    OSHMPI_PRINT_TIMER(iput_local_comp, factor);
    OSHMPI_PRINT_TIMER(iput_dtype_free, factor);
    OSHMPI_PRINT_TIMER(iput, factor);
    OSHMPI_PRINT_TIMER(quiet, factor);
    OSHMPI_PRINTF("\n");
#else
    OSHMPI_PRINTF("OSHMPI timer is disabled, recompile with CFLAGS=-DOSHMPI_ENABLE_TIMER to enable.\n");
#endif
}

void shmemx_timer_reset(void)
{
    OSHMPI_TIMER_RESET(iput_dtype_create);
    OSHMPI_TIMER_RESET(iput_put_nbi);
    OSHMPI_TIMER_RESET(iput_local_comp);
    OSHMPI_TIMER_RESET(iput_dtype_free);
    OSHMPI_TIMER_RESET(iput);
    OSHMPI_TIMER_RESET(quiet);
}
