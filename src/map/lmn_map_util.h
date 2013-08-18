/**
 * @file   lmn_map_util.h
 * @brief  common definition
 * @author Taketo Yoshida
 */
#ifndef LMN_MAP_UTIL_H
#  define LMN_MAP_UTIL_H

#include <stdlib.h>
#include <string.h>

#ifndef HASHMAP_LOCK_FREE
#include <pthread.h>
#define HASHMAP_SEGMENT 12
#endif

#define LMN_CAS(a_ptr, a_old, a_new) __sync_bool_compare_and_swap(a_ptr, a_old, a_new)
#define LMN_CAST(type, value)        (value)type
#define LMN_ATOMIC_ADD(ptr, v)       __sync_fetch_and_add(ptr, v)

#define lmn_malloc(size, type)       (type*)malloc(sizeof(type)*(size))
#define lmn_calloc(size, type)       (type*)calloc((size), sizeof(type))
#define lmn_free(ptr)                free(ptr);

#define DEBUG 1

#if defined(DEBUG)
#define dbgprint(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define dbgprint(fmt, ...)
#endif

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef unsigned long       lmn_word;
typedef unsigned int        lmn_key_t;
typedef void*               lmn_data_t;

#endif /* ifndef LMN_MAP_UTIL_H */

