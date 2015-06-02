//////////////////////////////////////////////////////////////////////////
///
/// @file std_string.h
///
/// @brief Generic strings functions memset and Co also leave here
///
/// @author Parallels
///
/// Copyright (c) 2006-2015 Parallels IP Holdings GmbH
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

#ifndef __STRING_H__
#define __STRING_H__

// va_list
#include <stdarg.h>

#ifdef _MAC_

// size_t
#include <sys/types.h>

// Taken from libkern/libkern.h
int snprintf(char *, size_t, const char *, ...) __printflike(3,4);
extern int vsnprintf(char *, size_t, const char *, va_list);

#elif defined(_LIN_)

// Taken from linux/kernel.h
extern __printf(3, 4)
int snprintf(char *buf, SIZE_T size, const char *fmt, ...);
extern __printf(3, 0)
int vsnprintf(char *buf, SIZE_T size, const char *fmt, va_list args);

#elif defined(_WIN_)

#define snprintf	_snprintf
#define vsnprintf	_vsnprintf

#endif


#define cd_null_sym '\0'

#define strcpy(pDst, pSrc) cd_strcpy(pDst, pSrc)
static UNUSED char* cd_strcpy(char* pDst,const char* pSrc) {

    char* pCopy = pDst;

    while ((*pCopy++ = *pSrc++)) ;

    return pDst;
}

#define strncpy(pDst, pSrc, uCount) cd_strncpy(pDst, pSrc, uCount)
static UNUSED char* cd_strncpy(char* pDst, const char* pSrc, SIZE_T uCount) {

    char* pStart = pDst;

    while (uCount && (*pDst++ = *pSrc++))
        uCount--;

    if (uCount)
        while (--uCount)
            *pDst++ = cd_null_sym;

    return pStart;
}

#define strlen(pStr) (cd_strlen(pStr))
static UNUSED SIZE_T cd_strlen(const char* pStr) {

    const char* pEos = pStr;

    while ( *pEos++ ) ;

    return (pEos - pStr - 1);
}

#define strcmp(pDst, pSrc) cd_strcmp(pDst, pSrc)
static UNUSED int cd_strcmp(const char* pSrc,const char* pDst) {

    int ret = 0;

    while ( ! (ret = *(unsigned char*)pSrc - *(unsigned char*)pDst) && *pDst)
        ++pSrc, ++pDst;

    if ( ret < 0 )
        ret = -1;
    else if ( ret > 0 )
        ret = 1;

    return ret;
}

#define strncmp(pFirst, pLast, uCount) cd_strncmp(pFirst, pLast, uCount)
static UNUSED int cd_strncmp(const char* pFirst,const char* pLast,unsigned int uCount) {

    if (!uCount)
        return 0;

    while (--uCount && *pFirst && *pFirst == *pLast) {
        pFirst++;
        pLast++;
    }

    return *(unsigned char*)pFirst - *(unsigned char*)pLast;
}

#define strchr(pString, ch) cd_strchr(pString, ch)
static UNUSED char* cd_strchr(const char* pString, int ch) {

    while (*pString && *pString != (char)ch)
        pString++;

    if (*pString == (char)ch)
        return (char*)pString;

    return cd_null_sym;
}

#define strrchr(pString, ch) cd_strrchr(pString, ch)
static UNUSED char* cd_strrchr(const char* pString, int ch) {

    char* pStart = (char*)pString;

    while (*pString++) ;

    while (--pString != pStart && *pString != (char)ch) ;

    if (*pString == (char)ch)
        return (char*)pString;

    return cd_null_sym;
}

#define memcpy(pDst, pSrc, uCount) (cd_memcpy(pDst, pSrc, uCount))
static UNUSED void* cd_memcpy(void* pDst, const void* pSrc, SIZE_T uCount) {

	void* ret = pDst;
	SIZE_T n;

	/* copy by longs first */
	n = uCount / sizeof(long);
	while (n--) {
		*(long*)pDst = *(long*)pSrc;
		pDst = (long*)pDst + 1;
		pSrc = (long*)pSrc + 1;
	}

	/* copy bytes left */
	n = uCount & (sizeof(long) - 1);
	while (n--) {
		*(char*)pDst = *(char*)pSrc;
		pDst = (char*)pDst + 1;
		pSrc = (char*)pSrc + 1;
	}

	return ret;
}

#define memcmp(pBuf1, pBuf2, uCount) (cd_memcmp(pBuf1,pBuf2,uCount))
static UNUSED int cd_memcmp(const void* pBuf1, const void* pBuf2, unsigned int uCount) {

    if (!uCount)
        return 0;

    while ( --uCount && *(char*)pBuf1 == *(char*)pBuf2 ) {
        pBuf1 = (char*)pBuf1 + 1;
        pBuf2 = (char*)pBuf2 + 1;
    }

    return *((unsigned char*)pBuf1) - *((unsigned char*)pBuf2);
}

#define memset(pDst, val, uCount) (cd_memset(pDst, val, uCount))
static UNUSED void* cd_memset(void* __pDst, int val, SIZE_T uCount)
{
	SIZE_T n;
#ifdef _AMD64_
typedef UINT64	copy_type;
#else
typedef unsigned long copy_type;
#endif
	copy_type lval;
	copy_type *pDst = (copy_type *)__pDst;

	if (uCount >= 32) {
		/* memset by longs first */
		val &= 0xFF;
		lval = (val << 8) | val;
		lval = (lval << 16) | lval;
#ifdef _AMD64_
		lval = (lval << 32) | lval;
#endif
		n = uCount / sizeof(lval);
		while (n--)
			*pDst++ = lval;

		n = uCount & (sizeof(lval) - 1);
	} else
		n = uCount;

	/* memset bytes left */
	while (n--) {
		*(unsigned char*)pDst = (unsigned char)val;
		pDst = (copy_type *)((unsigned char *)pDst + 1);
	}

    return __pDst;
}

#define strcat(pDst, pSrc) cd_strcat(pDst, pSrc)
static UNUSED char* cd_strcat(char* pDst,const char* pSrc) {

    char* pCopy = pDst;
    while ( *pCopy )
        pCopy++;

    while ((*pCopy++ = *pSrc++)) ;

    return pDst;
}

#define strncat(pFront, pBack, uCount) cd_strncat(pFront, pBack, uCount)
static UNUSED char* cd_strncat(char* pFront,const char* pBack,unsigned int uCount) {

    char* pStart = pFront;

    while (*pFront++) ;
    pFront--;

    while (uCount--)
        if (!(*pFront++ = *pBack++))
            return pStart;

    *pFront = cd_null_sym;
    return pStart;
}

#endif /* __STRING_H__ */
