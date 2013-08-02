/**
 * License GPL3
 * @file   lmn_map_util.h
 * @brief  common definition
 * @author Taketo Yoshida
*/
#ifndef LMN_MAP_UTIL_H
#  define LMN_MAP_UTIL_H

#include <stdlib.h>

#define LMN_CAS(a_ptr, a_old, a_new) __sync_bool_compare_and_swap(a_ptr, a_old, a_new)
#define LMN_CAST(type, value)        (value)type

#define lmn_malloc(size, type)       (type*)malloc(sizeof(type)*size)

#define DEBUG 1

#if defined(DEBUG)
#define dbgprint(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define dbgprint(fmt, ...)
#endif

typedef unsigned int lmn_key_t;
typedef void*        lmn_data_t;

#endif /* ifndef LMN_MAP_UTIL_H */

