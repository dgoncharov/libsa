#include "libsa.h"
#include "ctest.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

static int verbose;

static void*
alloc (size_t size)
{
    void *r = malloc (size);
    assert (size == 0 || r);
    return r;
}

static char *
random_string (char *input, size_t len, int min, int max)
{
    size_t k;
    int seed;

    seed = time (0);
    if (verbose)
      printf ("random seed = %d\n", seed);
    srand (seed);

    /* Only printable ascii chars.  */
    memset (input, (unsigned char) min, len);
    for (k = 0; k < len; ++k)
      input[k] += rand () % (max - min);
    input[len-1] = '\0';
    return input;
}

static void
testimp (const char *input, int lineno)
{
    int rc;
    size_t k, len = strlen (input);
    int *sa;

    sa = alloc ((len + 1) * sizeof *sa);
    memset (sa, -1, (len + 1) * sizeof *sa);
    libsa_build (sa, input, len + 1);

    ASSERT ((size_t) sa[0] == len, "sa[0] = %d, len = %zu, input = '%s'\n", sa[0], len, input);
    for (k = 0; k < len; ++k)
      {
        int x = sa[k];
        int y = sa[k+1];
        ASSERT (x >= 0, "x = %d, k = %zu\n", x, k);
        ASSERT ((size_t) x <= len, "x = %d, k = %zu, len = %zu\n", x, k, len);
        ASSERT (y >= 0, "y = %d, k = %zu\n", y, k);
        ASSERT ((size_t) y < len, "y = %d, k = %zu, len = %zu\n", y, k, len);
        ASSERT (x != y, "x = %d, y = %d\n", x, y);
        /* This strcmp causes the test to run in quadratic time.  */
        rc = strcmp (input + x, input + y);
        ASSERT (rc < 0, "rc = %d, len = %zu, k = %zu, lineno = %d, input + x = '%s', input + y = '%s'\n", rc, len, k, lineno, input+x, input+y);
      }

    if (verbose)
      {
        printf ("input \"%s\"\nresult\n", input);
        for (k = 0; k <= len; ++k)
          printf ("\"%s\"\n", input + sa[k]);
      }
    free (sa);
}

static
int run_test (long test, int argc, char *argv[])
{
    int retcode;

    (void) argc;
    (void) argv;

    printf ("test %ld\n", test);
    retcode = 0;

    switch (test)
      {
        case 0:
        case 1:
          {
            const char input[] = "hello";
            int sa[sizeof input];

            memset (sa, -1, sizeof sa);
            libsa_build (sa, input, sizeof input);
            ASSERT (sa[0] == 5, "sa[0] = %d\n", sa[0]);
            ASSERT (sa[1] == 1, "sa[1] = %d\n", sa[1]);
            ASSERT (sa[2] == 0, "sa[2] = %d\n", sa[2]);
            ASSERT (sa[3] == 2, "sa[3] = %d\n", sa[3]);
            ASSERT (sa[4] == 3, "sa[4] = %d\n", sa[4]);
            ASSERT (sa[5] == 4, "sa[5] = %d\n", sa[5]);
            testimp (input, __LINE__);
            break;
          }
        case 2:
          testimp ("", __LINE__);
          break;
        case 3:
          testimp ("a", __LINE__);
          break;
        case 4:
          testimp ("aa", __LINE__);
          break;
        case 5:
          testimp ("aaa", __LINE__);
          break;
        case 6:
          testimp ("aaaa", __LINE__);
          break;
        case 7:
          testimp ("abababab", __LINE__);
          break;
        case 8:
          testimp ("dabracadabrac", __LINE__);
          break;
        case 9:
          {
            /* This input causes multiple recursions.  */
            const char input[] =
                "dabracadabracdabracadabracdabracadabracdabracadabracdabrac"
                "adabracdabracadabracdabracadabracdabracadabrac";
            testimp (input, __LINE__);
            break;
          }
        case 97:
          {
            enum {len = 74391};
            char input[len];
            random_string (input, len, 32, 127);
            testimp (input, __LINE__);
            break;
          }
        case 98:
          {
            enum {len = 398421};
            char input[len];
            random_string (input, len, 0, 255);
            testimp (input, __LINE__);
            break;
          }
        case 99:
          {
            char *input, *r;
            int len, min = 32, max = 127;
            const char help[] = "usage: %s 99 <length> [min=32] [max=127]\n";

            if (argc < 3)
              {
                fprintf (stderr, help, argv[0]);
                return 1;
              }
            errno = 0;
            len = strtol (argv[2], &r, 0);
            if (errno || r == argv[2])
              {
                fprintf (stderr, help, argv[0]);
                return 1;
              }
            if (argc > 3)
              {
                errno = 0;
                min = strtol (argv[3], &r, 0);
                if (errno || r == argv[3])
                  {
                    fprintf (stderr, help, argv[0]);
                    return 1;
                  }
              }
            if (argc > 4)
              {
                errno = 0;
                max = strtol (argv[4], &r, 0);
                if (errno || r == argv[4])
                  {
                    fprintf (stderr, help, argv[0]);
                    return 1;
                  }
                if (max <= min)
                  {
                    fprintf (stderr, "max has to be > min\n");
                    return 1;
                  }
              }
            if (verbose)
              printf ("len = %d, min = %d, max = %d\n", len, min, max);
            input = alloc (len * sizeof *input);
            random_string (input, len, min, max);
            testimp (input, __LINE__);
            free (input);
            break;
          }
        default:
          retcode = -1;
          break;
      }
    return retcode;
}

int main (int argc, char *argv[])
{
    int k;

    verbose = getenv ("LIBSA_LOG") != 0;
    if (argc >= 2) {
      /* Run the specified test.  */
      char *r;
      long test;

      errno = 0;
      test = strtol (argv[1], &r, 0);
      if (errno || r == argv[1])
        {
          fprintf (stderr, "usage: %s [test] [test arg]...\n", argv[0]);
          return 1;
        }

      run_test (test, argc, argv);
      if (test_status > 0)
        fprintf (stderr, "%d tests failed\n", test_status);
      return test_status;
    }
    /* Run all tests.  */
    for (k = 0; run_test (k, argc, argv) != -1; ++k)
      ;
    if (test_status > 0)
      fprintf (stderr, "%d tests failed\n", test_status);
    return test_status;
}

/* Copyright (c) 2025 Dmitry Goncharov
 * dgoncharov@users.sf.net.
 *
 * Distributed under GPL v2 or the BSD License (see accompanying file copying),
 * your choice.
 */
