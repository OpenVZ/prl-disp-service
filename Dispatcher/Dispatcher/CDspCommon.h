///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspCommon.h
///
/// Header to implement common dispatcher macroses
///
/// @author sergeyt
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// How use COMMON_TRY / COMMON_CATCH  construction
///
/// COMMON_TRY
/// {
///    // some code with 'throw'
/// }
/// COMMON_CATCH
///
/// COMMON_CATCH_WITH_INT_PARAM
///
///////////////////////////////////////////////////////////////////////////////
#ifndef H__CDspCommon__H
#define  H__CDspCommon__H

#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"


//////////////////////////////////////////////////////////////////////////
//
//	COMMON_TRY
//
//////////////////////////////////////////////////////////////////////////
#ifdef COMMON_TRY
#   error "macros COMMON_TRY already defined"
#else
#   define COMMON_TRY try
#endif //COMMON_TRY


//////////////////////////////////////////////////////////////////////////
//
//	COMMON_CATCH
//
//////////////////////////////////////////////////////////////////////////
#ifdef COMMON_CATCH
#   error "macros COMMON_CATCH already defined"
#else
#   define COMMON_CATCH \
    catch ( std::exception& e ) \
    { \
        WRITE_TRACE(DBG_FATAL, "%s:%d std error was catched: [ %s ]", __FILE__, __LINE__, \
            e.what() ); \
    } \
    catch ( PRL_RESULT err ) \
    {                               \
        WRITE_TRACE(DBG_FATAL, "%s:%d PRL_RESULT error was catched: [ %#x, %s ]",  __FILE__, __LINE__, \
            err, PRL_RESULT_TO_STRING( err ) );  \
    }   \
    catch ( ... ) \
    {   \
        WRITE_TRACE(DBG_FATAL, "%s:%d UNEXPECTED error was catched",  __FILE__, __LINE__ ); \
    }   \

#endif //COMMON_CATCH


//////////////////////////////////////////////////////////////////////////
//
//	COMMON_CATCH_WITH_INT_PARAM
//
//////////////////////////////////////////////////////////////////////////
#ifdef COMMON_CATCH_WITH_INT_PARAM
#   error "macros COMMON_CATCH_WITH_INT_PARAM already defined"
#else
#   define COMMON_CATCH_WITH_INT_PARAM( int_val )  \
	catch ( std::exception& e ) \
	{   int i = (int_val); Q_UNUSED( i ); /* to check 'int' type only */  \
	WRITE_TRACE(DBG_FATAL, "%s:%d std error was catched: [ %s ], PARAM = %d", __FILE__, __LINE__, \
	e.what(), (int_val) ); \
	} \
	catch ( PRL_RESULT err ) \
	{                               \
	WRITE_TRACE(DBG_FATAL, "%s:%d PRL_RESULT error was catched: [ %#x, %s ] , PARAM = %d",  __FILE__, __LINE__, \
	err, PRL_RESULT_TO_STRING( err ), (int_val) );  \
	}   \
	catch ( ... ) \
	{   \
	WRITE_TRACE(DBG_FATAL, "%s:%d UNEXPECTED error was catched, PARAM = %d",  __FILE__, __LINE__, \
	(int_val) ); \
	}   \

#endif //COMMON_CATCH_WITH_INT_PARAM

//Secure storage defines
#define SECURE_TAG_ARRAY_BACKUP						"AB"
#define SECURE_TAG_BACKUP_LOGIN						"BL"
#define SECURE_TAG_BACKUP_PASSWORD					"BP"

// user cached data
#define SECURE_TAG_ARRAY_PROXY_CACHE				"CD"
#define SECURE_TAG_PROXY_SERVER_NAME				"PSN"
#define SECURE_TAG_PROXY_SERVER_PORT				"PSP"
#define SECURE_TAG_PROXY_SERVER_USER_NAME			"PUN"
#define SECURE_TAG_PROXY_SERVER_USER_PASSWORD		"PUP"

// user defined by hands settings
#define SECURE_TAG_PROXY_USER_DEFINED_GROUP			"PUDG"
#define SECURE_TAG_PROXY_DONT_USE_COMMON_SERVER		"PDNotUCS"
#define SECURE_TAG_PROXY_HTTP_AUTH_ENABLED			"PHAE"
#define SECURE_TAG_PROXY_USERNAME_HTTP				"PHU"
#define SECURE_TAG_PROXY_PASSWORD_HTTP				"PHP"
#define SECURE_TAG_PROXY_HTTPS_AUTH_ENABLED			"PHsAE"
#define SECURE_TAG_PROXY_USERNAME_HTTPS				"PHsU"
#define SECURE_TAG_PROXY_PASSWORD_HTTPS				"PHsP"

#define SECURE_TAG_AUTOSTART						"autostart"
#define SECURE_TAG_USERCRED							"user"
#define SECURE_TAG_USER_SSL_CRED					"sslcred"
#define SECURE_TAG_USERPASS							"data"
#define SECURE_TAG_FAST_REBOOT_USER					"FRuser"

#define SECURE_TAG_MOBILE_HOST_DATA					"MHD"
#define SECURE_TAG_MOBILE_HOST_USER_DEFINED_ID		"MHUID"

//END of secure storage defines

#endif // H__CDspCommon__H

