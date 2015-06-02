///////////////////////////////////////////////////////////////////////////////
///
/// @file SystemFlags.cpp
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


#include "Interfaces/ParallelsTypes.h"
#include "Interfaces/ParallelsCompiler.h"
#include "HackParam.h"

#include "Libraries/Logging/Logging.h"

#include <stdio.h>
#include <string.h>
#include <QtGlobal>

#ifdef _WIN_
	#define StrniCmp _strnicmp
#else
	#define StrniCmp strncasecmp
#endif


/**
 * Static variable holding subsystem state
 */
static SYSTEM_FLAGS_STATE s_SfState;


/**
 * Internal module method.
 * Small utility function to ease parsing.
 */
static BOOL SfIsParamSep( CHAR c )
{
	if ( c == ' ' || c == '\t' )
		return TRUE;

	return FALSE;
}


/**
 * Internal module method.
 * Small utility function to ease parsing.
 */
static BOOL SfIsValueSep( CHAR c )
{
	if ( c == '\n' || c == '\r' || c == ';'
			|| c == ',' || c == 0 || c == '|' )
		return TRUE;

	return FALSE;
}

/**
 * Returns true if Sf state was inited.
 */
PRL_BOOL SfIsInited()
{
	return SfIsInitedLocal(&s_SfState);
}

PRL_BOOL SfIsInitedLocal(SYSTEM_FLAGS_STATE *SfState)
{
	return !!SfState->bInited;
}

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
void SfInit(  PRL_CONST_STR strBuffer, PRL_UINT32 uSize  )
{
	SfInitLocal(&s_SfState, strBuffer, uSize);
}

void SfInitLocal(SYSTEM_FLAGS_STATE *SfState,  PRL_CONST_STR strBuffer, PRL_UINT32 uSize  )
{
	UINT i;

	if ( uSize > MONITOR_SYSTEM_FLAGS_STRING_SIZE - 2 )
		uSize = MONITOR_SYSTEM_FLAGS_STRING_SIZE - 2;

	memset( SfState->aSystemFlags, 0, MONITOR_SYSTEM_FLAGS_STRING_SIZE );
	memcpy( SfState->aSystemFlags + 1, strBuffer, uSize++ );

	BOOL bWaitForQuote = FALSE;

	// Replacing some charecters to ease parameters parsing
	for ( i = 0; i < uSize; i++ )
	{
		CHAR& c = SfState->aSystemFlags[i];

		BOOL bNeedIgnoreSybol = FALSE;

		if ( c == '"' )
			bWaitForQuote = !bWaitForQuote;

		if ( SfIsParamSep( c ) && !bWaitForQuote )
		{
			bNeedIgnoreSybol = TRUE;
		}

		if ( SfIsValueSep( c ) && !bWaitForQuote )
		{
			c = '\n';
		}

		if ( bNeedIgnoreSybol )
		{
			memmove(
				&SfState->aSystemFlags[ i ],
				&SfState->aSystemFlags[i + 1],
				uSize - i - 1
				);

			SfState->aSystemFlags[ uSize ] = 0;

			--uSize;
			--i;
		}
	}

	// System flags buffer must be terminated by end-of-string
	SfState->aSystemFlags[ MONITOR_SYSTEM_FLAGS_STRING_SIZE - 1] = 0;

	SfState->bInited = TRUE;
	SfState->uSize = uSize;
}


/**
 * Internal module method.
 * Search for the parameter value string pointer.
 * @param parameter value
 * @return pointer to the value or NULL on error
 */
static PCHAR GetParamValuePtr(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName )
{
	UINT i;
	UINT uNameLen = (UINT)strlen( strName );
	PCHAR strRet = NULL;

	for ( i = 1; i < SfState->uSize; i++ )
	{
		UINT uSizeLeft = SfState->uSize - i;

		// Nothing left to do
		if ( uNameLen > uSizeLeft )
			break;

		CHAR cPrev = SfState->aSystemFlags[ i - 1];
		CHAR cCurr = SfState->aSystemFlags[ i ];

		if (  SfIsValueSep( cPrev ) && !SfIsValueSep( cCurr ) )
		{
			int ret =
				StrniCmp(
					strName,
					&SfState->aSystemFlags[ i ],
					uNameLen
					);

			if ( ret == 0 )
			{
				if ( SfState->aSystemFlags[ i + uNameLen ] == '=' )
				{
					strRet = &SfState->aSystemFlags[ i + uNameLen + 1 ];
					break;
				}
			}
		}
	}

	return strRet;
}

PRL_UINT32 SfGetUINT32( PRL_CONST_STR strName, PRL_UINT32 uDefaultValue )
{
	return SfGetUINT32Local(&s_SfState, strName, uDefaultValue );
}


/**
 * Parse parameter value from the buffer by name
 * @param name of the parameter
 * @param default value of the parameter
 * @return parsed value of default value parse on error
 */
PRL_UINT32 SfGetUINT32Local(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName, PRL_UINT32 uDefaultValue )
{
	if ( !SfState->bInited ) {
#ifndef EXTERNALLY_AVAILABLE_BUILD
		WRITE_TRACE(DBG_FATAL, "SfGetUINT32 called, but SF engine has not "
					"initialized yet %s", strName);
#endif
		return uDefaultValue;
	}

	PCHAR strValue = GetParamValuePtr( SfState, strName );

	if ( !strValue )
		return uDefaultValue;

	if ( StrniCmp( strValue, "0x", 2) == 0 )
		sscanf( strValue, "0x%x", &uDefaultValue );
	else
		sscanf( strValue, "%u", &uDefaultValue );

    WRITE_TRACE( DBG_FATAL, "SystemFlag '%s' = 0x%x (%u)",
            strName, uDefaultValue, uDefaultValue );

	return uDefaultValue;
}



/**
 * Parse parameter value from the buffer by name
 * @param name of the parameter
 * @param default value of the parameter
 * @return parsed value of default value parse on error
 */
PRL_UINT64 SfGetUINT64( PRL_CONST_STR strName, PRL_UINT64 uDefaultValue )
{
	return SfGetUINT64Local(&s_SfState, strName, uDefaultValue);
}

PRL_UINT64 SfGetUINT64Local(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName, PRL_UINT64 uDefaultValue )
{
	if ( !SfState->bInited ) {
#ifndef EXTERNALLY_AVAILABLE_BUILD
		WRITE_TRACE(DBG_FATAL, "SfGetUINT64 called, but SF engine has not "
					"initialized yet %s", strName);
#endif
		return uDefaultValue;
	}

	PCHAR strValue = GetParamValuePtr(SfState, strName );

	if ( !strValue )
		return uDefaultValue;

	if ( StrniCmp( strValue, "0x", 2) == 0 )
		sscanf( strValue, "0x%llx", &uDefaultValue );
	else
		sscanf( strValue, "%llu", &uDefaultValue );
	WRITE_TRACE( DBG_FATAL, "SystemFlag '%s' = 0x%llx (%llu)",
			strName, uDefaultValue, uDefaultValue );

	return uDefaultValue;
}


/**
 * Parse parameter value from the buffer by name
 * @param name of the parameter
 * @param default value of the parameter
 * @return parsed value of default value parse on error
 */
PRL_CONST_STR SfGetString(PRL_CONST_STR strName, PRL_CONST_STR strDefaultValue )
{
	return SfGetStringLocal(&s_SfState, strName, strDefaultValue );
}

PRL_CONST_STR SfGetStringLocal(SYSTEM_FLAGS_STATE* SfState, PRL_CONST_STR strName, PRL_CONST_STR strDefaultValue )
{
	if ( !SfState->bInited ) {
#ifndef EXTERNALLY_AVAILABLE_BUILD
		WRITE_TRACE(DBG_FATAL, "SfGetString called, but SF engine has not "
					"initialized yet %s", strName);
#endif
		return strDefaultValue;
	}

	PCHAR strValue = GetParamValuePtr(SfState, strName );

	if ( !strValue )
		return strDefaultValue;

	bool bStr = false;
	if ( strValue[0] == '"' )
	{
		bStr = true;
		strValue++;
	}

	// Searching for the terminating " if needed
	unsigned int i = 0;
	for ( ; &strValue[i] < &SfState->aSystemFlags[MONITOR_SYSTEM_FLAGS_STRING_SIZE]; i++ )
	{
		if ( (bStr && strValue[i] == '"') || (!bStr && SfIsValueSep( strValue[i] ) ) )
		{
			strValue[i] = 0;
			break;
		}
	}

    WRITE_TRACE( DBG_FATAL, "SystemFlag '%s' = '%s'", strName, strValue );

	return strValue;
}
