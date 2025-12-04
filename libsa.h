#ifndef _LIBSA_H_
#define _LIBSA_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Store in result the indices of all suffixes of input sorted in ascending order.
   libsa_build runs in linear time and occupies linear space.
   It is caller's responsibility to allocate result of the same size as input.
   input does not have to be null terminated, but input[len - 1] has to be
   smaller than any element of input between (and including) 0 and len - 2.
   Return 0.  */
int libsa_build (int *result, const char *input, size_t len);

/* Store in result the lengths of the longest common prefixes of the pairs of
   adjacent suffixes of the specified sa.
   libsa_build_lcp runs in linear time and occupies linear space.
   It is caller's responsibility to allocate result of the same size as input.
   Return 0.  */
int libsa_build_lcp (int *result, int *sa, const char *input, size_t len);

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
