/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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

#ifndef _SARE_SHARED_HEADER_
#define _SARE_SHARED_HEADER_

#include "Interfaces/Config.h"
#ifdef _WIN_
#include <windows.h>
#endif
#include <QFile>
#include <QString>
#include <prlcommon/Interfaces/ParallelsTypes.h>

#include <prlcommon/Interfaces/packed.h>

//
// Shared *.sav files header containing general public description
// for all interested modules
//

#define SARE_FILE_MAGIC_NUMBER 0x65526153//'eRaS'
#define SARE_FILE_OPTION_MAGIC_NUMBER  0x6974704F//'itpO'

typedef struct _SARE_FILE_HDR{

	UINT uSaReMagicNum;		// Always MUST be SARE_FILE_MAGIC_NUMBER
	UINT uMajorVersion;		// so called HARD version (SAV_FILE_VERSION_HARD) declared in Monitor/Interfaces/Api.h
	UINT uMinorVersion;		// and SOFT version (SAV_FILE_VERSION_SOFT)
	UINT uLegacyFileFormat;// to detect what file format goes below SARE_ID_FILE_OLD or SARE_FILE_MAGIC_NUMBER
	                       // SARE_FILE_MAGIC_NUMBER will be used for future versions
	UINT uBuildVer;		   // corresponding build version
	UINT uRevision;		   // corresponding revision

	UINT Reserved[10];		// for future use

}SARE_FILE_HDR, *PSARE_FILE_HDR;

//
// File option description
//

//
// These constants MUST NOT be changed ever since they appear
//
enum{
SARE_MEMORY_FILE_NAME_HDR_OPT	= 0x1,
SARE_WS_BITMAP_HDR_OPT			= 0x2,
SARE_MEM_CHECKSUM_HDR_OPT		= 0x3,
SARE_DIRTY_PAGES_BITMAP_HDR_OPT	= 0x4,
SARE_MEMORY_FILE_PATH_HDR_OPT	= 0x5,
SARE_CPU_VENDOR_HDR_OPT			= 0x6,
SARE_CR4_MASK_HDR_OPT			= 0x7,
SARE_FEATURES_MASK_HDR_OPT		= 0x8,
SARE_EXT_FEATURES_MASK_HDR_OPT	= 0x9,
SARE_EXT_80000001_ECX_MASK_HDR_OPT = 0xa,
SARE_EXT_80000001_EDX_MASK_HDR_OPT = 0xb,
SARE_EXT_80000007_EDX_MASK_HDR_OPT = 0xc,
SARE_EXT_80000008_EAX_HDR_OPT	= 0xd,
SARE_MAPPED_PAGES_BITMAP_HDR_OPT = 0xe,
SARE_EXT_00000007_EBX_MASK_HDR_OPT = 0xf,
SARE_EXT_0000000D_EAX_MASK_HDR_OPT = 0x10,

SARE_NOTDECLARED_OPT
};

typedef struct _SARE_FILE_HDR_OPTION{

	UINT uOptMark;		// All options should begin from special marker SARE_FILE_OPTION_MAGIC_NUMBER
	UINT uOptId;		// Options' unique identification declared above
	UINT uDataSize;		// Options' data size EXCLUDING header itself
	UINT Reserved;		// For future use
	//-------------		//	 Actual data goes from here

}SARE_FILE_HDR_OPTION, *PSARE_FILE_HDR_OPTION;

#include <prlcommon/Interfaces/unpacked.h>

class	CSRFile
{
private:

	QFile			m_file;
	UINT			m_HeaderSize;

	char			*m_pFileBuf;
	UINT			m_uBufLen;

	UINT			m_VersionMajor;
	UINT			m_VersionMinor;

public:

	CSRFile();
	~CSRFile();

	char *GetFileBuffer();
	INT CreateHeader( void );
	UINT CompleteHeader( void );
	INT WriteOption(
		UINT uOptId,
		void *pBuffer,
		UINT uOptLen
		);
	INT ReWriteOption(
		UINT uOptId,
		void *pBuffer,
		UINT uOptLen
		);
	UINT ReadOption(
		UINT uOptId,
		void *pBuffer,
		UINT* puOptLen
		);
	UINT
		GetOptionSize(
			UINT uOptId
		);

	bool Create(const QString& qsPathFile);
	bool Open(const QString& qsPathFile);
	bool SymLink(const QString& qsPathLink);
	bool AllocateDataBuf(UINT uReqieredBytes);
	bool Close();
	UINT GetBufSize();
	bool FlushData();
	UINT CalcChecksum();
	bool VerifyFile();
	bool VerifyChecksum();
	bool IsCompatible();

	UINT GetVersion(UINT* pMajor);
};

bool
	SaReFileWriteHeader(
		QFile& file,
		PSARE_FILE_HDR pHdr
		);

bool
	SaReFileReadHeader(
		QFile& file,
		PSARE_FILE_HDR pHdr
		);

bool
	SaReFileWriteOption(
		QFile& file,
		UINT uOptId,
		void *pBuffer,
		UINT uOptLen
		);

INT
	SaReFileFindOption(
		QFile& file,
		UINT uOptId,
		quint64& OptionOffset
		);

UINT
	SaReFileReadOption(
		QFile& file,
		UINT uOptId,
		void *pBuffer,
		UINT* puOptLen
		);

UINT
	SaReFileGetHeaderSize(
		QFile& file
		);

#endif //_SARE_SHARED_HEADER_
