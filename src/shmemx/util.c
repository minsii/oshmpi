/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <shmem.h>
#include <shmemx.h>
#include "oshmpi_impl.h"

void shmemx_print_timer(void)
{
#ifdef OSHMPI_ENABLE_TIMER
#else
    OSHMPI_PRINTF("OSHMPI timer is disabled, recompile with -DOSHMPI_ENABLE_TIMER to enable.\n");
#endif
}

void shmemx_timer_reset(void)
{
}
