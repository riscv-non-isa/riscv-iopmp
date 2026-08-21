/*
 * Copyright 2018-2026 Andes Technology Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __IOPMP_UTIL_H__
#define __IOPMP_UTIL_H__

#include <stdio.h>

/* GCOVR_EXCL_START */
static inline void __assert_func(const char *filename,
                                 int line,
                                 const char *assert_func,
                                 const char *expr)
{
    printf("%s:%d: assertion \"%s\" failed in function %s\n",
           filename, line, expr, assert_func);

    while (1) {
        /* A failed assertion is not recoverable: hang here */
    }
}
/* GCOVR_EXCL_STOP */

#ifdef DEBUG
#define assert(__e) ((__e) ? (void)0 :  \
                             __assert_func(__FILE__, __LINE__, __func__, #__e))
#else
#define assert(__e) ((void)0)
#endif

#endif  /* __IOPMP_UTIL_H__ */
