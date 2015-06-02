//////////////////////////////////////////////////////////////////////////
///
/// @file std_hash.h
///
/// @brief
/// Simple intruisive fixed-size hash table.
///
/// @author Parallels
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
//////////////////////////////////////////////////////////////////////////
#ifndef __STD_HASH_H__
#define __STD_HASH_H__

#include "std_list.h"

/**
Hash-table entry should have fields
list to link entries in list and key to use in hash-table:

struct HashEntry {
	cd_list list;
	KEY_TYPE key;
}

hash_function should looks like int hash_func(KEY_TYPE key, int m);
*/


#define DECLARE_HASHTABLE_TYPE(HASHTYPE_NAME, KEY_TYPE, VAL_TYPE, M_SIZE, hash_function) \
	\
struct HASHTYPE_NAME	\
{	\
	struct cd_list h_table[M_SIZE];	\
};	\
	\
static __inline void HASHTYPE_NAME##_INIT(struct HASHTYPE_NAME *pHashTable)	\
{	\
	int i; \
	for(i = 0; i < M_SIZE; ++i ) \
	{ \
		cd_list_init(&pHashTable->h_table[i]); \
	} \
} \
	\
	\
static __inline void HASHTYPE_NAME##_CLEAR(struct HASHTYPE_NAME *pHashTable) \
{ \
	int i; \
	for(i = 0; i < M_SIZE; ++i ) \
	{ \
		cd_list_init(&pHashTable->h_table[i]); \
	} \
} \
	\
	\
static __inline void HASHTYPE_NAME##_INSERT(struct HASHTYPE_NAME *pHashTable, VAL_TYPE *pVal) \
{ \
	VAL_TYPE *pEntry; \
	int h = hash_function(pVal->key, M_SIZE); \
	cd_list_for_each_entry(VAL_TYPE, pEntry, &pHashTable->h_table[h], list) \
	{ \
		if (pEntry->key == pVal->key ) \
			return;  \
	}  \
	cd_list_add(&pVal->list, &pHashTable->h_table[h]); \
} \
	\
	\
static __inline void HASHTYPE_NAME##_REMOVE(struct HASHTYPE_NAME *pHashTable, VAL_TYPE *pVal) \
{ \
	cd_list_del(&pVal->list); \
} \
	\
	\
static __inline VAL_TYPE *HASHTYPE_NAME##_SEARCH(struct HASHTYPE_NAME *pHashTable, KEY_TYPE key) \
{ \
	VAL_TYPE *pEntry; \
	int h = hash_function(key, M_SIZE); \
	cd_list_for_each_entry(VAL_TYPE, pEntry, &pHashTable->h_table[h], list) \
	{ \
		if (pEntry->key == key ) \
			return pEntry; \
	} \
	return NULL; \
}

#define HASHTABLE_ENUM(VAL_TYPE, pHashTable, enum_cb, cb_ctx)	\
do \
{	\
	VAL_TYPE *pVal, *tmp; \
	int i; \
	for (i = 0; i<sizeof((pHashTable)->h_table)/sizeof((pHashTable)->h_table[0]); ++i) \
	{ \
		cd_list_for_each_entry_safe(VAL_TYPE, pVal, tmp, &(pHashTable)->h_table[i], list) \
		{ \
			enum_cb(pVal, cb_ctx); \
		} \
	} \
} while(0);

#endif //__STD_HASH_H__
