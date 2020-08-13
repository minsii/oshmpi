/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2018 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */
#ifndef INTERNAL_AM_PKT_DEF_H
#define INTERNAL_AM_PKT_DEF_H

/* Define MPI datatype indexes. Used when transfer datatype in AM packets.
 * Instead of using MPI datatypes directly, integer indexes can be safely
 * handled by switch structure.*/
typedef enum {
    OSHMPI_AM_MPI_CHAR,
    OSHMPI_AM_MPI_SIGNED_CHAR,
    OSHMPI_AM_MPI_SHORT,
    OSHMPI_AM_MPI_INT,
    OSHMPI_AM_MPI_LONG,
    OSHMPI_AM_MPI_LONG_LONG,
    OSHMPI_AM_MPI_UNSIGNED_CHAR,
    OSHMPI_AM_MPI_UNSIGNED_SHORT,
    OSHMPI_AM_MPI_UNSIGNED,
    OSHMPI_AM_MPI_UNSIGNED_LONG,
    OSHMPI_AM_MPI_UNSIGNED_LONG_LONG,
    OSHMPI_AM_MPI_INT8_T,
    OSHMPI_AM_MPI_INT16_T,
    OSHMPI_AM_MPI_INT32_T,
    OSHMPI_AM_MPI_INT64_T,
    OSHMPI_AM_MPI_UINT8_T,
    OSHMPI_AM_MPI_UINT16_T,
    OSHMPI_AM_MPI_UINT32_T,
    OSHMPI_AM_MPI_UINT64_T,
    OSHMPI_AM_OSHMPI_MPI_SIZE_T,
    OSHMPI_AM_OSHMPI_MPI_PTRDIFF_T,
    OSHMPI_AM_MPI_FLOAT,
    OSHMPI_AM_MPI_DOUBLE,
    OSHMPI_AM_MPI_LONG_DOUBLE,
    OSHMPI_AM_MPI_C_DOUBLE_COMPLEX,
    OSHMPI_AM_MPI_DATATYPE_MAX,
} OSHMPI_am_mpi_datatype_index_t;

typedef enum {
    OSHMPI_AM_MPI_BAND,
    OSHMPI_AM_MPI_BOR,
    OSHMPI_AM_MPI_BXOR,
    OSHMPI_AM_MPI_NO_OP,
    OSHMPI_AM_MPI_REPLACE,
    OSHMPI_AM_MPI_SUM,
    OSHMPI_AM_MPI_OP_MAX,
} OSHMPI_am_mpi_op_index_t;

/* Ensure packet header variables can fit all possible types.
 * Used only for AMO packets which embed variable in header. */
typedef union {
    int i_v;
    long l_v;
    long long ll_v;
    unsigned int ui_v;
    unsigned long ul_v;
    unsigned long long ull_v;
    int32_t i32_v;
    int64_t i64_v;
    uint32_t ui32_v;
    uint64_t ui64_v;
    size_t sz_v;
    ptrdiff_t ptr_v;
    float f_v;
    double d_v;
} OSHMPI_am_datatype_t;

/* Define AMO active message packet header. Note that we do not define
 * packet for ACK of each packet, because all routines that require ACK
 * are blocking, thus we can directly receive ACK without going through
 * the callback progress handling. */
typedef enum {
    OSHMPI_AM_PKT_CSWAP,
    OSHMPI_AM_PKT_FETCH,
    OSHMPI_AM_PKT_POST,
    OSHMPI_AM_PKT_PUT,
    OSHMPI_AM_PKT_GET,
    OSHMPI_AM_PKT_IPUT,
    OSHMPI_AM_PKT_IGET,
    OSHMPI_AM_PKT_FLUSH,
    OSHMPI_AM_PKT_TERMINATE,
    OSHMPI_AM_PKT_MAX,
} OSHMPI_am_pkt_type_t;

typedef struct OSHMPI_am_cswap_pkt {
    OSHMPI_am_datatype_t cond;
    OSHMPI_am_datatype_t value;
    uint32_t sobj_handle;
    OSHMPI_am_mpi_datatype_index_t mpi_type_idx;
    MPI_Aint target_disp;
    size_t bytes;
} OSHMPI_am_cswap_pkt_t;

typedef struct OSHMPI_am_fetch_pkt {
    OSHMPI_am_datatype_t value;
    uint32_t sobj_handle;
    OSHMPI_am_mpi_datatype_index_t mpi_type_idx;
    OSHMPI_am_mpi_op_index_t mpi_op_idx;
    MPI_Aint target_disp;
    size_t bytes;
} OSHMPI_am_fetch_pkt_t;

typedef OSHMPI_am_fetch_pkt_t OSHMPI_am_post_pkt_t;

typedef struct OSHMPI_am_get_pkt {
    uint32_t sobj_handle;
    MPI_Aint target_disp;
    size_t bytes;
} OSHMPI_am_get_pkt_t;

typedef OSHMPI_am_get_pkt_t OSHMPI_am_put_pkt_t;

typedef struct OSHMPI_am_iget_pkt {
    OSHMPI_am_mpi_datatype_index_t mpi_type_idx;
    ptrdiff_t target_st;
    size_t nelems;
    uint32_t sobj_handle;
    MPI_Aint target_disp;
} OSHMPI_am_iget_pkt_t;

typedef OSHMPI_am_iget_pkt_t OSHMPI_am_iput_pkt_t;

typedef struct {
} OSHMPI_am_flush_pkt_t;

typedef struct OSHMPI_am_pkt {
    int type;
    union {
        OSHMPI_am_cswap_pkt_t cswap;
        OSHMPI_am_fetch_pkt_t fetch;
        OSHMPI_am_post_pkt_t post;
        OSHMPI_am_put_pkt_t put;
        OSHMPI_am_get_pkt_t get;
        OSHMPI_am_iput_pkt_t iput;
        OSHMPI_am_iget_pkt_t iget;
        OSHMPI_am_flush_pkt_t flush;
    };
} OSHMPI_am_pkt_t;

#define OSHMPI_AM_PKT_TAG 2000
#define OSHMPI_AM_PKT_DATA_TAG 2003
#define OSHMPI_AM_TERMINATE_TAG 2001
#define OSHMPI_AM_PKT_ACK_TAG 2002

#endif /* INTERNAL_AM_PKT_DEF_H */
