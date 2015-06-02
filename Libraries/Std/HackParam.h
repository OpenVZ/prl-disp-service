///////////////////////////////////////////////////////////////////////////////
///
/// @file SystemFlags.h
///
/// Module for parsing of monitor parameters passed "by name" from the
/// application to allow tiny "customization" of the monitor
/// behaviour with internal parameters.
///
/// The sample of usage is the following:
///
///     char aParam[] =
///			"decintparam = 10;"
///			"hexintparam = 0x10;"
///			"decin64param = 20; hexint64param = 0x20;"
///
///     SfInit( aParam, sizeof(aParam) );
///     ULONG val = SfGetULONG( "decintparam", 100 );
///     ASSERT( val == 10 );
///
/// @author maximk
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __SYSTEM_FLAGS_H__
#define __SYSTEM_FLAGS_H__

#include <prlsdk/PrlTypes.h> // for PRL_CONST_STR, PRL_UINT32, etc.

#define			MONITOR_SYSTEM_FLAGS_STRING_SIZE	(0x500)

/**
 * Internal state of the hack parameters subsystem
 * Consists of all variables and declarations vital
 * for its functioning.
 */
typedef struct _SYSTEM_FLAGS_STATE
{

	/// Attribute showing that subsystem was or not inited
	PRL_BOOL bInited;

	/// Size of the string in the buffer
	PRL_UINT32 uSize;

	/// Copy of the parameters buffer
	/// (filled in SfInit method)
	PRL_CHAR aSystemFlags[ MONITOR_SYSTEM_FLAGS_STRING_SIZE ];

} SYSTEM_FLAGS_STATE;

/**
 * Returns true if Sf state was inited.
 */
PRL_BOOL SfIsInited();
PRL_BOOL SfIsInitedLocal(SYSTEM_FLAGS_STATE *SfState);

/**
 * Initialize hack parameters module with the buffer.
 * Values in this buffer are used by different accessors
 * methods to get the actual value.
 * @see SfGetULONG32
 * @see SfGetULONG64
 *
 * @param buffer with the actual parameters
 * @param size of the buffer
 */
void SfInit( PRL_CONST_STR strBuffer, PRL_UINT32 uSize );
void SfInitLocal(SYSTEM_FLAGS_STATE *SfState,  PRL_CONST_STR strBuffer, PRL_UINT32 uSize  );

/**
 * Parse parameter value from the buffer by name
 * @param name of the parameter
 * @param default value of the parameter
 * @return parsed value of default value parse on error
 */
PRL_UINT32 SfGetUINT32( PRL_CONST_STR strName, PRL_UINT32 uDefaultValue );
PRL_UINT32 SfGetUINT32Local(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName, PRL_UINT32 uDefaultValue );

/**
 * Parse parameter value from the buffer by name
 * @param name of the parameter
 * @param default value of the parameter
 * @return parsed value of default value parse on error
 */
PRL_UINT64 SfGetUINT64( PRL_CONST_STR strName, PRL_UINT64 uDefaultValue );
PRL_UINT64 SfGetUINT64Local(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName, PRL_UINT64 uDefaultValue );

/**
 * Parse parameter value from the buffer by name
 * @param name of the parameter
 * @param default value of the parameter
 * @return parsed value of default value parse on error
 */
PRL_CONST_STR SfGetString( PRL_CONST_STR strName, PRL_CONST_STR strDefaultValue );
PRL_CONST_STR SfGetStringLocal(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName, PRL_CONST_STR strDefaultValue );

#endif // __SYSTEM_FLAGS_H__
