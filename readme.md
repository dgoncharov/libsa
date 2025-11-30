### libsa is a library of suffix array facilities.

### How to build.

```
$ mkdir l64
$ cd l64
$ make -f ../makefile
$ make -f ../makefile install
```

### Examples

```
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
```

Copyright (c) 2025 Dmitry Goncharov
dgoncharov@users.sf.net.

Distributed under GPL v2 or the BSD License (see accompanying file copying),
your choice.

