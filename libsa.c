#include "libsa.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

/* This implementation uses term "lms block" to mean what the paper calls "lms
   substring".  Because word "block" is shorter than "substring".  */

/* nrecursion is a only used to print the depth of recursion for debugging
   purposes.  */
static int nrecursion;
static int verbose;

/* Forward all arguments to printf.
   The only reason this function is used instead of printf is its ability to
   avoid printing anything when logging is disabled.  */
static void
print (const char *f, ...)
{
    va_list ap;
    if (!verbose)
      return;
    va_start (ap, f);
    vprintf (f, ap);
    va_end (ap);
}

/* malloc and assert that malloc succeeded.  */
static void*
alloc (size_t size)
{
    void *r = malloc (size);
    assert (size == 0 || r);
    return r;
}

/* Allocate a copy of 'b' of 'len' elements.  */
static int*
alloc_copy (const int *b, size_t len)
{
    int *r = alloc (len * sizeof *r);
    return memcpy (r, b, len * sizeof *r);
}

/* Allocate an array of int of 'len' elements and memset it with 'value'.  */
static int*
alloc_init (int value, size_t len)
{
    int *r = alloc (len * sizeof *r);
    return memset (r, value, len * sizeof *r);
}

/* Print len elements of input either as character or integers.  */
static void
print_array (const int *input, size_t len, int ascii, int depth)
{
    size_t k;

    if (!verbose)
      return;

    print ("%*s", depth, "");
    for (k = 0; k < len; ++k)
      if (ascii && input[k] > 31 && input[k] < 127)
        print (" %c ", (char) input[k]);
      else
        print (" %d ", input[k]);
    print ("\n");
}

/* Pretty print a table that contains input, type, lms, suffix array and buckets
   along with the index of each element.  */
static void
print_sa (const int *result, const int *input, const int *type, const int *buckets,
       size_t len, size_t abclen, int depth)
{
    size_t k;
    int *b;
    const int ascii = depth == 0;

    if (!verbose)
      return;

    b = alloc_copy (buckets, abclen);
    print ("\n%*sindex  ", depth, "");
    for (k = 0; k < len; ++k)
      print ("%2lu ", k);
    print ("\n%*sinput  ", depth, "");
    print_array (input, len, depth == 0, 0);
    print ("%*stype   ", depth, "");
    for (k = 0; k < len; ++k)
      if (type[k])
        print (" L ");
      else
        print (" S ");
    print ("\n%*slms       ", depth, "");
    for (k = 1; k < len; ++k)
      if (!type[k] && type[k-1])
        print (" ^ ");
      else
        print ("   ");
    print ("\n%*ssufar  ", depth, "");
    for (k = 0; k < len; ++k)
      print ("%2d ", result[k]);


    print ("\n%*sbucke | 0|", depth, "");
    for (k = 1; k < len; ++k)
      {
        /* buckets[x-1] is the beginning of the bucket for character x.
           buckets[x]-1 is the end of the bucket for character x.  */
        const int pos = result[k];
        if (pos < 0)
          continue;
        const int c = input[pos];
        const int beg = b[c-1];
        const int end = b[c] - 1;
        if (beg < 0)
          continue;
        b[c-1] = -1;
        if (ascii && c > 31 && c < 127)
          print ("%2c%*s|", (unsigned char) c, 3*(end - beg), "");
        else
          print ("%2d%*s|", c, 3*(end - beg), "");
      }
    print ("\n\n");
    free (b);
}

/* Check that all initialized elements of result are unique.
   Return 1 on success. Return 0 on failure.  */
static int
unique (const int *result, size_t len)
{
    size_t k;
    int *seen;

    seen = alloc_init (-1, len);
    for (k = 0; k < len; ++k)
      if (result[k] >= 0)
        {
          assert (seen[result[k]] < 0);
          seen[result[k]] = result[k];
        }
    free (seen);
    return 1;
}

/* Check that all elements of result are initialized and unique.
   Return 1 on success. Return 0 on failure.  */
static int
all_unique (const int *result, size_t rlen, size_t len)
{
    size_t k;
    int *seen;

    seen = alloc_init (-1, len);
    for (k = 0; k < rlen; ++k)
      {
        assert (result[k] >= 0);
        assert (seen[result[k]] < 0);
        seen[result[k]] = result[k];
      }
    free (seen);
    return 1;
}

/* Compare two zero terminated arrays of integers.
   Return -1 if array x < array y.
   Return 1 if array x > array y.
   Return 0 if array x == array y.  */
static int
intcmp (const int *x, const int *y)
{
    for (; *x && *y; ++x, ++y)
      {
        if (*x > *y)
          return 1;
        if (*x < *y)
          return -1;
      }
    if (*x)
      return 1;
    if (*y)
      return -1;
    return 0;
}

/* Check that the initialized elements of result are sorted.
   Return 1 on success. Return 0 on failure.  */
static int
sorted (const int *result, const int *input, size_t len, int depth)
{
    size_t k;
    (void) depth;

    for (k = 1; k < len; ++k)
      {
        int pos, pos1;
        while (k < len && result[k-1] < 0)
          ++k;
        pos = result[k-1];
        if (k >= len)
          break;
        assert (pos >= 0);
        while (k < len && result[k] < 0)
          ++k;
        if (k >= len)
          break;
        pos1 = result[k];
        assert (pos1 >= 0);
        if (intcmp (input + pos, input + pos1) >= 0)
          {
            assert (0);
            return 0;
          }
      }
    return 1;
}

/* Check that all elements of result are initialized and sorted.
   Return 1 on success. Return 0 on failure.  */
static int
all_sorted (const int *result, const int *input, size_t len, int depth)
{
    size_t k;
    (void) depth;

    for (k = 1; k < len; ++k)
      {
        const int pos = result[k-1], pos1 = result[k];
        assert (pos >= 0);
        assert (pos1 >= 0);
        if (intcmp (input + pos, input + pos1) >= 0)
          {
            assert (0);
            return 0;
          }
      }
    return 1;
}

/* Return 1 if the last element of the input is the smallest.
   Return 0 otherwise.
   Suffix array requires that the last element is the smallest. This ensures
   that no suffix is a prefix of another suffix. Which in turn ensures that
   every suffix has its index in the suffix array.  */
static int
last_smallest (const unsigned char *input, size_t len)
{
    size_t k;

    for (k = 0; k < len - 2; ++k)
      {
        assert (input[len-1] < input[k]);
        if (input[len-1] >= input[k])
          return 0;
      }
    return 1;
}

/* Compare the lms block starting at position 'x' with the lms block at
   position 'y'. The lms blocks in 'input' are supposed to be sorted, even
   though equal lms blocks may still need to be swapped. The lms block at
   position 'x' <= the lms block at 'y'.
   Return 0 if the lengths of the blocks match and values and types match
   character for character for all characters.
   Return 1 otherwise.  */
static int
lms_blocks_differ (const int *input, const int *type, size_t len, int x, int y)
{
    if (x < 0)
      return 1;
    assert (x > 0);
    assert ((size_t) x < len);
    assert (y > 0);
    assert ((size_t) y < len);

    /* An infinite loop here is correct.
       If the blocks differ, then one of the returns below in the loop returns 1.
       If the blocks are equal, then 0 is returned from the loop below.
       If one of the blocks is the last lms of the null terminator, then the
       blocks differ, because the last lms block is unique.  */
    for (;;)
      if (input[x] != input[y])
        /* Values differ.  */
        return 1;
      else if (type[x] != type[y])
        /* Types differ.  */
        return 1;
      else if (type[x] && !type[x+1] && type[y] && !type[y+1])
        /* The blocks are of the same length.
           y+1 and x+1 are the ends of the respective blocks.  */
        return input[x+1] != input[y+1];
      else
        ++y, ++x, assert ((size_t) y < len), assert ((size_t) x < len);
}

/* Give each lms block a name.
   Store these names in lmsnames.
   Give the same name to the lms blocks equal according to lms_blocks_differ.
   When reduce is called lms blocks are supposed to be sorted in result.
   Return the size of the reduced alphabet.  */
static size_t
reduce (int *lmsnames, const int *result, const int *input, const int *type,
        size_t len, size_t lmslen, int depth)
{
    size_t k, j;
    size_t abclen = 0;
    int prior = -1;
    int *name;

    print ("%*sreducing ", depth, "");
    print_array (input, len, depth == 0, 0);
    name = alloc_init (-1, len);
    name[len - 1] = abclen;
    for (k = 1; k < len; ++k)
      {
        int pos;
        pos = result[k];
        if (pos > 0 && type[pos-1] && !type[pos])
          {
            /* pos is an lms position.  */
            abclen += lms_blocks_differ (input, type, len, prior, pos);
            name[pos] = abclen;
            prior = pos;
          }
      }

    for (k = 0, j = 0; k < len; ++k)
      if (name[k] >= 0)
        lmsnames[j++] = name[k];
    assert (j == lmslen);

    free (name);
    ++abclen;
    print ("%*sreduced abclen = %zu, lmslen = %zu\n", depth, "", abclen,
           lmslen);
    print ("%*sreduced lms names ", depth, "");
    print_array (lmsnames, lmslen, 0, 0);
    return abclen;
}

/* Insert the indices of lms positions.  */
static void
insert_lms (int *result, const int *input, const int *buckets, int *lms,
            size_t lmslen, size_t abclen, int depth)
{
    size_t k;
    int *b;

    print ("%*sinserting lms positions\n", depth, "");
    b = alloc_copy (buckets, abclen);
    for (k = lmslen; k > 0; --k)
      {
        int pos; /* Position in result.  */
        int inidx; /* Index in the input string.  */
        unsigned int c;

        inidx = lms[k-1];
        c = input[inidx];
        --b[c];
        pos = b[c];
        result[pos] = inidx;
      }
    free (b);
}

/* Induce the indices of L type positions from lms positions.  */
static void
induce_l (int *result, const int *input, const int *type, const int *buckets,
          size_t len, size_t abclen, int depth)
{
    size_t k;
    int *b;

    print ("%*sinducing L positions from lms pos\n", depth, "");
    b = alloc_copy (buckets, abclen);
    /* Induce L positions from lms positions.
       Scan from left to right.
       If pos is L type, then put pos to the beginning of the bucket.  */
    for (k = 0; k < len; ++k)
      {
        unsigned int c; /* L type character that this iteration is inserting.  */
        int bidx; /* The bucket index of the character that this iteration is inserting.  */
        int pos = result[k]; /* Position in the suffix array.  */
        if (pos <= 0)
          continue;
        --pos;
        if (!type[pos])
          /* S character.  */
          continue;
        c = input[pos];
        assert (c > 0);
        bidx = b[c-1];
        ++b[c-1]; /* Advance bucket head.  */
        result[bidx] = pos;
      }
    free (b);
    assert (unique (result, len));
}

/* Induce the indices of S type positions from the L type positions.  */
static void
induce_s (int *result, const int *input, const int *type, const int *buckets,
          size_t len, size_t abclen, int depth)
{
    int k;
    int *b;

    print ("%*sinducing S positions from L positions\n", depth, "");
    b = alloc_copy (buckets, abclen);
    /* Induce S positions from L positions.
       Scan from right to left.
       If pos is S type, then put pos to the back of the bucket.  */
    for (k = len - 1; k >= 0; --k)
      {
        unsigned int c; /* S type character that this iteration is inserting.  */
        int bidx; /* The bucket index of the character that this iteration is inserting.  */
        int pos = result[k]; /* Position in the suffix array.  */
        if (pos <= 0)
          continue;
        --pos;
        if (type[pos])
          /* L character.  */
          continue;
        c = input[pos];
        assert (c > 0);
        bidx = b[c] - 1;
        --b[c]; /* Retreat bucket tail.  */
        /* This overwrites the lms characters inserted earlier.  */
        result[bidx] = pos;
      }
    free (b);
    assert (unique (result, len));
}

/* The top level function of the sais algorithm.
   See "Linear Suffix Array Construction by Almost Pure Induced-Sorting"
   by Ge Nong at al for the description of this algorithm.  */
static int
build (int *result, const int *input, size_t len, size_t abclen, int depth)
{
    int *lms, *lmsbuf;
    /* lmslen contains the number of elements in lms array.
       redabclen is the alphabet size of the reduced input.  */
    size_t lmslen, redabclen;
    int *type;
    int *buckets;
    size_t k;

    ++nrecursion;

    print ("%*sdepth = %d, len = %zu, abclen = %zu\n", depth, "", depth, len, abclen);
    print ("%*sinput ", depth, "");
    print_array (input, len, depth == 0, 0);

    /* Init type, buckets and lmslen.  */
    buckets = alloc_init (0, abclen);
    lmslen = 0;
    type = alloc_init (0, len);
    /* We'll use 0 for S and 1 for L types.  */
    for (k = len - 1; k > 0; --k)
      {
        ++buckets[input[k]];
        if (input[k-1] > input[k])
          {
            type[k-1] = 1;
            if (!type[k])
              /* input[0] can be an S type character.
                 input[0] cannot be an lms character, by definition of lms. */
              ++lmslen;
          }
        else if (input[k-1] == input[k])
          /* This assignment requires a right to left walk.  */
          type[k-1] = type[k];
      }
    ++buckets[input[0]];
    /* buckets has one element for each character in the alphabet.
       buckets[x] is the number of characters in the input string that are <= x.
       buckets[x-1] is the beginning of the bucket for character x.
       buckets[x]-1 is the end of the bucket for character x.
       buckets[x] is one past the end of the bucket for character x.
       buckets[x] is the beginning of the bucket for character x+1.  */
    for (k = 1; k < abclen; ++k)
      buckets[k] += buckets[k-1];

    /* Init lms.  */
    lms = alloc (lmslen * sizeof *lms);
    for (k = 0, lmslen = 0; k < len - 1; ++k)
      if (type[k] > type[k+1])
        lms[lmslen++] = k + 1;
    print ("%*slmslen = %zu, lms positions", depth, "", lmslen);
    print_array (lms, lmslen, 0, 0);
    assert (all_unique (lms, lmslen, len));

    /* Write indices of all lms characters to their respective buckets.  */
    memset (result, -1, len * sizeof *result);
    insert_lms (result, input, buckets, lms, lmslen, abclen, depth);
    assert (unique (result, len));
    induce_l (result, input, type, buckets, len, abclen, depth);
    induce_s (result, input, type, buckets, len, abclen, depth);
    /* At this point lms blocks are sorted in result.
       However, equal lms blocks may still need to be swapped.  */

    lmsbuf = alloc (lmslen * sizeof *lmsbuf);
    redabclen = reduce (lmsbuf, result, input, type, len, lmslen, depth);
    /* lmsbuf contains lms names.  */
    if (redabclen == lmslen)
      {
        print ("%*seach lms block is unique, inducing L and S positions\n",
               depth, "");
      }
    else
      {
        /* There are equal lms blocks. */
        int *sa_of_lmsnames;

        sa_of_lmsnames = alloc (lmslen * sizeof *sa_of_lmsnames);
        print ("%*sfound equal lms blocks, building sa of lms names recursively\n",
               depth, "");
        build (sa_of_lmsnames, lmsbuf, lmslen, redabclen, depth + 3);
        print ("%*ssa of lms names ", depth, "");
        print_array (sa_of_lmsnames, lmslen, 0, 0);

        /* Use sa_of_lmsnames to sort lms blocks in result.  */
        print ("%*susing sa of lms names to sort lms positions\n", depth, "");
        /* lms names are no longer needed.  Reuse lmsbuf to keep the indices of
           lms positions sorted by the order in sa_of_lmsnames.  */
        for (k = 0; k < lmslen; ++k)
          {
            const int pos = sa_of_lmsnames[k];
            const int idx = lms[pos];
            lmsbuf[k] = idx;
          }
        print ("%*ssorted lms positions ", depth, "");
        print_array (lmsbuf, lmslen, 0, 0);
        assert (all_unique (lmsbuf, lmslen, len));
        assert (all_sorted (lmsbuf, input, lmslen, depth));

        /* It is important to init result again to avoid different elements of
           result having the same value.  */
        memset (result, -1, len * sizeof *result);
        insert_lms (result, input, buckets, lmsbuf, lmslen, abclen, depth);
        assert (unique (result, len));
        assert (sorted (result, input, len, depth));
        free (sa_of_lmsnames);
      }

    /* At this point all (even equal) lms blocks in result are sorted.
       Induce L and S positions from sorted lms blocks.  */
    induce_l (result, input, type, buckets, len, abclen, depth);
    induce_s (result, input, type, buckets, len, abclen, depth);
    print_sa (result, input, type, buckets, len, abclen, depth);
    assert (all_unique (result, len, len));
    assert (all_sorted (result, input, len, depth));
    print ("\n");

    free (lmsbuf);
    free (buckets);
    free (lms);
    free (type);
    return 0;
}

int
libsa_build (int *result, const char *input, size_t len)
{
    size_t k;
    int *copy, abclen = 0;

    verbose = getenv ("LIBSA_LOG") != 0;

    if (len < 2)
      return *result = 0;

    assert (last_smallest((const unsigned char*) input, len));

    nrecursion = 0;
    copy = alloc (len * sizeof *copy);
    for (k = 0; k < len; ++k)
      {
        copy[k] = (unsigned char) input[k];
        if (copy[k] >= abclen)
          abclen = copy[k] + 1;
      }
    build (result, copy, len, abclen, 0);
    if (verbose)
      printf ("recursion depth = %d\n", nrecursion - 1);
    free (copy);
    return 0;
}


/* The top level function of the phi algorithm.
   See "Permuted Longest-Common-Prefix Array"
   by Juha Karkkainen at al for the description of this algorithm.  */
int
libsa_build_lcp (int *result, int *sa, const char *input, size_t len)
{
    int *phi, *plcp;
    size_t k;
    int l;

    if (len < 2)
      /* Need atleast 2 suffixes to have a common prefix.  */
      return 0;

    /* Build phi.  */
    phi = alloc ((len - 1) * sizeof *phi);
    for (k = 1; k < len; ++k)
      phi[sa[k]] = sa[k-1];

    /* Build plcp from phi.  */
    plcp = alloc ((len - 1) * sizeof *plcp);
    for (k = 0, l = 0; k < len - 1; ++k)
      {
        int j = phi[k];
        while (input[k+l] == input[j+l])
          ++l;
        assert (l >= 0);
        plcp[k] = l;
        l = l ? l - 1 : 0;
      }

    /* Build lcp from plcp.  */
    for (k = 1; k < len; ++k)
      result[k] = plcp[sa[k]];

    print ("lcp    ");
    print_array (result + 1, len - 1, 0, 0);

    free (plcp);
    free (phi);
    return 0;
}

/* Copyright (c) 2025 Dmitry Goncharov
 * dgoncharov@users.sf.net.
 *
 * Distributed under GPL v2 or the BSD License (see accompanying file copying),
 * your choice.
 */
