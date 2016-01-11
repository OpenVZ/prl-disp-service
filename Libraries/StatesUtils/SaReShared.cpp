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

#include <prlcommon/Logging/Logging.h>
#include "sarefile.h"
#include "Build/Current.ver"

#include "SaReShared.h"

#ifndef _WIN_
    #include <fcntl.h>
    #include <errno.h>
	#include <sys/stat.h>
#endif

#define SARE_FI_BOUND 0x10

UINT SaReFileGetHeaderSize(QFile& file)
{
	SARE_FILE_HDR  SaReHdr;
	INT iOptLen;
	UINT uHeaderSize = 0;
	UINT uOptId;

	//
	// The first read
	//
	SaReHdr.uMajorVersion = -1;
	SaReHdr.uMinorVersion = -1;
	SaReHdr.uSaReMagicNum = -1;

	SaReFileReadHeader(file, &SaReHdr);

	if(SARE_FILE_MAGIC_NUMBER != SaReHdr.uSaReMagicNum){

		return uHeaderSize;
	}

	uHeaderSize = sizeof(SARE_FILE_HDR);
	for (uOptId = SARE_MEMORY_FILE_NAME_HDR_OPT; uOptId < SARE_NOTDECLARED_OPT; uOptId++)
	{
		quint64 pos;
		iOptLen = SaReFileFindOption(file, uOptId, pos);
		if (iOptLen > 0){
			iOptLen += SARE_FI_BOUND - (iOptLen&(SARE_FI_BOUND-1));
			uHeaderSize += sizeof(SARE_FILE_HDR_OPTION) + iOptLen;
		}
	}

	return uHeaderSize;
}


bool
	SaReFileWriteHeader(
		QFile& file,
		PSARE_FILE_HDR pHdr
		)
{
	quint64 uWrittenBytes;

	if(NULL == pHdr){

		return false;
	}


	file.seek(0);
	uWrittenBytes = file.write((const char *)pHdr, sizeof(*pHdr));

	return uWrittenBytes == sizeof(*pHdr);
}

bool
	SaReFileReadHeader(
		QFile& file,
		PSARE_FILE_HDR pHdr
		)
{
	quint64 uReadBytes;

	if(NULL == pHdr){

		return false;
	}

	file.seek(0);
	uReadBytes = file.read((char*)pHdr, sizeof(*pHdr));

	return uReadBytes == sizeof(*pHdr);
}

INT SaReFileFindOption(QFile& file, UINT uOptId, quint64& OptionOffset )
{
	SARE_FILE_HDR_OPTION OptionHdr;
	UINT uActualLen;
	INT	iOptSize = 0;
	quint64 filePosSaved;
	quint64 uFilePos = 0;
	// Invalid option identifier
	if (uOptId >= SARE_NOTDECLARED_OPT)
		return -1;

	// Save file position
	filePosSaved = file.pos();

	// go to the begin of option list
	file.seek(sizeof(SARE_FILE_HDR));
	uFilePos = sizeof(SARE_FILE_HDR);

	OptionHdr.uDataSize = (UINT)-1;
	OptionHdr.uOptMark = (UINT)-1;

	// read first option
	uActualLen = file.read((char*)&OptionHdr, sizeof(OptionHdr));
	uFilePos += uActualLen;

	while(SARE_FILE_OPTION_MAGIC_NUMBER == OptionHdr.uOptMark){

		if(sizeof(OptionHdr) != uActualLen)
			break;

		if(OptionHdr.uOptId ==  uOptId){

			OptionOffset = uFilePos;
			iOptSize = OptionHdr.uDataSize;
			break;
		}

		// update file pointer to the next option entry
		uFilePos += OptionHdr.uDataSize;
		// aligning to property boundary
		uFilePos += (SARE_FI_BOUND - (OptionHdr.uDataSize&(SARE_FI_BOUND-1))) % SARE_FI_BOUND;
		// read next option
		file.seek(uFilePos);
    	uActualLen = file.read((char*)&OptionHdr, sizeof(OptionHdr));
		uFilePos += uActualLen;
	}
	// Restore file position
	file.seek(filePosSaved);
	return iOptSize;
}

UINT
	SaReFileReadOption(
		QFile& file,
		UINT uOptId,
		void *pBuffer,
		UINT *puOptLen
		)
{
	UINT uActualLen;
	quint64 OptionOffset;
	if(NULL == puOptLen){

		return -1;
	}
	uActualLen = SaReFileFindOption(file, uOptId, OptionOffset);
	if (uActualLen > 0){

		if (*puOptLen >= uActualLen && pBuffer){
			file.seek(OptionOffset);
			uActualLen = file.read((char*)pBuffer, uActualLen);
		}
		else
			uOptId = (UINT)-1;
		*puOptLen = uActualLen;
		return uOptId;
	}
	return -1;
}

bool
	SaReFileWriteOption(
		QFile& file,
		UINT uOptId,
		void *pBuffer,
		UINT uOptLen
		)
{
	qint64 uWrittenBytes;
	SARE_FILE_HDR_OPTION OptionHdr;
	char BoundBuf[SARE_FI_BOUND] = {0};
	UINT BoundLen = 0;

	if(NULL == pBuffer){

		return false;
	}

	OptionHdr.uDataSize = uOptLen;
	OptionHdr.uOptId = uOptId;
	OptionHdr.uOptMark = SARE_FILE_OPTION_MAGIC_NUMBER;
	OptionHdr.Reserved = 0; // to make valgrind happy

	uWrittenBytes = file.write((const char *)&OptionHdr, sizeof(OptionHdr));

	if(uWrittenBytes != sizeof(OptionHdr))
		return false;

	uWrittenBytes = file.write((const char *)pBuffer, uOptLen);

	// update file pointer to the nearest valid boundary
	if(uOptLen&(SARE_FI_BOUND-1)){

		BoundLen = SARE_FI_BOUND - (uOptLen&(SARE_FI_BOUND-1));

		if(BoundLen != file.write((const char *)BoundBuf, BoundLen)){

			return false;
		}
		uWrittenBytes += BoundLen;
	}

	return uWrittenBytes == (uOptLen + BoundLen);
}

CSRFile::CSRFile()
{
	m_pFileBuf = NULL;
	m_uBufLen = 0;
	m_HeaderSize = 0;
}

CSRFile::~CSRFile()
{
	m_uBufLen = 0;
	delete [] m_pFileBuf;
	m_file.close();
}

bool CSRFile::Open(const QString& qsPathFile)
{
	m_file.close();
	m_HeaderSize = 0;

	m_file.setFileName(qsPathFile);

	if(!m_file.exists()){

		return false;
	}

	if(!m_file.open(QIODevice::ReadOnly)){

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Read header and update FileSize to m_HeaderSize value
	//
	m_HeaderSize = SaReFileGetHeaderSize(m_file);

	//
	// Initial buffer size is 16MB
	//
	if(!AllocateDataBuf(0x1000000)){

		return false;
	}

	return true;
}

bool CSRFile::Create(const QString& qsPathFile)
{
	m_file.close();
	m_HeaderSize = 0;
	m_file.setFileName(qsPathFile);
	if(!m_file.open(QIODevice::ReadWrite | QIODevice::Truncate)){

		return false;
	}

	//
	// Initial buffer size is 16MB
	//
	if(!AllocateDataBuf(0x1000000)){

		return false;
	}

	return true;
}

bool CSRFile::SymLink(const QString& qsPathLink)
{
	if (m_file.fileName().isEmpty())
		return false;
	return m_file.link(qsPathLink);
}

bool CSRFile::Close()
{
	if(m_pFileBuf){

		m_uBufLen = 0;
		delete [] m_pFileBuf;
		m_pFileBuf = NULL;
	}

	m_file.close();
	return true;
}

INT	CSRFile::CreateHeader()
{
	SARE_FILE_HDR	SaReHdr;

	SaReHdr.uMajorVersion = SAV_FILE_VERSION_HARD;
	SaReHdr.uMinorVersion = SAV_FILE_VERSION_SOFT;
	SaReHdr.uSaReMagicNum = SARE_FILE_MAGIC_NUMBER;
	SaReHdr.uLegacyFileFormat = SARE_ID_FILE_OLD;
	SaReHdr.uBuildVer = VER_BUILD;
	SaReHdr.uRevision =  VER_FEATURES;

	// to make valgrind happy
	memset(SaReHdr.Reserved, 0, sizeof(SaReHdr.Reserved));

	m_HeaderSize = 0;

	if(!SaReFileWriteHeader(m_file, &SaReHdr))
	{
		WRITE_TRACE(DBG_FATAL, "SaReFileWriteHeader Failed.");
		return -1;
	}
	return 0;
}

bool CSRFile::AllocateDataBuf(UINT uReqieredBytes)
{
	bool bIsOk = true;
	try
	{
		m_uBufLen = 0;
		delete[] m_pFileBuf;
		m_pFileBuf = new char[uReqieredBytes];
		memset(m_pFileBuf,0,uReqieredBytes);
		m_uBufLen = uReqieredBytes;
	}
	catch (...)
	{
		WRITE_TRACE(DBG_FATAL, "Unable to allocate memory");
		bIsOk = false;
	}
	return bIsOk;
}

UINT CSRFile::CompleteHeader()
{
	m_HeaderSize = SaReFileGetHeaderSize(m_file);
	WRITE_TRACE(DBG_FATAL, "CompleteHeader HeaderSize=0x%x", m_HeaderSize);
	return m_HeaderSize;
}

INT	CSRFile::WriteOption( UINT uOptId, void *pBuffer, UINT uOptLen )
{
	if( pBuffer && !SaReFileWriteOption(m_file, uOptId, pBuffer, uOptLen) )
	{
		WRITE_TRACE(DBG_FATAL, "SaReFileWriteOption Failed. OptId 0x%x uOptLen %u Data ptr %p",
			uOptId, uOptLen, pBuffer);
		return -1;
	}
	return 0;
}

INT	CSRFile::ReWriteOption(	UINT uOptId, void *pBuffer,	UINT uOptLen)
{
	UINT uActualLen;
	quint64 OptionOffset;
	quint64 filePosSaved;

	if(NULL == pBuffer || 0 == uOptLen){

		return 0;
	}
	filePosSaved = m_file.pos();

	uActualLen = SaReFileFindOption(m_file, uOptId, OptionOffset);
	if (0 == uActualLen || uOptLen != uActualLen){
		WRITE_TRACE(DBG_FATAL, "CSRFile::ReWriteOption Failed. OptId 0x%x uOptLen %u Actual %u",
			uOptId, uOptLen, uActualLen);
		m_file.seek(filePosSaved);
		return -1;
	}

	m_file.seek(OptionOffset);

	uActualLen = m_file.write((char*)pBuffer, uOptLen);
	if(uActualLen != uOptLen){
		WRITE_TRACE(DBG_FATAL, "CSRFile::ReWriteOption Failed. OptId 0x%x uOptLen %u Actual %u",
			uOptId, uOptLen, uActualLen);
		m_file.seek(filePosSaved);
		return -1;
	}

	m_file.seek(filePosSaved);
	return 0;
}


UINT CSRFile::ReadOption( UINT uOptId, void *pBuffer, UINT* puOptLen )
{
	return SaReFileReadOption(m_file, uOptId, pBuffer, puOptLen);
}

UINT
	CSRFile::GetOptionSize(
		UINT uOptId
	)
{
	quint64 pos;
	return SaReFileFindOption(m_file, uOptId, pos);
}


char * CSRFile::GetFileBuffer()
{
	return m_pFileBuf;
}

UINT CSRFile::GetBufSize()
{
	return m_uBufLen;
}

bool CSRFile::FlushData()
{
	PSARE_FILE_ITEM pFileFI = (PSARE_FILE_ITEM)m_pFileBuf;

	m_file.seek(m_HeaderSize);

	if(pFileFI->uLength > m_uBufLen){

		return false;
	}

	pFileFI->iID = CalcChecksum();

	if(pFileFI->uLength != m_file.write((const char*)pFileFI, pFileFI->uLength)){
		return false;
	}

	m_file.flush();
	return true;
}

#define AddCheckItem4(fuItem,fpItem,fpCheckSum)											\
	{																							\
		fuItem = 0x03432383;																	\
																								\
		fuItem += *(UINT *)(fpItem);															\
		fuItem += ((*(UINT *)(fpItem)) & 0xff000000) >> 24;										\
		fuItem += ((*(UINT *)(fpItem)) & 0x00ff0000) >> 7;										\
		fuItem += ((*(UINT *)(fpItem)) & 0x0000ff00) << 8;										\
		fuItem += ((*(UINT *)(fpItem)) & 0x000000ff) << 24;										\
																								\
		fuItem += (*(UINT *)(fpItem)) << 16;													\
		fuItem += (*(UINT *)(fpItem)) >> 14;													\
		*(fpCheckSum) = *(fpCheckSum) + fuItem;													\
		*(fpCheckSum) += (*(fpCheckSum) << 16) + (*(fpCheckSum) >> 11);							\
	}

#define AddCheckItem(fuSizeItem,fpItem,fpCheckSum)												\
	{																							\
		UINT uItem;																				\
		UINT i;																					\
		UINT uShif;																				\
		uItem = 0x02020101;																		\
		if ( fuSizeItem == 4 )																	\
		{																						\
			AddCheckItem4(uItem,fpItem,fpCheckSum);										\
		}																						\
		else																					\
		for ( i = 0; i < fuSizeItem; i++ )														\
		{																						\
			if ( i == 0 || i == 1 )																\
			{																					\
				uShif = (i == 0 ? 24 : 8);														\
				uItem += (UINT(*((UCHAR *)(fpItem)+i) & 0xff)) << uShif;						\
			}																					\
			else																				\
			if ( i == 2 )																		\
			{																					\
				uShif = 17;																		\
				uItem += (UINT(*((UCHAR *)(fpItem)+i) & 0xff)) << uShif;						\
			}																					\
		}																						\
																								\
		uItem += (*(fpCheckSum) >> 7) + (*(fpCheckSum) << 19);									\
		uItem += (uItem << 16) + (uItem >> 16);													\
		*(fpCheckSum) = *(fpCheckSum) + uItem;													\
		*(fpCheckSum) += *(fpCheckSum) << 16;													\
		*(fpCheckSum) += *(fpCheckSum) >> 11;													\
	}

UINT CSRFile::CalcChecksum()
{
	UINT uNumRead, uDataSize, uItem, uRest, uOffsetLow, i;
	UINT uCheckSum = 0;
	UINT *puData;
	PSARE_FILE_ITEM pFileFI = (PSARE_FILE_ITEM)m_pFileBuf;

	uDataSize = pFileFI->uLength;

	uNumRead = uDataSize;
	uNumRead = uNumRead - 4;
	uNumRead = (uNumRead / sizeof(UINT)) * sizeof(UINT);

	// skip checksum field
	uOffsetLow = 4;

	if(uNumRead >= 4){

 		puData = (UINT *)(m_pFileBuf+uOffsetLow);
		i = 0;

		do{

			AddCheckItem4(uItem, puData + i, &uCheckSum);
			i++;
		}while ( i*sizeof(UINT) < uNumRead);

		uOffsetLow += uNumRead;
	}

	// Calculate last bytes
	uRest = uDataSize - uOffsetLow;
	if (uRest && uRest < 4){
		puData = (UINT *)(m_pFileBuf+uOffsetLow);
		AddCheckItem(uRest,puData,&uCheckSum);
	}
	return uCheckSum;
}

UINT CSRFile::GetVersion(UINT* pMajor)
{
	if(pMajor)
		*pMajor = m_VersionMajor;

	return m_VersionMinor;
}

bool CSRFile::IsCompatible()
{
	return SAV_FILE_VERSION_HARD == m_VersionMajor && SAV_FILE_VERSION_SOFT >= m_VersionMinor;
}

bool CSRFile::VerifyFile()
{
	SARE_FILE_HDR SaReHdr;
	quint64 fileSz;
	PSARE_FILE_ITEM pFileFI = (PSARE_FILE_ITEM)m_pFileBuf;

	m_HeaderSize = SaReFileGetHeaderSize(m_file);

	SaReFileReadHeader(m_file, &SaReHdr);

	m_VersionMajor = 0;
	m_VersionMinor = 0;

	if(m_HeaderSize && SARE_FILE_MAGIC_NUMBER == SaReHdr.uSaReMagicNum){

		m_VersionMajor = SaReHdr.uMajorVersion;
		m_VersionMinor = SaReHdr.uMinorVersion;

		//
		// The file has a header with a recognized format
		//
		if(!IsCompatible()){
			//
			// The file is not compatible with current version
			//
			WRITE_TRACE(DBG_FATAL,
				"Incompatible version. Detected version= 0x%x.0x%x. "
				"Expected version= 0x%x.0x%x.  Header size=0x%x",
				SaReHdr.uMajorVersion, SaReHdr.uMinorVersion,
				SAV_FILE_VERSION_HARD, SAV_FILE_VERSION_SOFT,
				m_HeaderSize
				);
			return false;
		}

		WRITE_TRACE(DBG_FATAL,
			"Detected version= 0x%x.0x%x. Current version= 0x%x.0x%x. Header size=0x%x",
			SaReHdr.uMajorVersion, SaReHdr.uMinorVersion,
			SAV_FILE_VERSION_HARD, SAV_FILE_VERSION_SOFT,
			m_HeaderSize
			);
		if(SaReHdr.uMinorVersion >= 0x3001a){
			WRITE_TRACE(DBG_FATAL,
				"Saved with build %u.%u", SaReHdr.uBuildVer, SaReHdr.uRevision);
		}

	}

	fileSz = m_file.size();
	m_file.seek(m_HeaderSize);

	if(0 == fileSz || m_HeaderSize >= (UINT)fileSz){

		WRITE_TRACE(DBG_FATAL,
			"File is empty %u.%u", (UINT)fileSz, m_HeaderSize);
		return false;
	}

	fileSz -= m_HeaderSize;
	if((quint64)m_uBufLen < fileSz){

		WRITE_TRACE(DBG_FATAL,
			"Invalid buffer sizes %u.%llu", m_uBufLen, fileSz);
		return false;
	}

	if(fileSz != (quint64)m_file.read(m_pFileBuf, fileSz)){

		WRITE_TRACE(DBG_FATAL,
			"Reading failed %u", (UINT)fileSz);
		return false;
	}

	if((quint64)pFileFI->uLength != fileSz){

		WRITE_TRACE(DBG_FATAL,
			"Invalid file format %u %u", pFileFI->uLength, (UINT)fileSz);
		return false;
	}

	return true;
}

bool CSRFile::VerifyChecksum()
{
	SARE_FILE_HDR SaReHdr;
	PSARE_FILE_ITEM pFileFI = (PSARE_FILE_ITEM)m_pFileBuf;
	UINT uCurrentCheckSum = CalcChecksum();

	SaReFileReadHeader(m_file, &SaReHdr);

	if ( uCurrentCheckSum != pFileFI->iID ){

		bool bIgnore = false;

		WRITE_TRACE(DBG_FATAL, "Stored checksum=0x%x. Calculated checksum=0x%x", pFileFI->iID, uCurrentCheckSum);

		switch(SaReHdr.uMinorVersion){
		case 0x30010:
			// mnestratov@
			// Here we have the only exception if soft version is equal to 0x30010
			// since we have changed reading mechanism of the file that leaded to new checksums for the same
			// files. It was a bug related to different file lengths
			// when checksum was constructed and after then checked.
			//
			bIgnore = true;
			break;
		case 0x30013:
		case 0x30014:
		case 0x30015:
		case 0x30016:
		case 0x30017:
			if(0 == pFileFI->iID){
				//
				// Another great algorithm property.
				//
				bIgnore = true;
			}
			break;

		default:
			break;
		}

		if(!bIgnore){
			WRITE_TRACE(DBG_FATAL, "The saved state is corrupted.");
			return false;
		}
	}

	return true;
}
