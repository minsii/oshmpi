/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */
#ifndef OSHMPI_IMPL_H
#define OSHMPI_IMPL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <oshmpiconf.h>
#if defined(OSHMPI_ENABLE_THREAD_MULTIPLE) || defined(OSHMPI_ENABLE_THREAD_SERIALIZED)
#include <pthread.h>
#endif
#include <shmem.h>
#include <shmemx.h>

#include "dlmalloc.h"
#include "oshmpi_util.h"
#include "am_pkt_def.h"

#define OSHMPI_DEFAULT_SYMM_HEAP_SIZE (1L<<27)  /* 128MB */
#define OSHMPI_DEFAULT_DEBUG 0

/* DLMALLOC minimum allocated size (see create_mspace_with_base)
 * Part (less than 128*sizeof(size_t) bytes) of this space is used for bookkeeping,
 * so the capacity must be at least this large */
#define OSHMPI_DLMALLOC_MIN_MSPACE_SIZE (128 * sizeof(size_t))

#define OSHMPI_MPI_COLL32_T MPI_UINT32_T
#define OSHMPI_MPI_COLL64_T MPI_UINT64_T

#define OSHMPI_LOCK_MSG_TAG 999 /* For lock routines */

#ifdef OSHMPI_ENABLE_THREAD_SINGLE
#define OSHMPI_DEFAULT_THREAD_SAFETY SHMEM_THREAD_SINGLE
#elif defined(OSHMPI_ENABLE_THREAD_SERIALIZED)
#define OSHMPI_DEFAULT_THREAD_SAFETY SHMEM_THREAD_SERIALIZED
#elif defined(OSHMPI_ENABLE_THREAD_FUNNELED)
#define OSHMPI_DEFAULT_THREAD_SAFETY SHMEM_THREAD_FUNNELED
#else /* defined OSHMPI_ENABLE_THREAD_MULTIPLE */
#define OSHMPI_DEFAULT_THREAD_SAFETY SHMEM_THREAD_MULTIPLE
#endif

#if defined(OSHMPI_ENABLE_THREAD_MULTIPLE)
#include "opa_primitives.h"
typedef OPA_int_t OSHMPI_atomic_flag_t;
#define OSHMPI_ATOMIC_FLAG_STORE(flag, val) OPA_store_int(&(flag), val)
#define OSHMPI_ATOMIC_FLAG_LOAD(flag) OPA_load_int(&(flag))
#define OSHMPI_ATOMIC_FLAG_CAS(flag, old, new) OPA_cas_int(&(flag), (old), (new))

typedef OPA_int_t OSHMPI_atomic_cnt_t;
#define OSHMPI_ATOMIC_CNT_STORE(cnt, val) OPA_store_int(&(cnt), val)
#define OSHMPI_ATOMIC_CNT_LOAD(cnt) OPA_load_int(&(cnt))
#define OSHMPI_ATOMIC_CNT_INCR(cnt) OPA_incr_int(&(cnt))
#define OSHMPI_ATOMIC_CNT_DECR(cnt) OPA_decr_int(&(cnt))
#else
typedef unsigned int OSHMPI_atomic_flag_t;
#define OSHMPI_ATOMIC_FLAG_STORE(flag, val) do {(flag) = (val);} while (0)
#define OSHMPI_ATOMIC_FLAG_LOAD(flag) (flag)
#define OSHMPI_ATOMIC_FLAG_CAS(flag, old, new) OSHMPIU_single_thread_cas_int(&(flag), old, new)

typedef unsigned int OSHMPI_atomic_cnt_t;
#define OSHMPI_ATOMIC_CNT_STORE(cnt, val) do {(cnt) = (val);} while (0)
#define OSHMPI_ATOMIC_CNT_LOAD(cnt) (cnt)
#define OSHMPI_ATOMIC_CNT_INCR(cnt) do {(cnt)++;} while (0)
#define OSHMPI_ATOMIC_CNT_DECR(cnt) do {(cnt)--;} while (0)
#endif

typedef enum {
    OSHMPI_SOBJ_SYMM_DATA = 0,
    OSHMPI_SOBJ_SYMM_HEAP = 1,
    OSHMPI_SOBJ_SPACE_HEAP = 2,
    OSHMPI_SOBJ_SPACE_ATTACHED_HEAP = 3,
} OSHMPI_sobj_kind_t;

#define OSHMPI_SOBJ_HANDLE_KIND_MASK 0xc0000000
#define OSHMPI_SOBJ_HANDLE_KIND_SHIFT 30
#define OSHMPI_SOBJ_HANDLE_IDX_MASK 0x3FFFFFFF
#define OSHMPI_SOBJ_HANDLE_GET_KIND(handle) (((handle) & OSHMPI_SOBJ_HANDLE_KIND_MASK) >> OSHMPI_SOBJ_HANDLE_KIND_SHIFT)
#define OSHMPI_SOBJ_HANDLE_GET_IDX(handle) ((handle) & 0x3FFFFFFF)
#define OSHMPI_SET_SOBJ_HANDLE(handle, kind, idx) do { (handle) = ((kind) << OSHMPI_SOBJ_HANDLE_KIND_SHIFT) | (idx); } while (0)

/* Predefined symm object handles */
#define OSHMPI_SOBJ_HANDLE_SYMM_DATA (OSHMPI_SOBJ_SYMM_DATA << OSHMPI_SOBJ_HANDLE_KIND_SHIFT)
#define OSHMPI_SOBJ_HANDLE_SYMM_HEAP (OSHMPI_SOBJ_SYMM_HEAP << OSHMPI_SOBJ_HANDLE_KIND_SHIFT)

#if defined(OSHMPI_ENABLE_DIRECT_AMO)
#define OSHMPI_ENABLE_DIRECT_AMO_RUNTIME 1
#elif defined(OSHMPI_ENABLE_AM_AMO)
#define OSHMPI_ENABLE_DIRECT_AMO_RUNTIME 0
#else /* default make decision at runtime */
#define OSHMPI_ENABLE_DIRECT_AMO_RUNTIME (OSHMPI_global.amo_direct)
#endif

typedef enum {
    OSHMPI_PUT,
    OSHMPI_GET,
} OSHMPI_rma_op_t;

OSHMPI_STATIC_INLINE_PREFIX int OSHMPI_check_gpu_direct_rma(const void *origin_addr,
                                                            shmemx_space_memkind_t sobj_memkind,
                                                            OSHMPI_rma_op_t rma);

#if defined(OSHMPI_ENABLE_AM_RMA)
#define OSHMPI_ENABLE_DIRECT_RMA_RUNTIME(origin_addr, sobj_memkind, rma) 0
#elif defined(OSHMPI_ENABLE_DIRECT_RMA)
#define OSHMPI_ENABLE_DIRECT_RMA_RUNTIME(origin_addr, sobj_memkind, rma) 1
#else /* default make decision at runtime */
#define OSHMPI_ENABLE_DIRECT_RMA_RUNTIME(origin_addr, sobj_memkind, rma) \
            OSHMPI_check_gpu_direct_rma(origin_addr, sobj_memkind, rma)
#endif

#if defined(OSHMPI_ENABLE_AM_ASYNC_THREAD)
#define OSHMPI_ENABLE_AM_ASYNC_THREAD_RUNTIME 1
#elif defined(OSHMPI_RUNTIME_AM_ASYNC_THREAD)
#define OSHMPI_ENABLE_AM_ASYNC_THREAD_RUNTIME (OSHMPI_env.enable_async_thread)
#else /* default disable async thread */
#define OSHMPI_ENABLE_AM_ASYNC_THREAD_RUNTIME 0
#endif

typedef struct OSHMPI_comm_cache_obj {
    int pe_start;
    int pe_stride;
    int pe_size;
    MPI_Comm comm;
    MPI_Group group;            /* Cached in case we need to translate root rank. */
    struct OSHMPI_comm_cache_obj *next;
} OSHMPI_comm_cache_obj_t;

typedef struct OSHMPI_comm_cache_list {
    OSHMPI_comm_cache_obj_t *head;
    int nobjs;
} OSHMPI_comm_cache_list_t;

typedef struct OSHMPI_ictx {
    MPI_Win win;
    unsigned int outstanding_op;
} OSHMPI_ictx_t;

typedef struct OSHMPI_sobj_attr {
    uint32_t handle;
    shmemx_space_memkind_t memkind;
    void *base;
    MPI_Aint size;
} OSHMPI_sobj_attr_t;

typedef struct OSHMPI_ctx {
    OSHMPI_ictx_t ictx;
    OSHMPI_atomic_flag_t used_flag;
    OSHMPI_sobj_attr_t sobj_attr;
} OSHMPI_ctx_t;

typedef struct OSHMPI_space {
    OSHMPI_sobj_attr_t sobj_attr;
    OSUMPIU_mempool_t mem_pool;
    OSHMPIU_thread_cs_t mem_pool_cs;
    OSHMPI_ictx_t default_ictx;
    OSHMPI_ctx_t *ctx_list;     /* contexts created for this space. */

    shmemx_space_config_t config;
    struct OSHMPI_space *next;
} OSHMPI_space_t;

typedef struct OSHMPI_space_list {
    OSHMPI_space_t *head;
    int nspaces;
    OSHMPIU_thread_cs_t cs;
} OSHMPI_space_list_t;

struct OSHMPI_am_pkt;

typedef struct {
    int is_initialized;
    int is_start_pes_initialized;
    int world_rank;
    int world_size;
    int thread_level;
    size_t page_sz;

    MPI_Comm comm_world;        /* duplicate of COMM_WORLD */
    MPI_Group comm_world_group;

#ifdef OSHMPI_ENABLE_DYNAMIC_WIN
    OSHMPI_ictx_t symm_ictx;
    int symm_base_flag;         /* if both heap and data are symmetrically allocated. flag: 1 or 0 */
    int symm_heap_flag;         /* if heap is symmetrically allocated. flag: 1 or 0 */
    int symm_data_flag;         /* if data is symmetrically allocated. flag: 1 or 0 */

    MPI_Aint *symm_heap_bases;
    MPI_Aint *symm_data_bases;
#else
    OSHMPI_ictx_t symm_heap_ictx;
    OSHMPI_ictx_t symm_data_ictx;
#endif

    void *symm_heap_base;
    MPI_Aint symm_heap_base_end;
    MPI_Aint symm_heap_size;
    MPI_Aint symm_heap_true_size;
    mspace symm_heap_mspace;
    OSHMPIU_thread_cs_t symm_heap_mspace_cs;
    void *symm_data_base;
    MPI_Aint symm_data_size;
    MPI_Aint symm_data_base_end;

    OSHMPI_comm_cache_list_t comm_cache_list;
    OSHMPIU_thread_cs_t comm_cache_list_cs;

    OSHMPI_space_list_t space_list;

    /* Active message based AMO */
    MPI_Comm am_comm_world;     /* duplicate of COMM_WORLD, used for packet */
    MPI_Comm am_ack_comm_world; /* duplicate of COMM_WORLD, used for packet ACK */
#if defined(OSHMPI_ENABLE_AM_ASYNC_THREAD) || defined(OSHMPI_RUNTIME_AM_ASYNC_THREAD)
    pthread_mutex_t am_async_mutex;
    pthread_cond_t am_async_cond;
    volatile int am_async_thread_done;
    pthread_t am_async_thread;
#endif
    OSHMPI_atomic_flag_t *am_outstanding_op_flags;      /* flag indicating whether outstanding AM
                                                         * based AMO or RMA exists. When a post AMO (nonblocking)
                                                         * has been issued, this flag becomes 1; when
                                                         * a flush or fetch/cswap AMO issued, reset to 0;
                                                         * We only need flush a remote PE when flag is 1.*/
    MPI_Request am_req;
    struct OSHMPI_am_pkt *am_pkt;       /* Temporary pkt for receiving incoming active message.
                                         * Type OSHMPI_am_pkt_t is loaded later than global struct,
                                         * thus keep it as pointer. */
    MPI_Datatype *am_datatypes_table;
    MPI_Op *am_ops_table;
    OSHMPIU_thread_cs_t am_cb_progress_cs;

    unsigned int amo_direct;    /* Valid only when --enable-amo=runtime is set.
                                 * User may control it through env var
                                 * OSHMPI_AMO_OPS (see amo_ops in OSHMPI_env_t). */
} OSHMPI_global_t;

typedef enum {
    OSHMPI_AMO_CSWAP,
    OSHMPI_AMO_FINC,
    OSHMPI_AMO_INC,
    OSHMPI_AMO_FADD,
    OSHMPI_AMO_ADD,
    OSHMPI_AMO_FETCH,
    OSHMPI_AMO_SET,
    OSHMPI_AMO_SWAP,
    OSHMPI_AMO_FAND,
    OSHMPI_AMO_AND,
    OSHMPI_AMO_FOR,
    OSHMPI_AMO_OR,
    OSHMPI_AMO_FXOR,
    OSHMPI_AMO_XOR,
    OSHMPI_AMO_OP_LAST,
} OSHMPI_amo_op_shift_t;

typedef enum {
    OSHMPI_MPI_GPU_PT2PT,       /* MPI supports PT2PT with GPU buffers */
    OSHMPI_MPI_GPU_PUT,         /* MPI supports PUT with GPU buffers */
    OSHMPI_MPI_GPU_GET,         /* MPI supports GET with GPU buffers */
    OSHMPI_MPI_GPU_ACCUMULATES, /* MPI supports ACCUMULATES with GPU buffers */
} OSHMPI_mpi_gpu_feature_shift_t;

#define OSHMPI_CHECK_MPI_GPU_FEATURE(f) (OSHMPI_env.mpi_gpu_features & (1<<(f)))
#define OSHMPI_SET_MPI_GPU_FEATURE(f) (1<<(f))

typedef struct {
    /* SHMEM standard environment variables */
    MPI_Aint symm_heap_size;    /* SHMEM_SYMMETRIC_SIZE: Number of bytes to allocate for symmetric heap.
                                 * Value: Non-negative integer. Default OSHMPI_DEFAULT_SYMM_HEAP_SIZE. */
    unsigned int debug;         /* SHMEM_DEBUG: Enable debugging messages.
                                 * Value: 0 (default) |any non-zero value.
                                 * Always disabled when --enable-fast is set. */
    unsigned int version;       /* SHMEM_VERSION: Print the library version at start-up.
                                 * Value: 0 (default) |any non-zero value. */
    unsigned int info;          /* SHMEM_INFO: Print helpful text about all these environment variables.
                                 * Value: 0 (default) |any non-zero value. */

    /* OSHMPI extended environment variables */
    unsigned int verbose;       /* OSHMPI_VERBOSE: Print value of all OSHMPI configuration including
                                 * SHMEM standard environment varibales and OSHMPI extension.
                                 * Value: 0 (default) |any non-zero value. */
    uint32_t amo_ops;           /* OSHMPI_AMO_OPS: Arbitrary combination with bit shift defined in
                                 * OSHMPI_amo_op_shift_t. any_op and none are two special values.
                                 * any_op by default. */
    unsigned int enable_async_thread;   /* OSHMPI_ENABLE_ASYNC_THREAD:
                                         * Valid only when OSHMPI_RUNTIME_AM_ASYNC_THREAD
                                         * is set.*/
    uint32_t mpi_gpu_features;  /* OSHMPI_MPI_GPU_FEATURES: Arbitrary combination with bit shift defined in
                                 * OSHMPI_mpi_gpu_feature_shift_t. none and all are two special values. */
} OSHMPI_env_t;

#ifdef OSHMPI_ENABLE_IPO        /* define empty bracket to be compatible with code cleanup script */
#define OSHMPI_FORCEINLINE() _Pragma("forceinline")
#define OSHMPI_NOINLINE_RECURSIVE() _Pragma("noinline recursive")
#else
#define OSHMPI_FORCEINLINE() ;
#define OSHMPI_NOINLINE_RECURSIVE() ;
#endif

extern OSHMPI_global_t OSHMPI_global;
extern OSHMPI_env_t OSHMPI_env;

/* Per-object critical section MACROs. */
#ifdef OSHMPI_ENABLE_THREAD_MULTIPLE
#define OSHMPI_THREAD_INIT_CS(cs_ptr)  do {                   \
    if (OSHMPI_global.thread_level == SHMEM_THREAD_MULTIPLE) {\
        int __err = OSHMPIU_thread_cs_init(cs_ptr);           \
        OSHMPI_ASSERT(!__err);                                \
    }                                                         \
} while (0)

#define OSHMPI_THREAD_DESTROY_CS(cs_ptr)  do {                 \
    if (OSHMPI_global.thread_level == SHMEM_THREAD_MULTIPLE && \
            OSHMPIU_THREAD_CS_IS_INITIALIZED(cs_ptr)) {        \
        int __err = OSHMPIU_thread_cs_destroy(cs_ptr);         \
        OSHMPI_ASSERT(!__err);                                 \
    }                                                          \
} while (0)

#define OSHMPI_THREAD_ENTER_CS(cs_ptr)  do {                          \
    if (OSHMPI_global.thread_level == SHMEM_THREAD_MULTIPLE) {        \
        int __err;                                                    \
        __err = OSHMPIU_THREAD_CS_ENTER(cs_ptr);                      \
        OSHMPI_ASSERT(!__err);                                        \
    }                                                                 \
} while (0)

#define OSHMPI_THREAD_EXIT_CS(cs_ptr)  do {                     \
    if (OSHMPI_global.thread_level == SHMEM_THREAD_MULTIPLE) {  \
        int __err = 0;                                          \
        __err = OSHMPIU_THREAD_CS_EXIT(cs_ptr);                 \
        OSHMPI_ASSERT(!__err);                                  \
    }                                                           \
} while (0)

#else /* OSHMPI_ENABLE_THREAD_MULTIPLE */
#define OSHMPI_THREAD_INIT_CS(cs_ptr)
#define OSHMPI_THREAD_DESTROY_CS(cs_ptr)
#define OSHMPI_THREAD_ENTER_CS(cs_ptr)
#define OSHMPI_THREAD_EXIT_CS(cs_ptr)
#endif /* OSHMPI_ENABLE_THREAD_MULTIPLE */

/* SHMEM internal routines. */
int OSHMPI_initialize_thread(int required, int *provided);
void OSHMPI_implicit_finalize(void);
int OSHMPI_finalize(void);
void OSHMPI_global_exit(int status);
void OSHMPI_set_mpi_info_args(MPI_Info info);

void *OSHMPI_malloc(size_t size);
void OSHMPI_free(void *ptr);
void *OSHMPI_realloc(void *ptr, size_t size);
void *OSHMPI_align(size_t alignment, size_t size);

void OSHMPI_space_initialize(void);
void OSHMPI_space_finalize(void);
void OSHMPI_space_create(shmemx_space_config_t space_config, OSHMPI_space_t ** space_ptr);
void OSHMPI_space_destroy(OSHMPI_space_t * space);
int OSHMPI_space_create_ctx(OSHMPI_space_t * space, long options, OSHMPI_ctx_t ** ctx_ptr);
void OSHMPI_space_attach(OSHMPI_space_t * space);
void OSHMPI_space_detach(OSHMPI_space_t * space);
void *OSHMPI_space_malloc(OSHMPI_space_t * space, size_t size);
void *OSHMPI_space_align(OSHMPI_space_t * space, size_t alignment, size_t size);
void OSHMPI_space_free(OSHMPI_space_t * space, void *ptr);

void OSHMPI_ctx_destroy(OSHMPI_ctx_t * ctx);

/* Subroutines for rma. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_put_nbi(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                    MPI_Datatype mpi_type, size_t typesz,
                                                    const void *origin_addr, void *target_addr,
                                                    size_t nelems, int pe);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_put(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                MPI_Datatype mpi_type, size_t typesz,
                                                const void *origin_addr, void *target_addr,
                                                size_t nelems, int pe);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_iput(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                 MPI_Datatype mpi_type,
                                                 OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                 const void *origin_addr, void *target_addr,
                                                 ptrdiff_t target_st, ptrdiff_t origin_st,
                                                 size_t nelems, int pe);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_get_nbi(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                    MPI_Datatype mpi_type, size_t typesz,
                                                    void *origin_addr, const void *target_addr,
                                                    size_t nelems, int pe);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_get(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                MPI_Datatype mpi_type, size_t typesz,
                                                void *origin_addr, const void *target_addr,
                                                size_t nelems, int pe);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_iget(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                 MPI_Datatype mpi_type,
                                                 OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                 void *origin_addr, const void *target_addr,
                                                 ptrdiff_t origin_st, ptrdiff_t target_st,
                                                 size_t nelems, int pe);

/* Subroutines for am rma. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_put(OSHMPI_ictx_t * ictx,
                                                   MPI_Datatype mpi_type, size_t typesz,
                                                   const void *origin_addr, MPI_Aint target_disp,
                                                   size_t nelems, int pe, uint32_t sobj_handle);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_get(OSHMPI_ictx_t * ictx,
                                                   MPI_Datatype mpi_type, size_t typesz,
                                                   void *origin_addr, MPI_Aint target_disp,
                                                   size_t nelems, int pe, uint32_t sobj_handle);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_iput(OSHMPI_ictx_t * ictx,
                                                    MPI_Datatype mpi_type,
                                                    OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                    const void *origin_addr, MPI_Aint target_disp,
                                                    ptrdiff_t origin_st, ptrdiff_t target_st,
                                                    size_t nelems, int pe, uint32_t sobj_handle);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_iget(OSHMPI_ictx_t * ictx,
                                                    MPI_Datatype mpi_type,
                                                    OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                    void *origin_addr, MPI_Aint target_disp,
                                                    ptrdiff_t origin_st, ptrdiff_t target_st,
                                                    size_t nelems, int pe, uint32_t sobj_handle);

OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_put_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_get_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_iput_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_rma_am_iget_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);

/* Subroutines for collectives. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_coll_initialize(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_coll_finalize(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_barrier_all(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_barrier(int PE_start, int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_sync_all(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_sync(int PE_start, int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_broadcast(void *dest, const void *source, size_t nelems,
                                                  MPI_Datatype mpi_type, int PE_root, int PE_start,
                                                  int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_collect(void *dest, const void *source, size_t nelems,
                                                MPI_Datatype mpi_type, int PE_start,
                                                int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_fcollect(void *dest, const void *source, size_t nelems,
                                                 MPI_Datatype mpi_type, int PE_start,
                                                 int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_alltoall(void *dest, const void *source, size_t nelems,
                                                 MPI_Datatype mpi_type, int PE_start,
                                                 int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_alltoalls(void *dest, const void *source, ptrdiff_t dst,
                                                  ptrdiff_t sst, size_t nelems,
                                                  MPI_Datatype mpi_type, int PE_start,
                                                  int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_allreduce(void *dest, const void *source, int count,
                                                  MPI_Datatype mpi_type, MPI_Op op, int PE_start,
                                                  int logPE_stride, int PE_size);

OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_fence(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)));
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_ctx_quiet(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)));

OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_set_lock(long *lockp);
OSHMPI_STATIC_INLINE_PREFIX int OSHMPI_test_lock(long *lockp);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_clear_lock(long *lockp);

/* Subroutines for am routines. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_initialize(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_finalize(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_cb_progress(void);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_flush(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                 int PE_start, int logPE_stride, int PE_size);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_flush_all(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)));
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_flush_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);

/* Subroutines for atomics. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_cswap(shmem_ctx_t ctx
                                                  OSHMPI_ATTRIBUTE((unused)), MPI_Datatype mpi_type,
                                                  OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                  size_t bytes, void *dest, void *cond_ptr,
                                                  void *value_ptr, int pe, void *oldval_ptr);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_fetch(shmem_ctx_t ctx
                                                  OSHMPI_ATTRIBUTE((unused)), MPI_Datatype mpi_type,
                                                  OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                  size_t bytes, MPI_Op op,
                                                  OSHMPI_am_mpi_op_index_t op_idx, void *dest,
                                                  void *value_ptr, int pe, void *oldval_ptr);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_post(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                 MPI_Datatype mpi_type,
                                                 OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                 size_t bytes, MPI_Op op,
                                                 OSHMPI_am_mpi_op_index_t op_idx, void *dest,
                                                 void *value_ptr, int pe);

/* Subroutines for am atomics. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_am_cswap(shmem_ctx_t ctx
                                                     OSHMPI_ATTRIBUTE((unused)),
                                                     MPI_Datatype mpi_type,
                                                     OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                     size_t bytes, void *dest, void *cond_ptr,
                                                     void *value_ptr, int pe, void *oldval_ptr);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_am_fetch(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                     MPI_Datatype mpi_type,
                                                     OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                     size_t bytes, MPI_Op op,
                                                     OSHMPI_am_mpi_op_index_t op_idx, void *dest,
                                                     void *value_ptr, int pe, void *oldval_ptr);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_am_post(shmem_ctx_t ctx OSHMPI_ATTRIBUTE((unused)),
                                                    MPI_Datatype mpi_type,
                                                    OSHMPI_am_mpi_datatype_index_t mpi_type_idx,
                                                    size_t bytes, MPI_Op op,
                                                    OSHMPI_am_mpi_op_index_t op_idx, void *dest,
                                                    void *value_ptr, int pe);

OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_am_cswap_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_am_fetch_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_amo_am_post_pkt_cb(int origin_rank, OSHMPI_am_pkt_t * pkt);

/* Wrapper of MPI blocking calls with active message progress. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_send(const void *buf, int count,
                                                             MPI_Datatype datatype, int dest,
                                                             int tag, MPI_Comm comm);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_recv(void *buf, int count,
                                                             MPI_Datatype datatype, int src,
                                                             int tag, MPI_Comm comm,
                                                             MPI_Status * status);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_waitall(int count,
                                                                MPI_Request array_of_requests[],
                                                                MPI_Status array_of_statuses[]);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_barrier(MPI_Comm comm);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_bcast(void *buffer, int count,
                                                              MPI_Datatype datatype, int root,
                                                              MPI_Comm comm);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_allgather(const void *sendbuf,
                                                                  int sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, int recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  MPI_Comm comm);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_allgatherv(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const int *recvcounts,
                                                                   const int *displs,
                                                                   MPI_Datatype recvtype,
                                                                   MPI_Comm comm);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_alltoall(const void *sendbuf,
                                                                 int sendcount,
                                                                 MPI_Datatype sendtype,
                                                                 void *recvbuf, int recvcount,
                                                                 MPI_Datatype recvtype,
                                                                 MPI_Comm comm);
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_am_progress_mpi_allreduce(const void *sendbuf,
                                                                  void *recvbuf, int count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, MPI_Comm comm);

/* Common routines for internal use */
OSHMPI_STATIC_INLINE_PREFIX int OSHMPI_check_gpu_direct_rma(const void *origin_addr,
                                                            shmemx_space_memkind_t sobj_memkind,
                                                            OSHMPI_rma_op_t rma)
{
    int use_gpu = 0;

    if (sobj_memkind == SHMEMX_SPACE_CUDA
        || OSHMPIU_gpu_query_pointer_type(origin_addr) == OSHMPIU_GPU_POINTER_DEV)
        use_gpu = 1;

    if (!use_gpu)
        return 1;       /* always direct for host buffers */

    int direct = 0;
    switch (rma) {
        case OSHMPI_PUT:
            direct = OSHMPI_CHECK_MPI_GPU_FEATURE(OSHMPI_MPI_GPU_PUT);
            break;
        case OSHMPI_GET:
            direct = OSHMPI_CHECK_MPI_GPU_FEATURE(OSHMPI_MPI_GPU_GET);
            break;
        default:
            OSHMPI_ASSERT(rma == OSHMPI_PUT || rma == OSHMPI_GET);
            break;
    }

    if (!direct && !OSHMPI_CHECK_MPI_GPU_FEATURE(OSHMPI_MPI_GPU_PT2PT)) {
        /* abort if GPU is used but MPI supports neither RMA nor P2P */
        OSHMPI_ERR_ABORT("MPI does not support GPU over RMA nor PT2PT."
                         "Set environment variable OSHMPI_MPI_GPU_FEATURES from \"pt2pt,put,get,acc\".\n");
        return 0;
    }

    return direct;
}

OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_translate_ictx_disp(OSHMPI_ctx_t * ctx,
                                                            const void *abs_addr,
                                                            int target_rank,
                                                            MPI_Aint * disp_ptr,
                                                            OSHMPI_ictx_t ** ictx_ptr,
                                                            OSHMPI_sobj_attr_t * sobj_attr_ptr)
{
    MPI_Aint disp;

    if (ctx != SHMEM_CTX_DEFAULT) {
        *disp_ptr = (MPI_Aint) abs_addr - (MPI_Aint) ctx->sobj_attr.base;
        OSHMPI_ASSERT(*disp_ptr < ctx->sobj_attr.size);
        *ictx_ptr = &ctx->ictx;
        if (sobj_attr_ptr)
            *sobj_attr_ptr = ctx->sobj_attr;
        return;
    }

    /* search default contexts */
    *ictx_ptr = NULL;
#ifdef OSHMPI_ENABLE_DYNAMIC_WIN
    *ictx_ptr = &OSHMPI_global.symm_ictx;

    /* FIXME: how to set sobj_handle for AM? */

    /* fast path if both heap and data are symmetric or it is a local buffer
     * - skip heap or data check
     * - skip remote vaddr translation */
    if (OSHMPI_global.symm_base_flag || OSHMPI_global.world_rank == target_rank) {
        OSHMPI_FORCEINLINE()
            OSHMPI_CALLMPI(MPI_Get_address(abs_addr, disp_ptr));
        return;
    }

    disp = (MPI_Aint) abs_addr - (MPI_Aint) OSHMPI_global.symm_heap_base;
    if (disp >= 0 && disp < OSHMPI_global.symm_heap_size) {
        /* heap */
        if (OSHMPI_global.symm_heap_flag) {
            OSHMPI_FORCEINLINE()
                OSHMPI_CALLMPI(MPI_Get_address(abs_addr, disp_ptr));
        } else
            *disp_ptr = disp + OSHMPI_global.symm_heap_bases[target_rank];
        return;
    }

    disp = (MPI_Aint) abs_addr - (MPI_Aint) OSHMPI_global.symm_data_base;
    if (disp >= 0 && disp < OSHMPI_global.symm_data_size) {
        /* text */
        if (OSHMPI_global.symm_data_flag) {
            OSHMPI_FORCEINLINE()
                OSHMPI_CALLMPI(MPI_Get_address(abs_addr, disp_ptr));
        } else
            *disp_ptr = disp + OSHMPI_global.symm_data_bases[target_rank];
        return;
    }
#elif defined(OSHMPI_ENABLE_RMA_ABS)
    *disp_ptr = (MPI_Aint) abs_addr;
    if (*disp_ptr >= (MPI_Aint) OSHMPI_global.symm_heap_base &&
        *disp_ptr < OSHMPI_global.symm_heap_base_end) {
        /* heap */
        *ictx_ptr = &OSHMPI_global.symm_heap_ictx;
        if (sobj_attr_ptr) {
            sobj_attr_ptr->memkind = SHMEMX_SPACE_HOST;
            sobj_attr_ptr->handle = OSHMPI_SOBJ_HANDLE_SYMM_HEAP;
        }
        return;
    }
    if (*disp_ptr >= (MPI_Aint) OSHMPI_global.symm_data_base &&
        *disp_ptr < OSHMPI_global.symm_data_base_end) {
        /* text */
        *ictx_ptr = &OSHMPI_global.symm_data_ictx;
        if (sobj_attr_ptr) {
            sobj_attr_ptr->memkind = SHMEMX_SPACE_HOST;
            sobj_attr_ptr->handle = OSHMPI_SOBJ_HANDLE_SYMM_DATA;
        }
        return;
    }
#else
    disp = (MPI_Aint) abs_addr - (MPI_Aint) OSHMPI_global.symm_heap_base;
    if (disp >= 0 && disp < OSHMPI_global.symm_heap_size) {
        /* heap */
        *disp_ptr = disp;
        *ictx_ptr = &OSHMPI_global.symm_heap_ictx;
        if (sobj_attr_ptr) {
            sobj_attr_ptr->memkind = SHMEMX_SPACE_HOST;
            sobj_attr_ptr->handle = OSHMPI_SOBJ_HANDLE_SYMM_HEAP;
        }
        return;
    }

    disp = (MPI_Aint) abs_addr - (MPI_Aint) OSHMPI_global.symm_data_base;
    if (disp >= 0 && disp < OSHMPI_global.symm_data_size) {
        /* text */
        *disp_ptr = disp;
        *ictx_ptr = &OSHMPI_global.symm_data_ictx;
        if (sobj_attr_ptr) {
            sobj_attr_ptr->memkind = SHMEMX_SPACE_HOST;
            sobj_attr_ptr->handle = OSHMPI_SOBJ_HANDLE_SYMM_DATA;
        }
        return;
    }
#endif

    /* Search spaces */
    OSHMPI_space_t *space, *tmp;
    OSHMPI_THREAD_ENTER_CS(&OSHMPI_global.space_list.cs);
    LL_FOREACH_SAFE(OSHMPI_global.space_list.head, space, tmp) {
        disp = (MPI_Aint) abs_addr - (MPI_Aint) space->sobj_attr.base;
        if (disp >= 0 && disp < space->sobj_attr.size) {
            *disp_ptr = disp;
            *ictx_ptr = &space->default_ictx;
            if (sobj_attr_ptr)
                *sobj_attr_ptr = space->sobj_attr;
            /* TODO: support dynamic window version which attaches space at attach */
            break;
        }
    }
    OSHMPI_THREAD_EXIT_CS(&OSHMPI_global.space_list.cs);
}

OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_translate_disp_to_vaddr(uint32_t sobj_handle,
                                                                MPI_Aint disp, void **vaddr)
{
#ifdef OSHMPI_ENABLE_DYNAMIC_WIN
    *vaddr = (void *) disp;     /* Original computes the remote vaddr */
#else

    OSHMPI_space_t *space, *tmp;

    switch (OSHMPI_SOBJ_HANDLE_GET_KIND(sobj_handle)) {
        case OSHMPI_SOBJ_SYMM_HEAP:
            *vaddr = (void *) ((char *) OSHMPI_global.symm_heap_base + disp);
            break;
        case OSHMPI_SOBJ_SYMM_DATA:
            *vaddr = (void *) ((char *) OSHMPI_global.symm_data_base + disp);
            break;
        case OSHMPI_SOBJ_SPACE_ATTACHED_HEAP:
            /* Search spaces */
            OSHMPI_THREAD_ENTER_CS(&OSHMPI_global.space_list.cs);
            LL_FOREACH_SAFE(OSHMPI_global.space_list.head, space, tmp) {
                if (space->sobj_attr.handle == sobj_handle) {
                    *vaddr = (void *) ((char *) space->sobj_attr.base + disp);
                    break;
                }
            }
            OSHMPI_THREAD_EXIT_CS(&OSHMPI_global.space_list.cs);
            break;
        case OSHMPI_SOBJ_SPACE_HEAP:
        default:
            OSHMPI_ERR_ABORT("Unsupported symmetric object kind:%d, handle 0x%x\n",
                             OSHMPI_SOBJ_HANDLE_GET_KIND(sobj_handle), sobj_handle);
            break;
    }
#endif
}

/* Create derived datatype for strided data format.
 * If it is contig (stride == 1), then the basic datatype is returned.
 * The caller must check the returned datatype to free it when necessary. */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_create_strided_dtype(size_t nelems, ptrdiff_t stride,
                                                             MPI_Datatype mpi_type,
                                                             size_t required_ext_nelems,
                                                             size_t * strided_cnt,
                                                             MPI_Datatype * strided_type)
{
    /* TODO: check non-int inputs exceeds int limit */

    if (stride == 1) {
        *strided_type = mpi_type;
        *strided_cnt = nelems;
    } else {
        MPI_Datatype vtype = MPI_DATATYPE_NULL;
        size_t elem_bytes = 0;

        OSHMPI_CALLMPI(MPI_Type_vector((int) nelems, 1, (int) stride, mpi_type, &vtype));

        /* Vector does not count stride after last chunk, thus we need to resize to
         * cover it when multiple elements with the stride_datatype may be used (i.e., alltoalls).
         * Extent can be negative in MPI, however, we do not expect such case in OSHMPI.
         * Thus skip any negative one */
        if (required_ext_nelems > 0) {
            if (mpi_type == OSHMPI_MPI_COLL32_T)
                elem_bytes = 4;
            else
                elem_bytes = 8;
            OSHMPI_CALLMPI(MPI_Type_create_resized
                           (vtype, 0, required_ext_nelems * elem_bytes, strided_type));
        } else
            *strided_type = vtype;
        OSHMPI_CALLMPI(MPI_Type_commit(strided_type));
        if (required_ext_nelems > 0)
            OSHMPI_CALLMPI(MPI_Type_free(&vtype));
        *strided_cnt = 1;
    }
}

OSHMPI_STATIC_INLINE_PREFIX void ctx_local_complete_impl(int pe, OSHMPI_ictx_t * ictx)
{
    OSHMPI_FORCEINLINE()
        OSHMPI_CALLMPI(MPI_Win_flush_local(pe, ictx->win));
}


/* Workaround: some MPI routines may skip internal progress (e.g., MPICH CH3,
 * fetch_and_op(self) + flush_local(self) in test and wait_until). Thus,
 * we have to manually poll MPI progress at some "risky" MPI calls.  */
OSHMPI_STATIC_INLINE_PREFIX void OSHMPI_progress_poll_mpi(void)
{
    int iprobe_flag = 0;

    /* No need to make manual MPI progress if we are making OSHMPI progress for AM AMO
     * by either main thread or asynchronous thread. */
#ifdef OSHMPI_ENABLE_AM_AMO
    return;
#elif !defined(OSHMPI_ENABLE_DIRECT_AMO)        /* auto */
    if (!OSHMPI_global.amo_direct)
        return;
#endif

    OSHMPI_CALLMPI(MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, OSHMPI_global.comm_world,
                              &iprobe_flag, MPI_STATUS_IGNORE));
}

enum {
    OSHMPI_OP_OUTSTANDING,      /* nonblocking or PUT with local completion */
    OSHMPI_OP_COMPLETED         /* GET with local completion */
};


#ifdef OSHMPI_ENABLE_OP_TRACKING
#define OSHMPI_SET_OUTSTANDING_OP(ctx, completion) do {       \
        if (completion == OSHMPI_OP_COMPLETED) break;         \
        ctx->outstanding_op = 1;                              \
        } while (0)
#else
#define OSHMPI_SET_OUTSTANDING_OP(ctx, completion) do {} while (0)
#endif

OSHMPI_STATIC_INLINE_PREFIX size_t OSHMPI_get_mspace_sz(size_t bufsz)
{
    size_t mspace_sz = 0;

    /* Ensure extra bookkeeping space in MSPACE */
    mspace_sz = bufsz + OSHMPI_DLMALLOC_MIN_MSPACE_SIZE;
    mspace_sz = OSHMPI_ALIGN(bufsz, OSHMPI_global.page_sz);

    return mspace_sz;
}

#include "coll_impl.h"
#include "rma_impl.h"
#include "amo_impl.h"
#include "am_impl.h"
#include "order_impl.h"
#include "p2p_impl.h"
#include "lock_impl.h"
#include "am_progress_impl.h"

#endif /* OSHMPI_IMPL_H */
