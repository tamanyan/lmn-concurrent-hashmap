/**
 * License GPL3
 * @file   lmn_map_util.h
 * @brief  common definition
 * @author Taketo Yoshida
*/
#ifndef LMN_MAP_UTIL_H
#  define LMN_MAP_UTIL_H

#define LMN_CAS(a_ptr, a_old, a_new) __sync_bool_compare_and_swap(a_ptr, a_old, a_new)
#define LMN_CAST(type, value)        (value)type

typedef unsigned int lmn_key_t;
typedef void*        lmn_data_t;

#endif /* ifndef LMN_MAP_UTIL_H */

