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

#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Logging/Logging.h>
#include "SaReShared.h"
#include "sarefile.h"

#ifdef _WIN_
// warning C4793: 'vararg' : causes native code generation for function 'int LOG_MESSAGE(int,...)'
#pragma warning( disable : 4793 )
#define snprintf _snprintf
#endif

//
//  SARE *.sav file format description
//
//
//  1. General *.sav file scheme.
//     All data are grouped into subsystems.
//     There are two major data groups(subsystems): MONITOR and APPLICATION.
//     These two data groups at the same time are the data for main FILE group.
//     All groups are organized as a hierarchical tree.
//     Offsets are from the beginning of the file with the only exception
//     to record with TypeOpenTagSubSys type. FI - file item.
//
//                                  _____________________________________
//          |                      |                                     |
//    FI    |    FI       |      FI.uOffs   |                      |     \/
// FILE HDR | Monitor HDR | Application HDR |    Monitor data      | Application data
//          |    FI.uOffs |                 |                      |
//                   |                        ^
//                   |________________________|
//
//
//  2. Single group(subsystem) layout.
//
//           ____________________________
//           iID  ......................|  Not used
//           iType=TypeOpenTagSubSys    |  Start of the subsystem
//           uOffs......................|  The size of descriptors
//           uLength....................|  Not used
//           ___________________________|
//                                      |
//                    ...               |  either  TypeVar or TypeSubSys
//           ___________________________|
//           iID  ......................|  Data sequence number
//     e.g.  iType=TypeVar              |  Type: actual data
//  ---------uOffs......................|  Where the data starts (relative to the beginning of the file)
//  |        uLength....................|  Length of the data
//  |        ___________________________|
//  |        iID  ......................|  Subsystem sequence number
//  |  e.g.  iType=TypeSubSys           |  Type: subsystem
//  |    ----uOffs......................|  Where this subsystem starts (relative to the beginning of the file)
//  |    |   uLength....................|  Length of the subsystem including all data and descriptors
//  |    |   ___________________________|
//  |    |                              |
//  |    |            ...               |
//  |    |   ___________________________|
//  |    |   iID  ......................|  Not used
//  |    |   iType=TypeCloseTagSubSys   |  End of the subsystem
//  |    |   uOffs......................|  Not used
//  |    |   uLength....................|  Not used
//  |    |   ___________________________|
//  |    |                              |
//  -----^--> Actual data               |
//       |   ___________________________|
//       |                              |
//        --> TypeOpenTagSubSys         | --
//           ___________________________|   |
//                                      |   |
//            TypeVar or TypeSubSys     |   |
//           ___________________________|   |
//                                      |   |
//                    ...               |   | Another
//           ___________________________|   | subsystem
//                                      |   |
//            TypeCloseTagSubSys        |   |
//           ___________________________|   |
//                                      |   |
//                 Data                 |   |
//           ___________________________|   |
//                                      |   |
//                 Data                 |---
//           ___________________________|
//
//

#define SARE_FI_BOUND 0x10
#define ROUND_UP(val) (((val)&(SARE_FI_BOUND-1)) ? ((val) + SARE_FI_BOUND - ((val)&(SARE_FI_BOUND-1))) : val)

BOOL bx64tox32 = FALSE;
BOOL bx32tox64 = FALSE;
ULONG_PTR* puObsoleteDr0 = NULL;

UINT g_uSaReVerboseMask = 0;
static SARE_CUSTOM_ACTIVITY g_sareCustom;

static UINT DummyData;
static SARE_REC DummyRec = {
	SareIgnore,
	&DummyData,
	sizeof(DummyData),
	0,
	NULL,
	0,0,
	NULL,
	"DummyRec",
	0
};

typedef struct _SARE_ID2STR{
	UINT uId;
	const char* strId;
}SARE_ID2STR, *PSARE_ID2STR;

#define SARE_MAKE_ID2STR(id) {(UINT) id, #id}

static SARE_ID2STR g_sareFItypes[] =
{
	{0, "FIEmpty"},
	SARE_MAKE_ID2STR(TypeFile),
	SARE_MAKE_ID2STR(TypeSubSys),
	SARE_MAKE_ID2STR(TypeOpenTagSubSys),
	SARE_MAKE_ID2STR(TypeCloseTagSubSys),
	SARE_MAKE_ID2STR(TypeVar),
	SARE_MAKE_ID2STR(IDFile),
	{(UINT)-1, NULL},
};

static SARE_ID2STR g_sareDECLAREtypes[] =
{
	SARE_MAKE_ID2STR(SubSysStart),
	SARE_MAKE_ID2STR(LastElem),
	SARE_MAKE_ID2STR(Field),
	SARE_MAKE_ID2STR(Field_Arr),
	SARE_MAKE_ID2STR(ClassField),
	SARE_MAKE_ID2STR(ClassField_Arr),
	SARE_MAKE_ID2STR(ClassFuncPtr),
	SARE_MAKE_ID2STR(ClassFieldPtrPtr),
	SARE_MAKE_ID2STR(SareQString),
	SARE_MAKE_ID2STR(RegisterFuncPtr),
	SARE_MAKE_ID2STR(SubSystem),
	SARE_MAKE_ID2STR(ArraySubSystem),
	SARE_MAKE_ID2STR(CustomSubSystem),
	SARE_MAKE_ID2STR(SareCallback),
	SARE_MAKE_ID2STR(SareClassCallback),
	SARE_MAKE_ID2STR(SareClassCallbackEx),
	SARE_MAKE_ID2STR(ClassCustomSubSystem),
	SARE_MAKE_ID2STR(SareIgnore),
	SARE_MAKE_ID2STR(DynStructField),
	SARE_MAKE_ID2STR(ArraySingleField),
	{(UINT)-1, NULL},
};

static const char *sareGetStrById(UINT id, PSARE_ID2STR pArr)
{
	while(pArr->strId){
		if(id == pArr->uId)
			return pArr->strId;
		pArr++;
	}
	return "Unknown";
}

//////////////////////////////////////////////////////////////////////////
//
// Save & restore common functions
//
//////////////////////////////////////////////////////////////////////////

bool SaReRecIsData(PSARE_REC pRec, UINT arrayIndex)
{
	switch(pRec->iType){
	case SareCallback:
	case SareClassCallback:
	case SareClassCallbackEx:
	case RegisterFuncPtr:
		return false;
	case ArraySingleField:

		if(0 != arrayIndex){

			return false;
		}
		break;

	default:
		break;
	}

	return true;
}

void SaRePrintData(const char *prefixStr, PSARE_REC pRec, char* pStart, PSARE_FILE_ITEM pFI, UINT level)
{
	UINT uCurOff = (UINT)(ULONG_PTR)((char*)pFI - pStart);
	WRITE_TRACE(DBG_FATAL,"%s ========================", prefixStr);
	WRITE_TRACE(DBG_FATAL,"-------------------- %u", level);
	WRITE_TRACE(DBG_FATAL,"Current offset: 0x%08x", uCurOff);
	WRITE_TRACE(DBG_FATAL,"FileType:       0x%08x, %s", pFI->iType, sareGetStrById(pFI->iType,g_sareFItypes));
	WRITE_TRACE(DBG_FATAL,"iID:            0x%08x", pFI->iID);
	WRITE_TRACE(DBG_FATAL,"uLength:        0x%08x", pFI->uLength);
	WRITE_TRACE(DBG_FATAL,"uOffs:          0x%08x", pFI->uOffs);
	WRITE_TRACE(DBG_FATAL,"iType:          0x%08x, %s", pRec->iType, sareGetStrById(pRec->iType,g_sareDECLAREtypes));
	WRITE_TRACE(DBG_FATAL,"uSize:          0x%08x", pRec->uSize);
	WRITE_TRACE(DBG_FATAL,"iOff:           0x%08x", pRec->iOff);
	WRITE_TRACE(DBG_FATAL,"puSize:         %p", pRec->puSize);
	WRITE_TRACE(DBG_FATAL,"name:           %s", pRec->pItemName);
	WRITE_TRACE(DBG_FATAL,"--------------------- %u", level);
}
#define SARE_DUMP_PORTION 16

void SaReDumpData(PVOID _p, ULONG size)
{
	unsigned char *p = (unsigned char *)_p;
	ULONG i,j,whole,rest;
	char tmpstr[10];
	char datastr[200];

	whole = size/SARE_DUMP_PORTION;
	rest = size%SARE_DUMP_PORTION;
	for(i = 0;i<whole;i++){
		datastr[0] = 0;
		for(j = 0;j<SARE_DUMP_PORTION;j++){
			snprintf(tmpstr, sizeof(tmpstr), "%02x ", (p + i*SARE_DUMP_PORTION)[j]);
			strncat(datastr, tmpstr, 3);
		}
		for(j = 0;j<SARE_DUMP_PORTION;j++){
			snprintf(tmpstr, sizeof(tmpstr), "%c", (p + i*SARE_DUMP_PORTION)[j]);
			strncat(datastr, tmpstr, 1);
		}
		WRITE_TRACE(DBG_FATAL,"data [%04u-%04u]: %s",
			i*SARE_DUMP_PORTION,
			(i + 1)* SARE_DUMP_PORTION,
			datastr
			);
	}

	if(rest){

		datastr[0] = 0;
		for(j = 0;j<rest;j++){
			snprintf(tmpstr, sizeof(tmpstr), "%02x ", (p + i*SARE_DUMP_PORTION)[j]);
			strncat(datastr, tmpstr, 3);
		}

		for(j = 0;j<rest;j++){
			snprintf(tmpstr, sizeof(tmpstr), "%c", (p + i*SARE_DUMP_PORTION)[j]);
			strncat(datastr, tmpstr, 1);
		}

		WRITE_TRACE(DBG_FATAL,"data [%04u-%04u]: %s",
			i*SARE_DUMP_PORTION,
			i*SARE_DUMP_PORTION + rest,
			datastr
			);
	}
	WRITE_TRACE(DBG_FATAL,"---------------------");
}

static PVOID SaReGetOptArrPtr(PSARE_REC pArrRec, UINT arrayIndex)
{
	PVOID pCurDataAddr;
	SareGetPtrType pGetPtr = (SareGetPtrType)pArrRec->puSize;

	if(NULL == pGetPtr){

		WRITE_TRACE(DBG_FATAL,"Invalid fArrSysPtrOpt option for item %s[%u], pGetPtr is NULL, line=%u",
			pArrRec->pItemName, arrayIndex, __LINE__);
		return NULL;
	}

	pCurDataAddr = (*pGetPtr)(arrayIndex);
	if(NULL == pCurDataAddr){

		if(g_uSaReVerboseMask){
			WRITE_TRACE(DBG_FATAL,"Skipping absent item %s[%u], line=%u",
				pArrRec->pItemName, arrayIndex, __LINE__);
		}
	}
	return pCurDataAddr;
}

bool SaReProcessCallbacks(PSARE_REC pFisrtRec, PSARE_REC pEndRec, UINT actionMask, UINT level)
{
	int iRetCode = 0;
	void *pObj = NULL;

	for(PSARE_REC pRec = pFisrtRec;pRec <= pEndRec && LastElem != pRec->iType; pRec++){

		switch(pRec->iType){
		case SubSystem:
		case ArraySubSystem:

			if(!(pRec->uFlags& actionMask))
				continue;

			if(!SaReProcessCallbacks(
					(PSARE_REC)pRec->pAddr,
					(PSARE_REC)pRec->pAddr + (int)pRec->uSize+1,
					actionMask,
					level + 1))
				return false;

			continue;

		case SareCallback:
		case SareClassCallback:
		case SareClassCallbackEx:
			break;
		default:

			continue;
		}

		if(!(pRec->uFlags & actionMask))
			continue;

		if(g_uSaReVerboseMask){
			WRITE_TRACE(DBG_FATAL, "Calling %s...",pRec->pItemName);
		}

		switch(pRec->iType){
		case SareCallback:
			iRetCode = ((SARE_CALLBACK_PTR)(pRec->pAddr))();
			break;
		case SareClassCallback:
		case SareClassCallbackEx:

			if(NULL == pFisrtRec->pAddr){

				if(g_uSaReVerboseMask){
					WRITE_TRACE(DBG_FATAL, "Skipping %s ... Class pointer is NULL for item %s",
						pRec->pItemName, pFisrtRec->pItemName);
				}
				return true;
			}

			if ( pFisrtRec->uFlags & fPtrObj )
				pObj = (void *)pFisrtRec->pAddr;
			else if ( pFisrtRec->uFlags & fPtrPtrObj )
				pObj = (void *)(*(void **)pFisrtRec->pAddr);

			if (!pObj ){
				//
				// error or warning?
				//
				continue;
			}

			if(SareClassCallback == pRec->iType)
				iRetCode = ((ClassCallBackSuspendResume)(pRec->pAddr))(pObj);
			else{
				iRetCode = ((ClassCallBackSuspendResumeEx)(pRec->pAddr))(pObj,actionMask);
			}
		default:; //to make gcc happy
		}

		if(g_uSaReVerboseMask){
			WRITE_TRACE(DBG_FATAL, "Calling %s... done. Ret=0x%x",pRec->pItemName, iRetCode);
		}

		if ( iRetCode ){

			WRITE_TRACE(DBG_FATAL, "Callback function %s type 0x%x failed. Error code 0x%x, line=%u",
				pRec->pItemName, pRec->iType, iRetCode,__LINE__);
			return false;
		}
	}

	return true;
}

bool SaReLoadSaveCustomData(
		void* pDataAddr,
		PSARE_REC pRec,
		PSARE_REC pEndRec,
		char *pStart,
		UINT totLen,
		PSARE_FILE_ITEM pFI,
		UINT level,
		PVOID pFuncAddr
		)
{
	bool bError;

	if(NULL == pFuncAddr)
	{
		WRITE_TRACE(DBG_FATAL, "Invalid custom descriptor %s, line=%u",
			pRec->pItemName, __LINE__);
		return false;
	}

	if((char*)pFI + sizeof(SARE_FILE_ITEM) > pStart + totLen){
		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s (%p,0x%lx,%p,0x%x), line=%u",
			pRec->pItemName, pFI, sizeof(SARE_FILE_ITEM), pStart, totLen, __LINE__);
		return false;
	}

	g_sareCustom.pDataAddr = pDataAddr;
	g_sareCustom.pRec = pRec;
	g_sareCustom.pEndRec = pEndRec;
	g_sareCustom.pStart = pStart;
	g_sareCustom.totLen = totLen;
	g_sareCustom.pFI = pFI;
	g_sareCustom.level = level;

	switch(pRec->iType){
	case CustomSubSystem:
		bError = ((SARE_CALLBACK_PTR)(pFuncAddr))();
		break;

	case ClassCustomSubSystem:

		if(NULL == pDataAddr){

			if(g_uSaReVerboseMask){
				WRITE_TRACE(DBG_FATAL, "Item %s is not initialized. Skipped while restoring/saving, line=%u",
					pRec->pItemName, __LINE__);
			}
			return true;
		}

		bError = ((ClassCallBackSuspendResume)pFuncAddr)(pDataAddr);
		break;
	default:
	    bError = true;
	}

	if(bError){

		memset(&g_sareCustom, 0 ,sizeof(g_sareCustom));
		WRITE_TRACE(DBG_FATAL, "Custom function failed pFunc=%p, %s, line=%u",
			pFuncAddr, pRec->pItemName, __LINE__);
	}

	return !bError;
}

//////////////////////////////////////////////////////////////////////////
//
// Restore part functions
//
//////////////////////////////////////////////////////////////////////////

bool SaReLoadArrayPtr(void *pWriteTo, void *pReadFrom, PSARE_REC pRec, PSARE_FILE_ITEM pFI)
{
	UINT i;

	if(pFI->uLength > pRec->uSize
	   && !bx64tox32
	){
		WRITE_TRACE(DBG_FATAL,"Data overhead. Item %s 0x%x, 0x%x, line=%u",
			pRec->pItemName, pFI->uLength, pRec->uSize, __LINE__);
		return false;
	}

	if(pFI->uLength == pRec->uSize){

		if(g_uSaReVerboseMask > 1){

			SaReDumpData(pReadFrom, pFI->uLength);
		}

		memcpy(pWriteTo, pReadFrom, pFI->uLength);
		return true;
	}

	if(pFI->uLength > pRec->uSize){

		if((pFI->uLength%pRec->uSize) || ((pFI->uLength/pRec->uSize)%2)){

			WRITE_TRACE(DBG_FATAL, "Item %s restoring from x64 monitor failed. uSize=0x%x, uLen=0x%x, line=%u",
				pRec->pItemName, pRec->uSize, pFI->uLength, __LINE__);
			return false;
		}

		for(i = 0; i<pFI->uLength/sizeof(UINT64);i++){

			((ULONG_PTR*)pWriteTo)[i] = (ULONG_PTR)((UINT64 *)pReadFrom)[i];
		}

		if(g_uSaReVerboseMask > 1){

			SaReDumpData(pWriteTo, pRec->uSize);
		}
		return true;
	}

	if((pRec->uSize%pFI->uLength) || ((pRec->uSize/pFI->uLength)%2)){

		WRITE_TRACE(DBG_FATAL, "Item %s restoring from x32 monitor failed. uSize=0x%x, uLen=0x%x, line=%u",
			pRec->pItemName, pRec->uSize, pFI->uLength, __LINE__);
		return false;
	}
	for(i = 0; i<pFI->uLength/sizeof(UINT);i++){

		((ULONG_PTR*)pWriteTo)[i] = ((UINT *)pReadFrom)[i];
	}

	if(g_uSaReVerboseMask > 1){

		SaReDumpData(pWriteTo, pRec->uSize);
	}

	return true;
}

bool SaReLoadQString(PSARE_REC pRec, PSARE_FILE_ITEM pFI, char *pStart, UINT totLen)
{
	PSARE_REC pSizeRec;
	PSARE_FILE_ITEM pFISize;
	int *pSize;
	QString		*pQString;
	QByteArray	qbData;
	char *pReadFrom;

	if(fSpecialSize & pRec->uFlags){
		//
		// We will get here a step later
		//
		return true;
	}

	if(!(fSpecialDatum & pRec->uFlags)){

		WRITE_TRACE(DBG_FATAL, "Unxpected data type. Item %s, flag=0x%x, line=%u",
			pRec->pItemName, pRec->uFlags, __LINE__);
		return false;
	}

	//
	// the previous record have to be fSpecialSize
	//
	pSizeRec = pRec-1;
	if(!(fSpecialSize & pSizeRec->uFlags)){

		WRITE_TRACE(DBG_FATAL, "Unxpected prev data type. Item %s, flag=0x%x, line=%u",
			pRec->pItemName, pSizeRec->uFlags, __LINE__);
		return false;
	}

	pFISize = pFI-1;
	pReadFrom = pStart + pFI->uOffs;
	pSize = (int*)(pStart + pFISize->uOffs);

	if(pReadFrom + *pSize > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s (%p,0x%x,%p,0x%x), line=%u",
			pRec->pItemName, pReadFrom, *pSize, pStart, totLen, __LINE__);
		return false;
	}

	qbData = QByteArray(pReadFrom, *pSize);

	pQString = (QString*)pRec->pAddr;
	*pQString = UTF8SZ_2QSTR(qbData.data(), qbData.size());

	return true;
}

bool SaReGetFuncPtrById(
			VOID *pWriteTo,
			PSARE_REC pRec,
			VOID *pReadFrom,
			UINT uLength
			)
{
	UINT i;
	UINT uItems = pRec->uSize;

	if(uItems*sizeof(UINT) > uLength){

		WRITE_TRACE(DBG_FATAL, "Function ID assigning failed. RecSize=0x%x, Len=0x%x , line=%u",
			pRec->uSize, uLength, __LINE__);
		return false;
	}

	for(i = 0;i<uItems;i++){

		PSARE_REC pFuncRec = SaReFindFuncPtrById(
								pRec,
								((UINT*)pReadFrom)[i],
								&((void**)pWriteTo)[i]);
		if (!pFuncRec)
		{
			WRITE_TRACE(DBG_FATAL, "Function ID assigning failed. Name=%s, FuncId=%p, line=%u",
						pRec->pItemName,(void*)((ULONG_PTR*)pReadFrom)[i], __LINE__);
			return false;
		}
		else if (g_uSaReVerboseMask)
		{
			WRITE_TRACE(DBG_FATAL, "SaReGetFuncPtrById Function %s (%p) is set for %s[%05u] item, line=%u",
				pFuncRec->pItemName, pFuncRec->pAddr, pRec->pItemName, i, __LINE__);
		}
	}

	return true;
}

bool SaReLoadData(
		void* pDataAddr,
		PSARE_REC pRec,
		PSARE_REC pEndRec,
		PSARE_FILE_ITEM pFI,
		char *pStart,
		UINT totLen,
		UINT arrayIndex,
		UINT dataId
		)
{
	char *pReadFrom;
	void *pWriteTo = NULL;
	UINT uSize = -1;
	UINT uCopySize;
	SareFieldAfterRestoreFunc pAfterRestore = (SareFieldAfterRestoreFunc)pRec->pRestoreCb;

	(void)pEndRec;

	pReadFrom = pStart + pFI->uOffs;
	if(pReadFrom + pFI->uLength > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%x,%p,0x%x), line=%u",
			pRec->pItemName, arrayIndex, pReadFrom, pFI->uLength , pStart, totLen, __LINE__);
		return false;
	}

	switch(pRec->iType){
	case Field:
		uSize = pRec->uSize;
		pWriteTo = pRec->pAddr;
		break;
	case Field_Arr:
		pWriteTo = pRec->pAddr;
		return SaReLoadArrayPtr(pWriteTo, pReadFrom, pRec, pFI);

	case SareQString:
		return SaReLoadQString(pRec, pFI, pStart, totLen);
	case ClassField:
	{
		SareCustomRestoreType pRestoreFunc = (SareCustomRestoreType)pRec->pAddrResEx;
		pWriteTo = (char*)pDataAddr + (UINT)pRec->iOff;
		uSize = pRec->uSize;
		if(NULL == pRestoreFunc){

			break;
		}
		return pRestoreFunc(pWriteTo, pReadFrom, pRec, pFI);
	}
	case ClassField_Arr:
		pWriteTo = (char*)pDataAddr + (UINT)pRec->iOff;
		return SaReLoadArrayPtr(pWriteTo, pReadFrom, pRec, pFI);

	case ClassFieldPtrPtr:
		pWriteTo = *(void**)((char*)pDataAddr + (UINT)pRec->iOff);
		uSize = pRec->uSize;
		break;

	case ArraySingleField:

		if(0 == arrayIndex){
			SareRestoreFpuType pRestoreFpuFunc = (SareRestoreFpuType)pRec->pAddrResEx;
			if(NULL == pRestoreFpuFunc){

				uSize = pRec->uSize;
				pWriteTo = pRec->pAddr;
				break;
			}

			return pRestoreFpuFunc(pRec, pReadFrom, pFI->uLength);
		}

	case SareIgnore:

		if(g_uSaReVerboseMask){
			WRITE_TRACE(DBG_FATAL,"%s is skipped %s[%u], FileLen=0x%x id=%u, i=%u",
				sareGetStrById(pRec->iType,g_sareDECLAREtypes),
				pRec->pItemName, arrayIndex, pFI->uLength, pFI->iID, dataId);
		}
		return true;

	case DynData:
	{
		SareGetDynDataForRestoreFunc pGetPtr = (SareGetDynDataForRestoreFunc)pRec->pAddrResEx;
		if(NULL == pGetPtr){

			WRITE_TRACE(DBG_FATAL,"Invalid DynData for item %s[%u], pGetPtr is NULL, line=%u",
				pRec->pItemName, arrayIndex, __LINE__);
			return false;
		}

		pGetPtr(pDataAddr, &pWriteTo, pFI->uLength, &uSize);
		if(NULL == pWriteTo || 0 == uSize){

			if(g_uSaReVerboseMask){
				WRITE_TRACE(DBG_FATAL,"DynData is skipped item %s[%u], line=%u",
					pRec->pItemName, arrayIndex, __LINE__);
			}
			return true;
		}
	}
		break;

	case DynStructField:
	case DynStructOffsetField:
	{
		SareGetPtrType pGetPtr = (SareGetPtrType)pRec->pAddr;

		if(NULL == pGetPtr){

			WRITE_TRACE(DBG_FATAL,"Invalid DynStructField for item %s[%u], pGetPtr is NULL, line=%u",
				pRec->pItemName, arrayIndex, __LINE__);
			return false;
		}

		uSize = pRec->uSize;
		if(DynStructOffsetField == pRec->iType)
			pWriteTo = (*pGetPtr)(pRec->iOff);
		else if (DynStructField == pRec->iType)
			pWriteTo = (*pGetPtr)(arrayIndex);
		else
			pWriteTo = NULL;
		if(NULL == pWriteTo){

			if(g_uSaReVerboseMask){
				WRITE_TRACE(DBG_FATAL,"DynStructField is skipped item %s[%u], line=%u",
					pRec->pItemName, arrayIndex, __LINE__);
			}
			return true;
		}
		break;
	}

	case ClassFuncPtr:

		pWriteTo = (char*)pDataAddr + (UINT)pRec->iOff;
		return SaReGetFuncPtrById(pWriteTo, pRec, pReadFrom, pFI->uLength);

	case LastElem:
	case CustomSubSystem:
	case ClassCustomSubSystem:
	case SubSystem:
	case ArraySubSystem:
		WRITE_TRACE(DBG_FATAL,"Item %s[%u]. Ordinary data detected and skipped. Type %s=0x%x. line=%u",
			pRec->pItemName, arrayIndex, sareGetStrById(pRec->iType,g_sareDECLAREtypes), pRec->iType, __LINE__);
		return true;
	default:
		WRITE_TRACE(DBG_FATAL,"Item %s[%u]. Unexpected var type %s=0x%x, line=%u",
			pRec->pItemName, arrayIndex, sareGetStrById(pRec->iType,g_sareDECLAREtypes), pRec->iType, __LINE__);
		return false;
	}

	if(pFI->uLength > uSize){

		if(!bx64tox32 && !bx32tox64 &&
			0 == arrayIndex &&
			puObsoleteDr0 == pWriteTo){

			if(sizeof(UINT) == uSize && pFI->uLength == 2*uSize){

				//
				// Workaround x64 -> x32 issue
				//
				bx64tox32 = true;
				WRITE_TRACE(DBG_FATAL,"x64 -> x32 load detected");

			}else if(sizeof(UINT64) == uSize && 2*pFI->uLength == uSize){

				//
				// Workaround x32 -> x64 issue
				//
				bx32tox64 = true;
				WRITE_TRACE(DBG_FATAL,"x32 -> x64 load detected");
			}
		}

		if(!bx64tox32){
			WRITE_TRACE(DBG_FATAL,"Data overhead. Item %s[%u] 0x%x, 0x%x, line=%u",
				pRec->pItemName, arrayIndex, pFI->uLength, uSize, __LINE__);
			return false;
		}
	}

	uCopySize = MIN(pFI->uLength, uSize);

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL,"DATA loading: #=0x%08x, fid=0x%08x %3u(%3u) bytes copied to %p: '%s[%u]'",
			dataId, pFI->iID, uCopySize, pFI->uLength, pWriteTo, pRec->pItemName, arrayIndex);
	}

	if(dataId != (pFI->iID&0xffffff)){

		WRITE_TRACE(DBG_FATAL,"Data ID mistiming. Item %s[%u] 0x%x, 0x%x, line=%u",
			pRec->pItemName, arrayIndex, dataId, (pFI->iID&0xffffff), __LINE__);
		return false;
	}

	memcpy(pWriteTo, pReadFrom, uCopySize);
	if(uSize > uCopySize)
		memset((char*)pWriteTo+uCopySize,0,uSize - uCopySize);

	if(g_uSaReVerboseMask > 1){

		SaReDumpData(pWriteTo, uSize);
	}

	if(pAfterRestore){

		if(g_uSaReVerboseMask){
			WRITE_TRACE(DBG_FATAL,"DATA after load calling: #=0x%08x, fid=0x%08x %3u(%3u) bytes copied to %p: '%s[%u]'",
				dataId, pFI->iID, uCopySize, pFI->uLength, pWriteTo, pRec->pItemName, arrayIndex);
		}
		pAfterRestore(arrayIndex, pWriteTo, uSize);
	}

	return true;
}

BOOL CustomSubSystemStartRead()
{
	if(NULL == g_sareCustom.pFI || NULL == g_sareCustom.pRec){

		WRITE_TRACE(DBG_FATAL,"Invalid custom data. NULL ptr, line=%u", __LINE__);
		return false;
	}

	if(TypeOpenTagSubSys != g_sareCustom.pFI->iType){

		WRITE_TRACE(DBG_FATAL,"Invalid custom data. Item %s 0x%x, line=%u",
			g_sareCustom.pRec->pItemName, g_sareCustom.pFI->iType, __LINE__);
		return false;
	}

	if(g_sareCustom.started){
		WRITE_TRACE(DBG_FATAL,"Nested custom systems are not supported. Item %s 0x%x, line=%u",
			g_sareCustom.pRec->pItemName, g_sareCustom.pFI->iType, __LINE__);
		return false;
	}

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL, "%s: start loading",g_sareCustom.pRec->pItemName);
	}

	g_sareCustom.curItem = 0;
	g_sareCustom.started = true;
	return true;
}

BOOL CustomSubSystemContinueRead(void *pAddr, UINT uSize, BOOL& bFound, UINT *puSize)
{
	char *pReadAddr;
	UINT uItems, curItem;
	char *pStart;
	UINT totLen;
	PSARE_FILE_ITEM pFI;
	PSARE_REC pRec;

	bFound = FALSE;

	if ( !pAddr )
		return true;

	if(NULL == g_sareCustom.pFI || NULL == g_sareCustom.pRec){

		WRITE_TRACE(DBG_FATAL, "Invlalid custom subsys state. NULL ptr, line=%u",__LINE__);
		return false;
	}

	pStart = g_sareCustom.pStart;
	totLen = g_sareCustom.totLen;
	pFI = g_sareCustom.pFI;
	curItem = g_sareCustom.curItem;
	pRec = g_sareCustom.pRec;

	if((char*)pFI + sizeof(SARE_FILE_ITEM) > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%lx,%p,%x), line=%u",
			pRec->pItemName, curItem, pFI, sizeof(SARE_FILE_ITEM) , pStart, totLen, __LINE__);
		return false;
	}

	uItems = pFI->uOffs/sizeof(SARE_FILE_ITEM);
	pFI++;

	if((char*)pFI + uItems*sizeof(SARE_FILE_ITEM) > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%lx,%p,0x%x), line=%u",
			pRec->pItemName, curItem, pFI, uItems*sizeof(SARE_FILE_ITEM), pStart, totLen, __LINE__);
		return false;
	}

	if(curItem >= uItems){
		WRITE_TRACE(DBG_FATAL, "Invalid index. Item %s[%u], %u, line=%u",
			pRec->pItemName, curItem, uItems, __LINE__);
		return false;
	}

	pReadAddr = (pStart + pFI[curItem].uOffs);
	if(pReadAddr + pFI[curItem].uLength > pStart + totLen){
		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%x,%p,0x%x), line=%u",
			pRec->pItemName, curItem, pReadAddr, pFI[curItem].uLength, pStart, totLen, __LINE__);
		return false;
	}

	if(pFI[curItem].uLength > uSize){
		WRITE_TRACE(DBG_FATAL, "Custom data overhead. Item %s[%u] (0x%x,0x%x), line=%u",
			pRec->pItemName, curItem, pFI[curItem].uLength, totLen, __LINE__);
		return false;
	}

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL, "Custom data loading: loading 0x%08x bytes from %p to %p '%s[%u]', line=%u",
			uSize, pReadAddr, pAddr, g_sareCustom.pRec->pItemName, curItem, __LINE__);
	}

	memcpy(pAddr, pReadAddr, uSize);

	if(g_uSaReVerboseMask > 1){

		SaReDumpData(pAddr, uSize);
	}

	g_sareCustom.curItem++;
	bFound = true;
	if ( puSize )
		*puSize = uSize;

	return true;
}

BOOL CustomSubSystemStopRead()
{
	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL, "%s: stop loading",g_sareCustom.pRec->pItemName);
	}
	memset(&g_sareCustom, 0 ,sizeof(g_sareCustom));
	return true;
}

bool
	SaReDoLoad(
		void *pDataAddr,			// the actual address of current subsystem data beginning
		PSARE_REC pRec,				// current subsystem data descriptor
		PSARE_REC pEndRec,			// the limit of current subsystem descriptor
		char *pStart,				// data storage beginning
		UINT totLen,				// data storage limit
		PSARE_FILE_ITEM pFI,	// current subsystem storage data description
		UINT level,					// current recursion level
		UINT actionMask,			// current processing stage & action
		UINT arrayIndex				// index for data declared as arrays
		)
{
	static PSARE_REC RecState[SARE_MAX_EXT_SUB_SYS];
	PSARE_FILE_ITEM pNextFI = NULL;
	PSARE_REC pNextRec = NULL;
	UINT uItems = 0;
	UINT i;

	if(level>=SARE_MAX_EXT_SUB_SYS){

		WRITE_TRACE(DBG_FATAL, "Too deep recursion. Item %s[%u], lev=0x%x",
			pRec->pItemName, arrayIndex, level);
		return false;
	}

	if((char*)pFI + sizeof(SARE_FILE_ITEM) > pStart + totLen){
		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%lx,%p,0x%x), line=%u",
			pRec->pItemName, arrayIndex, pFI, sizeof(SARE_FILE_ITEM), pStart, totLen, __LINE__);
		return false;
	}

	if(pRec > pEndRec){
		WRITE_TRACE(DBG_FATAL, "Current item is out of range. Cur=%p, End=%p %s[%u], line=%u",
			pRec, pEndRec, pEndRec->pItemName, arrayIndex, __LINE__);
		return false;
	}

	RecState[level] = pRec;

	if(g_uSaReVerboseMask){
		SaRePrintData("Loading", pRec, pStart, pFI, level);
	}

	switch(pFI->iType){
	case TypeSubSys:

		pNextFI = (PSARE_FILE_ITEM)(pStart + pFI->uOffs);

		if((char*)pNextFI + pNextFI->uLength > pStart + totLen){

			WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%x,%p,0x%x), line=%u",
				pRec->pItemName, arrayIndex, pNextFI, pNextFI->uLength, pStart, totLen, __LINE__);
			return false;
		}

		switch(pRec->iType){
		case SareIgnore:
			if(g_uSaReVerboseMask){
				WRITE_TRACE(DBG_FATAL,"Subsystem %s[%u] is ignored",pRec->pItemName, arrayIndex);
			}
			return true;
		case ArraySubSystem:
		case SubSystem:

			if(!(pRec->uFlags & actionMask)){

				//
				// skip those subsystems that were not requested
				//
				return true;
			}

			pNextRec = (PSARE_REC)pRec->pAddr;
			if(ArraySubSystem == pRec->iType)
				pEndRec = pNextRec + 2;
			else
				pEndRec = pNextRec + (int)pRec->uSize + 1;

			if(pRec->uFlags & fArrSysNested){
				// Subsystems declared as NESTED get their addresses from above systems
				pDataAddr = (char*)pDataAddr + pRec->iOffNested;
			}else if ( pNextRec->uFlags & fPtrObj ){
				pDataAddr = pNextRec->pAddr;
				if (!pDataAddr)
					return true;
			}
			else if ( pNextRec->uFlags & fPtrPtrObj ){
				pDataAddr = *(void **)pNextRec->pAddr;
				if (!pDataAddr)
					return true;
			}

			break;
		case SubSysStart:
			pNextRec = pRec;
			break;
		case ClassCustomSubSystem:
		case CustomSubSystem:

			if(!(pRec->uFlags & actionMask)){

				//
				// skip those subsystems that were not requested
				//
				return true;
			}

			return SaReLoadSaveCustomData(pDataAddr, pRec, pEndRec, pStart, totLen, pNextFI, level + 1, pRec->pAddrResEx);

		default:

			WRITE_TRACE(DBG_FATAL,"Item %s[%u], unexpected data type 0x%x, line=%u",
				pRec->pItemName, arrayIndex, pRec->iType, __LINE__);
			return false;
		}

		return SaReDoLoad(pDataAddr, pNextRec, pEndRec, pStart, totLen, pNextFI, level + 1, actionMask, arrayIndex);

	case TypeOpenTagSubSys:

		uItems = pFI->uOffs/sizeof(SARE_FILE_ITEM);
		break;

	case TypeCloseTagSubSys:
	case TypeZero:

		return true;

	default:
		WRITE_TRACE(DBG_FATAL, "Unexpected file data. Item %s[%u], type=0x%x, line=%u",
			pRec->pItemName, arrayIndex, pFI->iType, __LINE__);
		return false;
	}

	int SkippedItems = 0;
	bool bArray;
	PSARE_REC pArrRec = NULL;
	void *pCurDataAddr;

	if(level == 0 || ArraySubSystem != RecState[level-1]->iType)
		bArray = false;
	else{
		bArray = true;
		pArrRec = RecState[level-1];
	}

	for(i = 0, pNextFI = pFI + 1, pNextRec = pRec + 1;
		i<uItems;
		i++,pNextFI++
		){

		if((char*)pFI + sizeof(SARE_FILE_ITEM) > pStart + totLen){

			WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%lx,%p,0x%x), line=%u",
				pRec->pItemName, arrayIndex, pFI, sizeof(SARE_FILE_ITEM), pStart, totLen, __LINE__);
			return false;
		}

		while(i < (pNextFI->iID&0xffffff)){

			//
			// sync data with descriptor by ID
			//
			if(g_uSaReVerboseMask){

				WRITE_TRACE(DBG_FATAL, "Skipping %u item %s[%u] to sync with var=%u",
					i, pNextRec->pItemName, arrayIndex, pNextFI->iID);
				}

			pNextRec++;
			if(pNextRec > pEndRec){
				WRITE_TRACE(DBG_FATAL, "Current item is out of range. Cur=%p, End=%p %s[%u], line=%u",
					pNextRec, pEndRec, pEndRec->pItemName, arrayIndex, __LINE__);
				return false;
				}
			i++;
		}

		if(TypeVar == pNextFI->iType){

			if(g_uSaReVerboseMask){
				SaRePrintData("Data loading", pNextRec, pStart, pNextFI, level);
			}

			while(ArraySubSystem == pNextRec->iType && (pNextRec->uFlags & fOptional)){

				if(g_uSaReVerboseMask){

					WRITE_TRACE(DBG_FATAL, "Skipping %u item %s[%u] to sync with var=%u",
						i, pNextRec->pItemName, arrayIndex, pNextFI->iID);
				}

				pNextRec++;
				if(pNextRec > pEndRec){
					WRITE_TRACE(DBG_FATAL, "Current item is out of range. Cur=%p, End=%p %s[%u], line=%u",
						pNextRec, pEndRec, pEndRec->pItemName, arrayIndex, __LINE__);
					return false;
					}
				i++;
			}

			if((pArrRec && (pArrRec->uFlags & fArrSysPtrOpt))){

				if(NULL == SaReGetOptArrPtr(pArrRec, i))
					continue;
			}

			if(!SaReLoadData(pDataAddr, pNextRec, pEndRec, pNextFI, pStart, totLen, arrayIndex, i)){

				return false;
			}

			pNextRec++;
			continue;
		} // if(TypeVar == pNextFI->iType){

		if(pArrRec){

			if(pArrRec->uFlags & fArrSysPtrOpt){

				pCurDataAddr = SaReGetOptArrPtr(pArrRec, i);
				if(NULL == pCurDataAddr)
					continue;

			}else{

				//
				// pArrRec->iOff actually is sizeof array item
				//
				pCurDataAddr = (char*)pDataAddr + i*pArrRec->iOff;
			}
			arrayIndex = i;
		}else{

			pCurDataAddr = pDataAddr;
		}

		if(!SaReDoLoad(pCurDataAddr, pNextRec, pEndRec, pStart, totLen, pNextFI, level + 1, actionMask, arrayIndex)){

			return false;
		}

		if(LastElem == pNextRec->iType){

			//
			// There were some items skipped
			// and we've reached the end of declaration array
			// To sync with file structure we have to stay on last element
			// and skip file data that correspond to items that placed
			// at the end of these data scope. Those file items have to contain zeros
			//
			SkippedItems--;

			if(SkippedItems < 0 || 0 != pNextFI->iType){

				WRITE_TRACE(DBG_FATAL,"Item %s[%u], Empty data mistiming %d, 0x%x, line=%u",
					pNextRec->pItemName, arrayIndex, SkippedItems, pNextFI->iType,__LINE__);
			}

			continue;
		}

		//
		// We have to use the same descriptor in case of array declaration
		//
		if(!bArray){

			pNextRec++;
			continue;
		}

	} // for(i = 0 ...

	return true;
}

bool SaReCallbackOnly(PSARE_REC pRootRec, UINT stageMask)
{
	return SaReProcessCallbacks(pRootRec, pRootRec+2, stageMask, 0);
}


bool SaReLoadMain(PSARE_REC pRootRec, void *pBuf, UINT fileSize, UINT stageMask, bool bMonitor)
{
	bool bOk = false;
	UINT curCallMask;

	//
	// Root SARE_REC array always contains 3(three) elements
	// so end record is always &pRootRec[2] or pRootRec+2
	//

	curCallMask = stageMask & ~(fAllCallbacks & ~fBeforeRestore);

	bOk = SaReProcessCallbacks(pRootRec, pRootRec+2, curCallMask, 0);
	if(!bOk) {

		WRITE_TRACE(DBG_FATAL, "Processing callbacks before loading failed.");
		return bOk;
	}

	bOk = SaReDoLoad(
				NULL,
				pRootRec,
				pRootRec+2,
				(char*)pBuf,
				fileSize,
                (PSARE_FILE_ITEM)pBuf + (bMonitor ? 1 : 2),
				0,
				stageMask,
				0
				);
	if(!bOk){
		WRITE_TRACE(DBG_FATAL, "Loading data failed");
		return bOk;
	}

	curCallMask = stageMask & ~(fAllCallbacks & ~fAfterRestore);

	bOk = SaReProcessCallbacks(pRootRec, pRootRec+2, curCallMask, 0);
	if(!bOk) {

		WRITE_TRACE(DBG_FATAL, "Processing callbacks after loading failed");
		return bOk;
	}

	return bOk;
}

//////////////////////////////////////////////////////////////////////////
//
// Saving part functions
//
//////////////////////////////////////////////////////////////////////////

bool SaReSaveQString(PSARE_REC pRec, PSARE_FILE_ITEM pFI, char *pStart, UINT totLen, UINT dataId)
{
	PSARE_REC pSizeRec;
	int *pSize;
	QString		*pQString;
	QByteArray	qbData;
	char *pWriteTo;
	UINT uStrSize;

	if(fSpecialSize & pRec->uFlags){

		pQString = (QString*)pRec->pAddr;

		pFI->iID = dataId;
		pFI->uLength = sizeof(UINT);
		pFI->iType = TypeVar;
		pSize = (int*)(pStart + pFI->uOffs);

		if((char*)(pSize + 1) > pStart + totLen){

			WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s (%p,0x%lx,%p,0x%x)",
				pRec->pItemName, pSize, sizeof(*pSize), pStart, totLen);
			return false;
		}

		*pSize = pQString->toUtf8().size();
		return true;
	}

	if(!(fSpecialDatum & pRec->uFlags)){

		WRITE_TRACE(DBG_FATAL,"Invalid item %s, Flag=0x%x, line=%u",
			pRec->pItemName, pRec->uFlags,__LINE__);
		return false;
	}

	//
	// the previous record have to be fSpecialSize
	//
	pSizeRec = pRec-1;
	if(!(fSpecialSize & pSizeRec->uFlags)){

		WRITE_TRACE(DBG_FATAL,"Invalid prev item %s, Flag=0x%x, line=%u",
			pRec->pItemName, pSizeRec->uFlags,__LINE__);
		return false;
	}

	pWriteTo = pStart + pFI->uOffs;

	pQString = (QString*)pRec->pAddr;
	uStrSize = pQString->toUtf8().size();

	if(pWriteTo + uStrSize > pStart + totLen){
		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s (%p,0x%x,%p,0x%x)",
			pRec->pItemName, pWriteTo, uStrSize, pStart, totLen);
		return false;
	}

	pFI->iID = dataId;
	pFI->uLength = uStrSize;
	pFI->iType = TypeVar;

	memcpy(pWriteTo, pQString->toUtf8().data(), uStrSize);

	if(g_uSaReVerboseMask > 1){

		SaReDumpData(pWriteTo, uStrSize);
	}

	return true;
}

BOOL CustomSubSystemStartWrite(UINT uItems)
{
	PSARE_FILE_ITEM pFI;

	if(NULL == g_sareCustom.pFI || NULL == g_sareCustom.pRec){

		WRITE_TRACE(DBG_FATAL, "Invalid custom data state");
		return false;
	}

	if(g_sareCustom.started){
		WRITE_TRACE(DBG_FATAL,"Nested custom systems are not supported. Item %s 0x%x, line=%u",
			g_sareCustom.pRec->pItemName, g_sareCustom.pFI->iType, __LINE__);
		return false;
	}

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL, "%s: start saving",g_sareCustom.pRec->pItemName);
	}

	g_sareCustom.curItem = 0;
	g_sareCustom.started = true;

	pFI = g_sareCustom.pFI;

	pFI->uLength = (uItems+2)*sizeof(SARE_FILE_ITEM);
	pFI->iID = 0;
	pFI->iType = TypeSubSys;

	pFI = (PSARE_FILE_ITEM)(g_sareCustom.pStart + pFI->uOffs);

	if((char*)pFI + (uItems + 2)*sizeof(SARE_FILE_ITEM) > g_sareCustom.pStart + g_sareCustom.totLen){
		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s (%p,0x%lx,%p,0x%x)",
			g_sareCustom.pRec->pItemName, pFI, (uItems + 2)*sizeof(SARE_FILE_ITEM),
			g_sareCustom.pStart, g_sareCustom.totLen);
		return false;
	}

	pFI->iID = 0;
	pFI->iType = TypeOpenTagSubSys;
	pFI->uOffs = uItems*sizeof(SARE_FILE_ITEM);
	pFI->uLength = 0;

	if(uItems){

		pFI++;
		pFI->uOffs = g_sareCustom.pFI->uOffs + g_sareCustom.pFI->uLength;
	}

	return true;
}

BOOL CustomSubSystemContinueWrite(void *pAddr, UINT uSize)
{
	char *pWriteAddr;
	UINT uItems, curItem;
	char *pStart;
	UINT totLen;
	PSARE_FILE_ITEM pFI;
	PSARE_FILE_ITEM pSubsysFI;
	PSARE_REC pRec;

	if ( !pAddr ){
		WRITE_TRACE(DBG_FATAL, "Invalid parameter in saving custom data");
		return false;
	}

	if(NULL == g_sareCustom.pFI || NULL == g_sareCustom.pRec){

		WRITE_TRACE(DBG_FATAL, "Invalid custom data state");
		return false;
	}

	pStart = g_sareCustom.pStart;
	totLen = g_sareCustom.totLen;
	pSubsysFI = g_sareCustom.pFI;
	curItem = g_sareCustom.curItem;
	pRec = g_sareCustom.pRec;

	pFI = (PSARE_FILE_ITEM)(g_sareCustom.pStart + pSubsysFI->uOffs);

	uItems = pFI->uOffs/sizeof(SARE_FILE_ITEM);

	if(curItem >= uItems){
		WRITE_TRACE(DBG_FATAL, "Custom data index out of range %s[%u], 0x%x, line=%u",
			pRec->pItemName, curItem, uItems, __LINE__);
		return false;
	}

	pFI++;

	pWriteAddr = (pStart + pFI[curItem].uOffs);
	if(pWriteAddr + uSize > pStart + totLen){
		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%x,%p,0x%x)",
			pRec->pItemName, curItem, pWriteAddr, uSize, pStart, totLen);
		return false;
	}

	pFI[curItem].uLength = uSize;
	pFI[curItem].iID = curItem;
	pFI[curItem].iType = TypeVar;

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL, "Custom data saving: item saving 0x%08x bytes from %p to %p '%s[%u]'",
			uSize, pAddr, pWriteAddr, g_sareCustom.pRec->pItemName, curItem);
	}

	if(g_uSaReVerboseMask > 1){

		SaReDumpData(pAddr, uSize);
	}

	memcpy(pWriteAddr, pAddr, uSize);

	//
	// Update current subsystem length
	//
	pSubsysFI->uLength += ROUND_UP(uSize);

	//
	// Prepare for the next step
	//
	curItem++;
	pFI[curItem].uOffs = pSubsysFI->uOffs + pSubsysFI->uLength;
	g_sareCustom.curItem = curItem;

	return true;
}

BOOL CustomSubSystemStopWrite()
{
	UINT curItem;
	PSARE_FILE_ITEM pFI;
	PSARE_FILE_ITEM pSubsysFI;

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL, "%s: stop writing",g_sareCustom.pRec->pItemName);
	}

	pSubsysFI = g_sareCustom.pFI;
	curItem = g_sareCustom.curItem;

	pFI = (PSARE_FILE_ITEM)(g_sareCustom.pStart + pSubsysFI->uOffs);

	curItem++;

	pFI[curItem].iID = 0;
	pFI[curItem].iType = TypeCloseTagSubSys;
	pFI[curItem].uOffs = 0;
	pFI[curItem].uLength = 0;

	memset(&g_sareCustom, 0 ,sizeof(g_sareCustom));
	return true;
}


PSARE_REC SaReFindFuncIdByPtr(
			PSARE_REC pStartRec,
			UINT *pFuncId,
			VOID *pFuncPtr
			)
{
	PSARE_REC pNextRec;

	for (pNextRec = pStartRec; ; pNextRec++)
	{
		if (LastElem == pNextRec->iType)
			break;

		if (RegisterFuncPtr != pNextRec->iType)
			continue;

		if (pNextRec->pAddr == pFuncPtr)
			break;
	}

	if (RegisterFuncPtr != pNextRec->iType)
		return NULL;

	*pFuncId = (UINT)(ULONG_PTR)pNextRec->pAddrResEx;

	return pNextRec;
}


PSARE_REC SaReFindFuncPtrById(
			PSARE_REC pStartRec,
			UINT uFuncId,
			VOID **ppFuncPtr
			)
{
	PSARE_REC pNextRec;

	for (pNextRec = pStartRec; ; pNextRec++)
	{
		if (LastElem == pNextRec->iType)
			break;

		if (RegisterFuncPtr != pNextRec->iType)
			continue;

		if ((ULONG_PTR)pNextRec->pAddrResEx == uFuncId)
			break;
	}

	if (RegisterFuncPtr != pNextRec->iType)
		return NULL;

	*ppFuncPtr = pNextRec->pAddr;

	return pNextRec;
}


bool SaReSetFuncIdByPtr(
			VOID *pReadFrom,
			PSARE_REC pRec,
			VOID *pWriteTo,
			PSARE_FILE_ITEM pFI,
			char *pStart,
			UINT totLen,
			UINT dataId
			)
{
	UINT i;
	UINT uItems = pRec->uSize;

	if((char*)pWriteTo + uItems*sizeof(UINT) > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Function ID assigning failed. Ptr is out of range. Item %s (%p,0x%lx,%p,0x%x)",
			pRec->pItemName, pWriteTo, uItems*sizeof(UINT), pStart, totLen);
		return false;
	}

	for(i = 0;i<uItems;i++){

		PSARE_REC pFuncRec = SaReFindFuncIdByPtr(
								pRec,
								&((UINT*)pWriteTo)[i],
								((void**)pReadFrom)[i]);
		if (!pFuncRec)
		{

			WRITE_TRACE(DBG_FATAL, "Function ID assigning failed. Name=%s[%u], FuncId=%p, line=%u",
						pRec->pItemName, i,(void*)((ULONG_PTR*)pReadFrom)[i], __LINE__);
			return false;
		}
		else if (g_uSaReVerboseMask)
		{
			WRITE_TRACE(DBG_FATAL, "Function %s (%p) is set for %s[%05u] item, line=%u",
				pFuncRec->pItemName, pFuncRec->pAddr, pRec->pItemName, i, __LINE__);
		}
	}

	pFI->iID = dataId;
	pFI->iType = TypeVar;
	pFI->uLength = uItems*sizeof(UINT);

	return true;
}


bool SaReSaveData(
		void* pDataAddr,
		PSARE_REC pRec,
		PSARE_REC pEndRec,
		PSARE_FILE_ITEM pFI,
		char *pStart,
		UINT totLen,
		UINT arrayIndex,
		UINT dataId
		)
{
	char *pWriteAddr;
	void *pReadAddr = NULL;
	UINT uSize = -1;

	(void)pEndRec;

	pWriteAddr = pStart + pFI->uOffs;

	switch(pRec->iType){
	case Field:
	case Field_Arr:
		uSize = pRec->uSize;
		pReadAddr = pRec->pAddr;
		break;

	case SareQString:
		return SaReSaveQString(pRec, pFI, pStart, totLen, dataId);

	case ClassField:
	case ClassField_Arr:
		pReadAddr = (char*)pDataAddr + (UINT)pRec->iOff;
		uSize = pRec->uSize;
		break;

	case ClassFieldPtrPtr:
		pReadAddr = *(void**)((char*)pDataAddr + (UINT)pRec->iOff);
		uSize = pRec->uSize;
		break;

	case SareIgnore:
		if(g_uSaReVerboseMask){
			WRITE_TRACE(DBG_FATAL,"Item %s ignored", pRec->pItemName);
		}
		//
		// Assume UINT32 if size is not specified
		//
		uSize = pRec->uSize ? pRec->uSize : 4;
		pReadAddr = NULL;
		break;

	case ArraySingleField:

		if(0 != arrayIndex){

			WRITE_TRACE(DBG_FATAL,"ArraySingleField should be skipped at above level i=0x%x, line=%u",
				arrayIndex, __LINE__);
			return false;
		}
		uSize = pRec->uSize;
		pReadAddr = pRec->pAddr;
		break;

	case DynData:
	{
		SareGetDynDataForSaveFunc pGetPtr = (SareGetDynDataForSaveFunc)pRec->pAddr;
		if(NULL == pGetPtr){

			WRITE_TRACE(DBG_FATAL,"Invalid DynData for item %s[%u], pGetPtr is NULL, line=%u",
				pRec->pItemName, arrayIndex, __LINE__);
			return false;
		}

		pGetPtr(pDataAddr, &pReadAddr, &uSize);
	}
		break;

	case DynStructField:
	case DynStructOffsetField:
	{
		SareGetPtrType pGetPtr = (SareGetPtrType)pRec->pAddr;

		if(NULL == pGetPtr){

			WRITE_TRACE(DBG_FATAL,"Invalid DynStructField item %s[%u], pGetPtr is NULL, line=%u",
				pRec->pItemName, arrayIndex, __LINE__);
			return false;
		}

		uSize = pRec->uSize;
		if(DynStructOffsetField == pRec->iType)
			pReadAddr = (*pGetPtr)(pRec->iOff);
		else if (DynStructField == pRec->iType)
			pReadAddr = (*pGetPtr)(arrayIndex);
		else
			pReadAddr = NULL;
		if(NULL == pReadAddr){

			//
			// at this case we will write zeros into the file
			// but not more than four bytes
			//
			if(uSize > sizeof(UINT))
				uSize = sizeof(UINT);
		}
		break;
	}

	case ClassFuncPtr:

		pReadAddr = (char*)pDataAddr + (UINT)pRec->iOff;
		return SaReSetFuncIdByPtr(pReadAddr, pRec, pWriteAddr, pFI, pStart, totLen, dataId);

	default:
		WRITE_TRACE(DBG_FATAL," unexpected var type=0x%x, item=%s[%u], line=%u",
			pRec->iType, pRec->pItemName, arrayIndex, __LINE__);
		return false;
	}

	if(pWriteAddr + uSize > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Invalid data length Item %s[%u] (%p,0x%x,%p,0x%x), line=%u",
			pRec->pItemName, arrayIndex, pWriteAddr, uSize, pStart, totLen, __LINE__);
		return false;
	}

	pFI->uLength = uSize;
	pFI->iID = dataId;
	pFI->iType = TypeVar;

	if(pReadAddr)
		memcpy(pWriteAddr, pReadAddr, uSize);
	else
		memset(pWriteAddr, 0, uSize);

	if(g_uSaReVerboseMask){
		WRITE_TRACE(DBG_FATAL,"Data saving: #=0x%08x, fid=0x%08x %u bytes copied to %p from %p '%s[%u]'",
			dataId, pFI->iID, pFI->uLength, pWriteAddr, pReadAddr, pRec->pItemName, arrayIndex);
	}
	if(g_uSaReVerboseMask > 1){

		SaReDumpData(pWriteAddr, uSize);
	}

	return true;
}

bool
	SaReDoSave(
		void *pDataAddr,		// the actual address of current subsystem data beginning
		PSARE_REC pRec,			// current subsystem data descriptor
		PSARE_REC pEndRec,		// the limit of current subsystem descriptor
		char *pStart,			// data storage beginning
		UINT totLen,			// data storage limit
		PSARE_FILE_ITEM pFI,	// current subsystem storage data description
		UINT level,				// current recursion level
		UINT actionMask,		// current processing stage & action
		UINT arrayIndex,		// index for data declared as arrays
		UINT dataId				// data sequence number
		)
{
	static PSARE_REC RecState[SARE_MAX_EXT_SUB_SYS];
	PSARE_FILE_ITEM pNextFI = NULL;
	PSARE_REC pNextRec = NULL;
	UINT uItems = 0;
	UINT i;
	bool bArray;
	PSARE_REC pArrRec = NULL;
	void *pCurDataAddr;

	if(level>=SARE_MAX_EXT_SUB_SYS){

		WRITE_TRACE(DBG_FATAL, "Too deep recursion. %s[%u], lev=%u, line=%u",
			pRec->pItemName, arrayIndex, level, __LINE__);
		return false;
	}

	if((char*)pFI + sizeof(SARE_FILE_ITEM) > pStart + totLen){

		WRITE_TRACE(DBG_FATAL, "Ptr is out of range. Item %s[%u] (%p,0x%lx,%p,0x%x), line=%u",
			pRec->pItemName, arrayIndex, pFI, sizeof(SARE_FILE_ITEM), pStart, totLen, __LINE__);
		return false;
	}

	if(pRec > pEndRec){
		WRITE_TRACE(DBG_FATAL, "Current item is out of range. Cur=%p, End=%p %s[%u], line=%u",
			pRec, pEndRec, pEndRec->pItemName, arrayIndex, __LINE__);
		return false;
	}

	RecState[level] = pRec;

	switch(pRec->iType){
	case SubSystem:
	case ArraySubSystem:

		if(!(pRec->uFlags & actionMask)){
			//
			// skip those subsystems that were not requested
			//
			return true;
		}

		pNextRec = (PSARE_REC)pRec->pAddr;
		if(ArraySubSystem == pRec->iType)
			pEndRec = pNextRec + 2;
		else
			pEndRec = pNextRec + (int)pRec->uSize + 1;

		if(pRec->uFlags & fArrSysNested){
			// Subsystems declared as NESTED get their addresses from above systems
			pDataAddr = (char*)pDataAddr + pRec->iOffNested;
		}else if ( pNextRec->uFlags & fPtrObj ){
			pDataAddr = pNextRec->pAddr;
			if (!pDataAddr)
				return true;

		}
		else if ( pNextRec->uFlags & fPtrPtrObj ){
			pDataAddr = *(void **)pNextRec->pAddr;
			if (!pDataAddr)
				return true;
		}

		return SaReDoSave(pDataAddr, pNextRec, pEndRec, pStart, totLen, pFI,	level + 1, actionMask, arrayIndex, dataId);

	case ClassCustomSubSystem:
	case CustomSubSystem:

		if(!(pRec->uFlags & actionMask)){
			//
			// skip those subsystems that were not requested
			//
			return true;
		}

		return SaReLoadSaveCustomData(pDataAddr, pRec, pEndRec, pStart, totLen, pFI, level + 1, pRec->puSize);

	case SubSysStart:

		break;

	default:
		WRITE_TRACE(DBG_FATAL, "Unexpected type 0x%x, item=%s[%u], line=%u",
			pRec->iType, pRec->pItemName, arrayIndex, __LINE__);
		return false;
	}

	if(level == 0 || ArraySubSystem != RecState[level-1]->iType){
		bArray = false;
		uItems = pEndRec - pRec - 1;
	}
	else{
		bArray = true;
		pArrRec = RecState[level-1];
		uItems = pArrRec->uSize;
	}

	pFI->iID = dataId;
	pFI->iType = TypeSubSys;
	pFI->uLength = (uItems+2)*sizeof(SARE_FILE_ITEM); // TypeOpenTagSubSys + Actual data + TypeCloseTagSubSys

	pNextFI = (PSARE_FILE_ITEM)(pStart + pFI->uOffs);
	pNextFI->iID = 0;
	pNextFI->iType = TypeOpenTagSubSys;
	pNextFI->uOffs = uItems*sizeof(SARE_FILE_ITEM);
	pNextFI->uLength = 0;

	if(g_uSaReVerboseMask){
		SaRePrintData("Saving", pRec, pStart, pNextFI, level);
	}

	for(i = 0, pNextFI = pNextFI+1, pNextRec = pRec + 1;
		i<uItems;
		i++, pNextFI++){

		PSARE_REC pToSaveRec;

		while(!SaReRecIsData(pNextRec, arrayIndex)){

			pNextRec++;
			if(pNextRec > pEndRec){
				WRITE_TRACE(DBG_FATAL, "Current item is out of range. Cur=%p, End=%p %s[%u], line=%u",
					pNextRec, pEndRec, pEndRec->pItemName, arrayIndex, __LINE__);
				return false;
			}
			i++;
		}

		if(LastElem == pNextRec->iType){

			break;
		}

		pNextFI->uOffs = pFI->uOffs + pFI->uLength;

		pToSaveRec = pNextRec;

		if(pArrRec){

			arrayIndex = i;

			if(pArrRec->uFlags & fArrSysPtrOpt){

				pCurDataAddr = SaReGetOptArrPtr(pArrRec, i);
				if(NULL == pCurDataAddr)
					 pToSaveRec = &DummyRec;

			}else{
				//
				// pArrRec->iOff actually is sizeof array item
				//
				pCurDataAddr = (char*)pDataAddr + i*pArrRec->iOff;
			}

		}else{

			pCurDataAddr = pDataAddr;
		}

		switch(pToSaveRec->iType){
		case SubSystem:
		case ArraySubSystem:
		case CustomSubSystem:
		case ClassCustomSubSystem:

			if(!SaReDoSave(pCurDataAddr, pToSaveRec, pEndRec, pStart, totLen, pNextFI, level + 1, actionMask, arrayIndex, i)){

				return false;
			}
			break;

		default:

			if(!SaReSaveData(pCurDataAddr, pToSaveRec, pEndRec, pNextFI, pStart, totLen, arrayIndex, i)){

				return false;
			}
		}

		pFI->uLength += ROUND_UP(pNextFI->uLength);

		if(g_uSaReVerboseMask){
			SaRePrintData("Saving", pToSaveRec, pStart, pNextFI, level);
		}

		if(!bArray){

			pNextRec++;
		}
	}

	pNextFI = (PSARE_FILE_ITEM)(pStart + pFI->uOffs) + uItems + 1;

	pNextFI->iID = 0;
	pNextFI->iType = TypeCloseTagSubSys;
	pNextFI->uOffs = 0;
	pNextFI->uLength = 0;

	if(g_uSaReVerboseMask){
		SaRePrintData("Saving", pNextRec, pStart, pNextFI, level);
	}

	return true;
}

bool SaRePrepareSave(void *pBuf, UINT fileSize, UINT offset, bool bMonitor)
{
	PSARE_FILE_ITEM pFI;

	if(4*sizeof(SARE_FILE_ITEM) + offset > fileSize){

		WRITE_TRACE(DBG_FATAL, "Invalid buffer length 0x%x, 0x%x, line=%u",
			offset, fileSize, __LINE__);
		return false;
	}

	pFI = (PSARE_FILE_ITEM)pBuf;

	pFI->iID = 0;
	pFI->iType = TypeFile;
	pFI->uOffs = 0;
	pFI->uLength = 0;
    pFI+= (bMonitor ? 1 : 2);
	pFI->uOffs = 3*sizeof(SARE_FILE_ITEM)+offset;
	pFI->iID = 0;
	pFI->uLength = 3*sizeof(SARE_FILE_ITEM);
	pFI->iType = TypeSubSys;
	return true;
}


bool SaReSaveMain(PSARE_REC pRootRec, void *pBuf, UINT fileSize, UINT stageMask, bool bMonitor)
{
	bool bOk = false;
	UINT curCallMask;
	//
	// Root SARE_REC array always contains 3(three) elements
	// so end record is always &pRootRec[2] or pRootRec + 2
	//

	curCallMask = stageMask & ~(fAllCallbacks & ~fBeforeSave);

	bOk = SaReProcessCallbacks(pRootRec, pRootRec+2, curCallMask, 0);
	if(!bOk) {

		WRITE_TRACE(DBG_FATAL, "Processing callbacks before saving failed");
		return bOk;
	}

	bOk = SaReDoSave(
				NULL,
				pRootRec,
				pRootRec+2,
				(char*)pBuf,
				fileSize,
				(PSARE_FILE_ITEM)pBuf + (bMonitor ? 1 : 2),
				0,
				stageMask,
				0,
				0
				);
	if(!bOk) {
		WRITE_TRACE(DBG_FATAL, "Saving data failed");
		return bOk;
	}

	curCallMask = stageMask & ~(fAllCallbacks & ~fAfterSave);

	bOk = SaReProcessCallbacks(pRootRec, pRootRec+2, curCallMask, 0);
	if(!bOk) {

		WRITE_TRACE(DBG_FATAL, "Processing callbacks after saving failed");
		return bOk;
	}

	return bOk;
}
