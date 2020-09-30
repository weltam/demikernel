// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <dmtr/sga.h>

#include <dmtr/annot.h>
#include <dmtr/fail.h>
#include <dmtr/types.h>
#include <errno.h>
#include <stdio.h>
//#define DMTR_ALLOCATE_SEGMENTS
int dmtr_sgalen(size_t *len_out, const dmtr_sgarray_t *sga) {
    DMTR_NOTNULL(EINVAL, len_out);
    *len_out = 0;
    DMTR_NOTNULL(EINVAL, sga);

    size_t len = 0;
    for (size_t i = 0; i < sga->sga_numsegs; ++i) {
        len += sga->sga_segs[i].sgaseg_len;
    }

    *len_out = len;
    return 0;
}

int dmtr_sgafree(dmtr_sgarray_t *sga) {
    // we haven't got a good solution for communicating how to free
    // scatter/gather arrays.
    //printf("Addr of sga: %p, segments: %p\n", (void *)sga, (void *)sga->sga_segs);

    if (NULL == sga) {
        return 0;
    }

    if (NULL != sga->dpdk_pkt) {
        // the one sga here has dpdk data
        return 0;
    }

    if (NULL != sga->segments) {
        // this one has a linked list of rte_mbuf** that needs to be freed
    }


    if (NULL == sga->sga_buf) {
        //printf("num segs: %d\n", sga->sga_numsegs);
        for (size_t i = 0; i < sga->sga_numsegs; ++i) {
            //printf("freeing a scatter-gather array: %p\n",sga->sga_segs[i].sgaseg_buf);
            free(sga->sga_segs[i].sgaseg_buf);
        }
    } else {
        //printf("freeing a scatter-gather array stored in sga_buf:%p\n", sga->sga_buf);
        free(sga->sga_buf);
    }

    return 0;
}

// free a particular segment within an SGA
int dmtr_sgafree_seg(dmtr_sgarray_t *sga, int seg) {
    if (NULL == sga) {
        return 0;
    }

    if (NULL == sga->sga_buf) {
        free(sga->sga_segs[seg].sgaseg_buf);
    }


    return 0;
}

// free the allocated segments
int dmtr_sgafree_segments(dmtr_sgarray_t *sga) {
#ifdef DMTR_ALLOCATE_SEGMENTS
    //printf("Trying to free segments allocated to: %p\n", (void *)sga->sga_segs);
    free((void *)(sga->sga_segs));
#endif
    return 0;
}
