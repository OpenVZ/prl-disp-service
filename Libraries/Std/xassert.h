/**
* @file xassert.h
*
* @brief Defines assert function that can be used on any platform
*
* To use this header:
* - _USERSPACE_MODE_ must be defined in userspace
* - _KERNELSPACE_MODE_ must be defined in kernelspace
*
* @author andreyp@parallels.com
* @author owner is alexg@parallels.com
*
* Copyright (c) 2005-2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
*/

#ifndef PRL_LIB_STD_ASSERT_INCLUDED
#define PRL_LIB_STD_ASSERT_INCLUDED

////////////////////////////////////////////////////////////////////////////////
// includes
#include "Interfaces/ParallelsTypes.h"



////////////////////////////////////////////////////////////////////////////////
// defines

// Define assert macros --------------------------------------------------------

#if defined(_USERSPACE_MODE_)

	// In userspace we can use assert() macros from standard header <assert.h>
	#include <assert.h>

#elif defined(_KERNELSPACE_MODE_)

	#if defined(_WIN_)

		// Note, that such tanci s bubnom are required only for windows
		// kernelspace where we have no normal assert() macros
		#if DBG
			// In windows kernelspace we will use RtlAssert() function
			#include <wdm.h>

			// Grabbed from <wdm.h> header
			#define assert(exp)												\
				((!(exp))													\
				? (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE)		\
				: TRUE)
		#else
			// Grabbed from <wdm.h> header
			#define assert(exp) ((void)0)
		#endif

	#elif defined(_MAC_)

		// We have nice asserts in Mac OS X kernelspace
		#include <kern/assert.h>

	#else
		#error No platform support
	#endif

#else
	#error Userspace or kernelspace mode must be selected
#endif



#endif	// PRL_LIB_STD_ASSERT_INCLUDED
