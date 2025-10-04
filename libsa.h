#ifndef _LIBSA_H_
#define _LIBSA_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Store in result the indices of all suffixes of input sorted in ascending order.
   libsa_build runs in linear time and occupies linear space.
   It is caller's responsibility to allocate result of the same size as input.
   Return 0.  */
int libsa_build (int *result, const char *input, size_t len);

#ifdef __cplusplus
}
#endif
#endif

/* Copyright (c) 2025 Dmitry Goncharov
 * dgoncharov@users.sf.net.
 *
 * Distributed under GPL v2 or the BSD License (see accompanying file copying),
 * your choice.
 */
