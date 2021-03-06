/* Copyright (c) 2017 LiteSpeed Technologies Inc.  See LICENSE. */
/*
 * lsquic_rechist.c -- History of received packets.
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "lsquic_int_types.h"
#include "lsquic_types.h"
#include "lsquic_rechist.h"

#define LSQUIC_LOGGER_MODULE LSQLM_RECHIST
#define LSQUIC_LOG_CONN_ID rechist->rh_cid
#include "lsquic_logger.h"


void
lsquic_rechist_init (struct lsquic_rechist *rechist, lsquic_cid_t cid)
{
    memset(rechist, 0, sizeof(*rechist));
    rechist->rh_cid = cid;
    rechist->rh_cutoff = 1;
    lsquic_packints_init(&rechist->rh_pints);
    LSQ_DEBUG("instantiated received packet history");
}


void
lsquic_rechist_cleanup (lsquic_rechist_t *rechist)
{
    lsquic_packints_cleanup(&rechist->rh_pints);
    memset(rechist, 0, sizeof(*rechist));
}


enum received_st
lsquic_rechist_received (lsquic_rechist_t *rechist, lsquic_packno_t packno,
                         lsquic_time_t now)
{
    const struct lsquic_packno_range *first_range;

    LSQ_DEBUG("received %"PRIu64, packno);
    if (packno < rechist->rh_cutoff)
    {
        if (packno)
            return REC_ST_DUP;
        else
            return REC_ST_ERR;
    }

    first_range = lsquic_packints_first(&rechist->rh_pints);
    if (!first_range || packno > first_range->high)
        rechist->rh_largest_acked_received = now;

    switch (lsquic_packints_add(&rechist->rh_pints, packno))
    {
    case PACKINTS_OK:
        ++rechist->rh_n_packets;
        return REC_ST_OK;
    case PACKINTS_DUP:
        return REC_ST_DUP;
    default:
        assert(0);
    case PACKINTS_ERR:
        return REC_ST_ERR;
    }
}


void
lsquic_rechist_stop_wait (lsquic_rechist_t *rechist, lsquic_packno_t cutoff)
{
    LSQ_INFO("stop wait: %"PRIu64, cutoff);

    if (rechist->rh_flags & RH_CUTOFF_SET)
    {
        assert(cutoff >= rechist->rh_cutoff);  /* Check performed in full_conn */
        if (cutoff == rechist->rh_cutoff)
            return;
    }

    rechist->rh_cutoff = cutoff;
    rechist->rh_flags |= RH_CUTOFF_SET;
    struct packet_interval *pi, *next;
    for (pi = TAILQ_FIRST(&rechist->rh_pints.pk_intervals); pi; pi = next)
    {
        next = TAILQ_NEXT(pi, next_pi);
        if (pi->range.low < cutoff)
        {
            if (pi->range.high < cutoff)
            {
                rechist->rh_n_packets -= pi->range.high - pi->range.low + 1;
                TAILQ_REMOVE(&rechist->rh_pints.pk_intervals, pi, next_pi);
                free(pi);
            }
            else
            {
                rechist->rh_n_packets -= cutoff - pi->range.low;
                pi->range.low = cutoff;
            }
        }
    }
    lsquic_packints_sanity_check(&rechist->rh_pints);
}


lsquic_packno_t
lsquic_rechist_largest_packno (const lsquic_rechist_t *rechist)
{
    const struct packet_interval *pi =
                                TAILQ_FIRST(&rechist->rh_pints.pk_intervals);
    if (pi)
        return pi->range.high;
    else
        return 0;   /* Don't call this function if history is empty */
}


lsquic_packno_t
lsquic_rechist_cutoff (const lsquic_rechist_t *rechist)
{
    if (rechist->rh_flags & RH_CUTOFF_SET)
        return rechist->rh_cutoff;
    else
        return 0;
}


lsquic_time_t
lsquic_rechist_largest_recv (const lsquic_rechist_t *rechist)
{
    return rechist->rh_largest_acked_received;
}


const struct lsquic_packno_range *
lsquic_rechist_first (lsquic_rechist_t *rechist)
{
    return lsquic_packints_first(&rechist->rh_pints);
}


const struct lsquic_packno_range *
lsquic_rechist_next (lsquic_rechist_t *rechist)
{
    return lsquic_packints_next(&rechist->rh_pints);
}
