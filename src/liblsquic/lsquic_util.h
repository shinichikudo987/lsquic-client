/* Copyright (c) 2017 LiteSpeed Technologies Inc.  See LICENSE. */
/*
 * lsquic_util.h -- Utility functions
 */

#ifndef LSQUIC_UTIL_H
#define LSQUIC_UTIL_H 1

#ifdef __cplusplus
extern "C" {
#endif

lsquic_time_t
lsquic_time_now (void);

/* Returns 1 if `buf' contains only zero bytes, 0 otherwise.
 */
int
lsquic_is_zero (const void *buf, size_t bufsz);



char * get_bin_str(const void *s, size_t len, size_t max_display_len);


/* `out_sz' is assumed to be at least 1.  `out' is always NUL-terminated. */
size_t
hexdump (const void *src, size_t src_sz, char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif


#endif
