/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include "synonyms.h"
#include <sys/types.h>

/* table of 64 multiples of 10**1 */
const unsigned short __tbl_10_small_digits [] = { 1,
/* 10**1 = */
5 /* h    1 */,
/* 10**2 = */
25 /* h    2 */,
/* 10**3 = */
125 /* h    3 */,
/* 10**4 = */
625 /* h    4 */,
/* 10**5 = */
3125 /* h    5 */,
/* 10**6 = */
15625 /* h    6 */,
/* 10**7 = */
12589 /* h    7 */, 1 /* h   23 */,
/* 10**8 = */
62945 /* h    8 */, 5 /* h   24 */,
/* 10**9 = */
52581 /* h    9 */, 29 /* h   25 */,
/* 10**10 = */
761 /* h   10 */, 149 /* h   26 */,
/* 10**11 = */
3805 /* h   11 */, 745 /* h   27 */,
/* 10**12 = */
19025 /* h   12 */, 3725 /* h   28 */,
/* 10**13 = */
29589 /* h   13 */, 18626 /* h   29 */,
/* 10**14 = */
16873 /* h   14 */, 27596 /* h   30 */, 1 /* h   46 */,
/* 10**15 = */
18829 /* h   15 */, 6909 /* h   31 */, 7 /* h   47 */,
/* 10**16 = */
28609 /* h   16 */, 34546 /* h   32 */, 35 /* h   48 */,
/* 10**17 = */
11973 /* h   17 */, 41660 /* h   33 */, 177 /* h   49 */,
/* 10**18 = */
59865 /* h   18 */, 11692 /* h   34 */, 888 /* h   50 */,
/* 10**19 = */
37181 /* h   19 */, 58464 /* h   35 */, 4440 /* h   51 */,
/* 10**20 = */
54833 /* h   20 */, 30178 /* h   36 */, 22204 /* h   52 */,
/* 10**21 = */
12021 /* h   21 */, 19822 /* h   37 */, 45486 /* h   53 */, 1 /* h   69 */,

/* 10**22 = */
60105 /* h   22 */, 33574 /* h   38 */, 30823 /* h   54 */, 8 /* h   70 */,

/* 10**23 = */
38381 /* h   23 */, 36802 /* h   39 */, 23045 /* h   55 */, 42 /* h   71 */,

/* 10**24 = */
60833 /* h   24 */, 52940 /* h   40 */, 49691 /* h   56 */, 211 /* h   72 */,

/* 10**25 = */
42021 /* h   25 */, 2560 /* h   41 */, 51851 /* h   57 */, 1058 /* h   73 */,

/* 10**26 = */
13497 /* h   26 */, 12803 /* h   42 */, 62647 /* h   58 */, 5293 /* h   74 */,

/* 10**27 = */
1949 /* h   27 */, 64016 /* h   43 */, 51091 /* h   59 */, 26469 /* h   75 */,

/* 10**28 = */
9745 /* h   28 */, 57936 /* h   44 */, 58851 /* h   60 */, 1276 /* h   76 */,
2 /* h   92 */,
/* 10**29 = */
48725 /* h   29 */, 27536 /* h   45 */, 32115 /* h   61 */, 6384 /* h   77 */,
10 /* h   93 */,
/* 10**30 = */
47017 /* h   30 */, 6611 /* h   46 */, 29505 /* h   62 */, 31922 /* h   78 */,
50 /* h   94 */,
/* 10**31 = */
38477 /* h   31 */, 33058 /* h   47 */, 16453 /* h   63 */, 28540 /* h   79 */,
252 /* h   95 */,
/* 10**32 = */
61313 /* h   32 */, 34220 /* h   48 */, 16731 /* h   64 */, 11629 /* h   80 */,
1262 /* h   96 */,
/* 10**33 = */
44421 /* h   33 */, 40032 /* h   49 */, 18121 /* h   65 */, 58146 /* h   81 */,
6310 /* h   97 */,
/* 10**34 = */
25497 /* h   34 */, 3555 /* h   50 */, 25072 /* h   66 */, 28587 /* h   82 */,
31554 /* h   98 */,
/* 10**35 = */
61949 /* h   35 */, 17776 /* h   51 */, 59824 /* h   67 */, 11864 /* h   83 */,
26700 /* h   99 */, 2 /* h  115 */,
/* 10**36 = */
47601 /* h   36 */, 23348 /* h   52 */, 36977 /* h   68 */, 59324 /* h   84 */,
2428 /* h  100 */, 12 /* h  116 */,
/* 10**37 = */
41397 /* h   37 */, 51207 /* h   53 */, 53814 /* h   69 */, 34478 /* h   85 */,
12144 /* h  101 */, 60 /* h  117 */,
/* 10**38 = */
10377 /* h   38 */, 59430 /* h   54 */, 6929 /* h   70 */, 41322 /* h   86 */,
60722 /* h  102 */, 300 /* h  118 */,
/* 10**39 = */
51885 /* h   39 */, 35006 /* h   55 */, 34649 /* h   71 */, 10002 /* h   87 */,
41469 /* h  103 */, 1504 /* h  119 */,
/* 10**40 = */
62817 /* h   40 */, 43961 /* h   56 */, 42175 /* h   72 */, 50012 /* h   88 */,
10737 /* h  104 */, 7523 /* h  120 */,
/* 10**41 = */
51941 /* h   41 */, 23201 /* h   57 */, 14270 /* h   73 */, 53455 /* h   89 */,
53688 /* h  105 */, 37615 /* h  121 */,
/* 10**42 = */
63097 /* h   42 */, 50472 /* h   58 */, 5815 /* h   74 */, 5132 /* h   90 */,
6300 /* h  106 */, 57007 /* h  122 */, 2 /* h  138 */,
/* 10**43 = */
53341 /* h   43 */, 55756 /* h   59 */, 29078 /* h   75 */, 25660 /* h   91 */,
31500 /* h  107 */, 22891 /* h  123 */, 14 /* h  139 */,
/* 10**44 = */
4561 /* h   44 */, 16640 /* h   60 */, 14322 /* h   76 */, 62766 /* h   92 */,
26429 /* h  108 */, 48921 /* h  124 */, 71 /* h  140 */,
/* 10**45 = */
22805 /* h   45 */, 17664 /* h   61 */, 6075 /* h   77 */, 51687 /* h   93 */,
1077 /* h  109 */, 47999 /* h  125 */, 358 /* h  141 */,
/* 10**46 = */
48489 /* h   46 */, 22785 /* h   62 */, 30376 /* h   78 */, 61827 /* h   94 */,
5388 /* h  110 */, 43387 /* h  126 */, 1793 /* h  142 */,
/* 10**47 = */
45837 /* h   47 */, 48392 /* h   63 */, 20809 /* h   79 */, 46993 /* h   95 */,
26944 /* h  111 */, 20327 /* h  127 */, 8968 /* h  143 */,
/* 10**48 = */
32577 /* h   48 */, 45355 /* h   64 */, 38512 /* h   80 */, 38358 /* h   96 */,
3651 /* h  112 */, 36101 /* h  128 */, 44841 /* h  144 */,
/* 10**49 = */
31813 /* h   49 */, 30169 /* h   65 */, 61491 /* h   81 */, 60720 /* h   97 */,
18257 /* h  113 */, 49433 /* h  129 */, 27599 /* h  145 */, 3 /* h  161 */,

/* 10**50 = */
27993 /* h   50 */, 19775 /* h   66 */, 45313 /* h   82 */, 41460 /* h   98 */,
25753 /* h  114 */, 50558 /* h  130 */, 6926 /* h  146 */, 17 /* h  162 */,

/* 10**51 = */
8893 /* h   51 */, 33341 /* h   67 */, 29958 /* h   83 */, 10695 /* h   99 */,
63232 /* h  115 */, 56183 /* h  131 */, 34633 /* h  147 */, 85 /* h  163 */,

/* 10**52 = */
44465 /* h   52 */, 35633 /* h   68 */, 18720 /* h   84 */, 53477 /* h  100 */,
54016 /* h  116 */, 18775 /* h  132 */, 42097 /* h  148 */, 427 /* h  164 */,

/* 10**53 = */
25717 /* h   53 */, 47096 /* h   69 */, 28066 /* h   85 */, 5242 /* h  101 */,
7940 /* h  117 */, 28343 /* h  133 */, 13878 /* h  149 */, 2138 /* h  165 */,

/* 10**54 = */
63049 /* h   54 */, 38873 /* h   70 */, 9261 /* h   86 */, 26212 /* h  102 */,
39700 /* h  118 */, 10643 /* h  134 */, 3856 /* h  150 */, 10691 /* h  166 */,

/* 10**55 = */
53101 /* h   55 */, 63297 /* h   71 */, 46307 /* h   87 */, 65524 /* h  103 */,
1893 /* h  119 */, 53218 /* h  135 */, 19280 /* h  151 */, 53455 /* h  167 */,

/* 10**56 = */
3361 /* h   56 */, 54345 /* h   72 */, 34931 /* h   88 */, 65479 /* h  104 */,
9469 /* h  120 */, 3946 /* h  136 */, 30868 /* h  152 */, 5132 /* h  168 */,
4 /* h  184 */,
/* 10**57 = */
16805 /* h   57 */, 9581 /* h   73 */, 43587 /* h   89 */, 65253 /* h  105 */,
47349 /* h  121 */, 19730 /* h  137 */, 23268 /* h  153 */, 25662 /* h  169 */,
20 /* h  185 */,
/* 10**58 = */
18489 /* h   58 */, 47906 /* h   74 */, 21327 /* h   90 */, 64124 /* h  106 */,
40141 /* h  122 */, 33117 /* h  138 */, 50805 /* h  154 */, 62775 /* h  170 */,
101 /* h  186 */,
/* 10**59 = */
26909 /* h   59 */, 42923 /* h   75 */, 41102 /* h   91 */, 58477 /* h  107 */,
4101 /* h  123 */, 34516 /* h  139 */, 57419 /* h  155 */, 51734 /* h  171 */,
509 /* h  187 */,
/* 10**60 = */
3473 /* h   60 */, 18009 /* h   76 */, 8905 /* h   92 */, 30244 /* h  108 */,
20509 /* h  124 */, 41508 /* h  140 */, 24953 /* h  156 */, 62066 /* h  172 */,
2548 /* h  188 */,
/* 10**61 = */
17365 /* h   61 */, 24509 /* h   77 */, 44526 /* h   93 */, 20148 /* h  109 */,
37011 /* h  125 */, 10933 /* h  141 */, 59232 /* h  157 */, 48187 /* h  173 */,
12744 /* h  189 */,
/* 10**62 = */
21289 /* h   62 */, 57010 /* h   78 */, 26023 /* h   94 */, 35207 /* h  110 */,
53984 /* h  126 */, 54667 /* h  142 */, 34016 /* h  158 */, 44331 /* h  174 */,
63723 /* h  190 */,
/* 10**63 = */
40909 /* h   63 */, 22907 /* h   79 */, 64583 /* h   95 */, 44964 /* h  111 */,
7778 /* h  127 */, 11195 /* h  143 */, 39012 /* h  159 */, 25049 /* h  175 */,
56474 /* h  191 */, 4 /* h  207 */,
0};

/* table of starting indexes into previous table */
const unsigned short __tbl_10_small_start [] = {
0, 1, 2, 3, 4, 5, 6, 7,
9, 11, 13, 15, 17, 19, 21, 24,
27, 30, 33, 36, 39, 42, 46, 50,
54, 58, 62, 66, 70, 75, 80, 85,
90, 95, 100, 105, 111, 117, 123, 129,
135, 141, 147, 154, 161, 168, 175, 182,
189, 196, 204, 212, 220, 228, 236, 244,
252, 261, 270, 279, 288, 297, 306, 315,
325, 0};
