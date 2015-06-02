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

#ifndef __SAREFILE_H__
#define __SAREFILE_H__

#include "Interfaces/ParallelsTypes.h"

/**
 *  * Version of sav file. It must be increment to 1, if we added new suspend data.
 *   * If current soft version of sav file >= saved version, we will resume it state.
 *    * If current soft version of sav file < saved version, we will not resume it state.
 *     */
#define SAV_FILE_VERSION_SOFT 0x30077

/**
 *  * Hard version of sav file. DO NOT INCREMENT IT!
 *   * If current hard version of sav file == saved hard version, we will resume it state.
 *    * If current hard version of sav file != saved version, we will not resume it state.
 *     * It version must be increment if we can't resume old version in new version.
 *      */
#define SAV_FILE_VERSION_HARD 0x40001

#define SARE_ID_FILE_OLD 0x1234567F

#define SARE_IO_BUF_SZ 0x500000

extern UINT g_uSaReVerboseMask;
extern ULONG_PTR *puObsoleteDr0;

#define SARE_MAX_EXT_SUB_SYS 128

/*
 * This a hack to use OffsetOf for C++ classes. Cast to 0
 * can't be used because of GCC compiler warnings.
 */
#define OffsetOf(class_name,public_field_name)						\
		((UINT)((ULONG_PTR)&(((class_name *)16)->public_field_name))-16)

#define SizeOf(class_name,public_field_name) sizeof((((class_name *)16)->public_field_name))

typedef enum _SareRecType
{
	SubSysStart = 0,
	LastElem,

	Field,
	Field_Arr,
	ClassField,
	ClassField_Arr,
	ClassFuncPtr,
	ClassFieldPtrPtr,
	SareQString,
	RegisterFuncPtr,

	SubSystem,
	ArraySubSystem,
	CustomSubSystem,

	SareCallback,
	SareClassCallback,
	SareClassCallbackEx,

	ClassCustomSubSystem,

	SareIgnore,
	DynStructField,
	ArraySingleField,
	DynStructOffsetField,
	DynData,

}SareRecType;

enum _DescrFlags_t
{
	fClear =				0,          // initial
	fFirstStage =			0x00000001, // for those that should be processed first
	fMainStage =			0x00000002, // for those that should be processed at the main stage
	fPramStage =			0x00000004, // for those that should be processed at the PRAM loading stage
	fSpecialSize =			0x00000010, // Element describe sizeof special datum
	fSpecialDatum =			0x00000020, // Element describe special datum
	fArrSysNested =			0x00000040,	// For array subsystem - if subsystem array is nested
	fOptional =				0x00000080,	// For workaround usage ONLY - if subsystem array is nested
	fPtrObj =				0x00000100, // Pointer to real obj
	fPtrPtrObj =			0x00000200, // Pointer to pointer to real obj
	fArrSysPtrOpt =			0x00000400, // Array of pointers that are optional in general
	fBeforeRestore =		0x00001000, // for callbacks that should be processed before restore
	fBeforeSave	=			0x00002000, // for callbacks that should be processed before save
	fAfterRestore =			0x00004000, // for callbacks that should be processed after restore
	fAfterSave =			0x00008000, // for callbacks that should be processed after save
	fRollbackSave =			0x00010000, // to rollbacks failed saving
	fCbWasCalled =			0x00020000, // This callback was called

	fActionInit	=			0x00100000,
	fActionDeleteSnapshot=	0x00200000,
	fActionDiskSnapshot=	0x00400000,
	fActionSuspend =		0x01000000,
	fActionResume =			0x02000000,
	fActionTakeSnapshot=		0x04000000,
	fActionRevertSnapshot=		0x08000000,
	fActionPRAM =                   0x10000000,
	fActionCommitUnfinished=        0x20000000,
	fAllCallbacks	=  fBeforeRestore | fBeforeSave	| fAfterRestore | fAfterSave | fRollbackSave
};

enum _SareMonAppCmds_t{

	SareReqSetConnect = 0x101,
	SareAnsConnectOk = 0x201,
	SareReqSetDisconnect = 0x210,
	SareAnsDisconnectOk = 0x211,
	SareDataLoaded = 0x220,
	SareDataSaved = 0x202,
	SareAskUserToContinue = 0x120,
	SareRestoreTuneFinished = 0x102,
};

typedef	enum _SARE_FI_TYPES{

	TypeFile =		0x8A9FFFFE,
	TypeSubSys =		0x8A9FFFFA,
	TypeOpenTagSubSys =	0x8A9FFFFD,
	TypeCloseTagSubSys =	0x8A9FFFFB,
	TypeVar =		0x8A9FFFFC,
	IDFile =		0x1234567F,
	TypeZero =		0,

}SARE_FI_TYPES;

#include <Interfaces/packed.h>

typedef struct _STransportHeader
{
	UINT	uSizeHeader;
	int		iCommand;
	BOOL	bAsync;
	UINT	uSizePack;
	int		iError;
	UINT	uParam;
} STransportHeader;

struct SElemMessage
{
	UINT	uSize;
	int		iCommand;
	int		iError;
	BOOL	bAsync;
	UINT	uParam;
};

typedef struct _SARE_REC
{
	SareRecType		iType;
	void			*pAddr;
	UINT			uSize;
	UINT			iOff;
	UINT			*puSize;
	UINT			uFlags;
	UINT			iOffNested;
	void			*pAddrResEx;
	const char		*pItemName;
	void			*pRestoreCb;

}SARE_REC, *PSARE_REC;

typedef struct _SARE_FILE_ITEM
{
	UINT	iID;
	UINT    iType; // actually SARE_FI_TYPES
	UINT	uLength;
	UINT	uOffs;

}SARE_FILE_ITEM, *PSARE_FILE_ITEM;

#include <Interfaces/unpacked.h>

typedef struct _SARE_CUSTOM_ACTIVITY{
	PSARE_REC pRec;
	PSARE_REC pEndRec;
	char *pStart;
	UINT totLen;
	PSARE_FILE_ITEM pFI;
	UINT level;
	UINT curItem;
	void *pDataAddr;
	bool started;
}SARE_CUSTOM_ACTIVITY, *PSARE_CUSTOM_ACTIVITY;

#define SARE_SUBSYS_START(SubSysName)	\
	SARE_REC SubSysName[] = {	\
	{SubSysStart,NULL,SubSysStart,(UINT)-1,NULL,fClear,0,0, #SubSysName".SubSysStart",0},

#define SARE_SUBSYS_START_EX(SubSysName, Ptr) \
	SARE_REC SubSysName[] = {										\
	{SubSysStart,Ptr,SubSysStart,(UINT)-1,NULL, fPtrObj, 0,0, #SubSysName".SubSysStart",0},

#define SARE_SUBSYS_FINISH(SubSysName)								\
	{LastElem,0,0,(UINT)-1,NULL,fClear,0,0, #SubSysName".LastElem",0} };

#define SARE_FIELD(field)											\
	{Field,&field,sizeof(field),(UINT)-1,NULL,0,0,0,#field,0},

#define SARE_FIELD_CB(field, afterResumeCb)											\
	{Field,&field,sizeof(field),(UINT)-1,NULL,0,0,0,#field,(void*)afterResumeCb},

#define SARE_FIELD_PTR_ARR(field)											\
	{Field_Arr,&field,sizeof(field),(UINT)-1,NULL,0,0,0,#field,0},

#define SARE_DYN_STRUCT(pFuncGetPtr, Size) \
	{DynStructField, (void *)pFuncGetPtr, Size, (UINT)-1, (UINT*)&pFuncGetPtr,0,0,0,#pFuncGetPtr,0},

#define SARE_DYN_STRUCT_OFFSET(pFuncGetPtr, ClassName, field) \
	{DynStructOffsetField, (void *)pFuncGetPtr, SizeOf(ClassName,field),OffsetOf(ClassName,field),\
		(UINT*)&pFuncGetPtr,0,0,0,#ClassName"::"#field,0},

#define SARE_CLASS_DYN_DATA(saveCb, restoreCb) \
	{DynData, (void *)saveCb, 0, (UINT)-1, 0,0,0,(void*)restoreCb,#saveCb#restoreCb,0},

#define SARE_FIELD_SINGLE(field,pRestoreFunc) \
	{ArraySingleField,&field,sizeof(field),(UINT)-1,NULL,0,0,(void *)pRestoreFunc,#field,0},

#define SARE_IGNORE(field)	\
	{SareIgnore,NULL,0,0,NULL,fClear,0,0,#field,0},

#define SARE_IGNORE_WITH_SIZE(field, size)	\
	{SareIgnore,NULL,size,0,NULL,fClear,0,0,#field,0},

#define SARE_IGNORE2(ClassName,field) \
	{SareIgnore,NULL,0,0,NULL,fClear,0,0,#ClassName"::"#field,0},

#define SARE_SUBSYSTEM(SubSysAddr)									\
	{SubSystem,&SubSysAddr,sizeof(SubSysAddr)/sizeof(SubSysAddr[0])-2,(UINT)-1,NULL,fMainStage,0,0,#SubSysAddr,0},

#define SARE_SUBSYSTEM_FIRST_STAGE(SubSysAddr)									\
	{SubSystem,&SubSysAddr,sizeof(SubSysAddr)/sizeof(SubSysAddr[0])-2,(UINT)-1,NULL,fFirstStage,0,0,#SubSysAddr,0},

#define SARE_SUBSYSTEM_PRAM_STAGE(SubSysAddr)									\
	{SubSystem,&SubSysAddr,sizeof(SubSysAddr)/sizeof(SubSysAddr[0])-2,(UINT)-1,NULL,fPramStage,0,0,#SubSysAddr,0},

#define SARE_SUBSYSTEM_ALL_STAGES(SubSysAddr)									\
	{SubSystem,&SubSysAddr,sizeof(SubSysAddr)/sizeof(SubSysAddr[0])-2,(UINT)-1,NULL,fFirstStage|fPramStage|fMainStage,0,0,#SubSysAddr,0},

#define EXT_SUBSYS_NAME SubSysAddr##_array_element

#define SARE_MAKE_SUBSYS_ARRAY(SubSysAddr) \
	SARE_SUBSYS_START(SubSysAddr##_array_element)	  \
	SARE_SUBSYSTEM(SubSysAddr)						  \
	SARE_SUBSYS_FINISH(SubSysAddr##_array_element)

#define SARE_MAKE_SUBSYS_ARRAY_PTR(SubSysAddr, Addr) \
	SARE_SUBSYS_START_EX(SubSysAddr##_array_element, Addr)	  \
	SARE_SUBSYSTEM(SubSysAddr)	\
	SARE_SUBSYS_FINISH(SubSysAddr##_array_element)

#define SARE_CUSTOM_SUSPEND_RESUME_MECHNISM(f_pSaveFunc,f_pLoadFunc)			\
	{CustomSubSystem,NULL,0,(UINT)-1, \
		(UINT *)(&f_pSaveFunc),fClear|fMainStage,0,(void *)(&f_pLoadFunc), #f_pSaveFunc,0},

#define SARE_CLASS_CUSTOM_SUSPEND_RESUME_MECHNISM(ClassName,f_pSaveFunc,f_pLoadFunc)	\
	{ClassCustomSubSystem,NULL,0,(UINT)-1, \
		(UINT *)(&ClassName::f_pSaveFunc),fClear|fMainStage,0,(void *)(&ClassName::f_pLoadFunc),\
			#ClassName"::"#f_pSaveFunc,0},

#define SARE_CLASS_CUSTOM_PRAM_SUBSYS(ClassName,f_pSaveFunc,f_pLoadFunc)	\
	{ClassCustomSubSystem,NULL,0,(UINT)-1, \
		(UINT *)(&ClassName::f_pSaveFunc),fClear|fPramStage,0,(void *)(&ClassName::f_pLoadFunc),\
			#ClassName"::"#f_pSaveFunc,0},

#define SARE_FUNCTION(pFunc, funcID) \
	{RegisterFuncPtr,(void *)pFunc,sizeof(void *),(UINT)-1,NULL,fClear,0,(void*)funcID, #pFunc,0},


#define SARE_CLASS_FIELD(ClassName,field) \
	{ClassField,NULL,SizeOf(ClassName,field),OffsetOf(ClassName,field),NULL,fClear,0,0,#ClassName"::"#field,0},

#define SARE_CLASS_FIELD_CB(ClassName,field,afterResumeCb) \
	{ClassField,NULL,SizeOf(ClassName,field),OffsetOf(ClassName,field),NULL,fClear,0,0,#ClassName"::"#field,(void*)afterResumeCb},

#define SARE_CLASS_FIELD_CUSTOM(ClassName,field, pRestoreFunc) \
	{ClassField,NULL,SizeOf(ClassName,field),OffsetOf(ClassName,field),\
	  NULL,fClear,0,(void *)pRestoreFunc,#ClassName"::"#field,0},

#define SARE_CLASS_FIELD_PTR_ARR(ClassName,field) \
	{ClassField_Arr,NULL,SizeOf(ClassName,field),OffsetOf(ClassName,field),\
	  NULL,fClear,0,0,#ClassName"::"#field,0},

#define SARE_CLASS_FUNC_PTR(ClassName, pFunc) \
	{ClassFuncPtr,NULL, 1, OffsetOf(ClassName,pFunc),NULL,fClear,0,0, #ClassName"::"#pFunc,0},

#define SARE_SPECIAL_FIELD(field,SpecialType)	\
	{SpecialType,&field, 0, 0, \
		NULL,fClear|fSpecialSize,0,0,#field,0},	\
	{SpecialType,&field, 0,0, \
		NULL,fClear|fSpecialDatum,0,0,#field,0},

#define SARE_CLASS_FIELD_PTR_SIZE(ClassName,field,uSizeField)		\
	{ClassFieldPtrPtr,NULL,uSizeField,OffsetOf(ClassName,field),NULL,fClear,0,0,#ClassName"::"#field,0},

#define SARE_CLASS_CALLBACK_BEFORE_SUSPEND(ClassName,StatFunName)	\
	{SareClassCallback,(void *)(&ClassName::StatFunName), \
		0,(UINT)-1,NULL,fClear|fBeforeSave,0,0,#ClassName"::"#StatFunName,0},

#define SARE_CLASS_CALLBACK_BEFORE_SUSPEND_EX(ClassName,StatFunName)	\
	{SareClassCallbackEx,(void *)(&ClassName::StatFunName), \
	0,(UINT)-1,NULL,fClear|fBeforeSave,0,0,#ClassName"::"#StatFunName,0},

#define SARE_CLASS_ROLLBACK_SUSPEND(ClassName,StatFunName)	\
	{SareClassCallback,(void *)(&ClassName::StatFunName), \
	0,(UINT)-1,NULL,fClear|fRollbackSave,0,0,#ClassName"::"#StatFunName,0},

#define SARE_CLASS_CALLBACK_AFTER_SUSPEND(ClassName,StatFunName)	\
	{SareClassCallback,(void *)(&ClassName::StatFunName), \
		0,(UINT)-1,NULL,fClear|fAfterSave,0,0,#ClassName"::"#StatFunName,0},

#define SARE_CLASS_CALLBACK_BEFORE_RESUME(ClassName,StatFunName)	\
	{SareClassCallback,(void *)(&ClassName::StatFunName), \
		0,(UINT)-1,NULL,fClear|fBeforeRestore,0,0,#ClassName"::"#StatFunName,0},

#define SARE_CLASS_CALLBACK_AFTER_RESUME(ClassName,StatFunName)	\
	{SareClassCallback,(void *)(&ClassName::StatFunName), \
		0,(UINT)-1,NULL,fClear|fAfterRestore,0,0,#ClassName"::"#StatFunName,0},

#define SARE_CALLBACK_BEFORE_SUSPEND(pCallBack)	\
	{SareCallback,(void *)pCallBack,0,(UINT)-1,NULL,fClear|fBeforeSave,0,0, #pCallBack,0},

#define SARE_ROLLBACK_SUSPEND(pCallBack) \
	{SareCallback,(void *)pCallBack,0,(UINT)-1,NULL,fClear|fRollbackSave,0,0,#pCallBack,0},

#define SARE_CALLBACK_AFTER_SUSPEND(pCallBack)	\
	{SareCallback,(void *)pCallBack,0,(UINT)-1,NULL,fClear|fAfterSave,0,0,#pCallBack,0},

#define SARE_CALLBACK_BEFORE_RESUME(pCallBack)	\
	{SareCallback,(void *)pCallBack,0,(UINT)-1,NULL,fClear|fBeforeRestore,0,0,#pCallBack,0},

#define SARE_CALLBACK_AFTER_RESUME(pCallBack) \
	{SareCallback,(void *)pCallBack,0,(UINT)-1,NULL,fClear|fAfterRestore,0,0,#pCallBack,0},

#define SARE_CLASS_ARRAY_SUBSYSTEM(AddrArraySubSys,uSizeArraySubSys,uSizeArraySubSysElem)		\
	{ArraySubSystem,&(AddrArraySubSys##_array_element),uSizeArraySubSys,uSizeArraySubSysElem,NULL,	\
	 fClear|fMainStage,0,0,#AddrArraySubSys".ArraySubSystem",0},

#define SARE_ARRAY_SUBSYSTEM(AddrArraySubSys,uSizeArraySubSys,uSizeArraySubSysElem)	\
	{ArraySubSystem,&(AddrArraySubSys##_array_element),\
	uSizeArraySubSys,uSizeArraySubSysElem,NULL,fMainStage,0,0,#AddrArraySubSys".ArraySubSystem",0},

#define SARE_CLASS_ARRAY_SUBSYSTEM_NESTED(AddrArraySubSys,uSizeArraySubSys,uSizeArraySubSysElem,ClassName,field)	\
	{ArraySubSystem,&(AddrArraySubSys##_array_element),uSizeArraySubSys,uSizeArraySubSysElem,	\
	 NULL,fClear|fArrSysNested|fMainStage,OffsetOf(ClassName,field),0,#AddrArraySubSys".ArraySubSystem",0},

#define SARE_ARRAY_OPT_PTR_SUBSYSTEM(pFuncGetPtr, AddrArraySubSys,uSizeArraySubSys,uSizeArraySubSysElem)	\
	{ArraySubSystem,&(AddrArraySubSys##_array_element),uSizeArraySubSys,uSizeArraySubSysElem,	\
	(UINT*)&pFuncGetPtr,fClear|fArrSysPtrOpt|fMainStage,0,0,#AddrArraySubSys".ArraySubSystem",0},

#define SARE_CLASS_ARRAY_SUBSYS_NESTED_OPTIONAL(AddrArraySubSys,uSizeArraySubSys,uSizeArraySubSysElem,ClassName,field)\
	{ArraySubSystem,&(AddrArraySubSys##_array_element),uSizeArraySubSys,uSizeArraySubSysElem,	\
	NULL,fClear|fArrSysNested|fMainStage|fOptional,OffsetOf(ClassName,field),0,#AddrArraySubSys".ArraySubSystem",0},

#define SARE_ASSIGN_PTR_TO_SUBSYS(SubSysName,pPtr)	{	\
	SARE_EXTERN_SUBSYS(SubSysName);	\
	SubSysName->pAddr = pPtr; \
	SubSysName->uFlags |= fPtrObj; \
}

#define SARE_ASSIGN_PTR_ADDRESS_TO_SUBSYS(SubSysName,pPtr)	{	\
	SARE_EXTERN_SUBSYS(SubSysName);	\
	SubSysName->pAddr = pPtr; \
	SubSysName->uFlags |= fPtrPtrObj; \
}

#define SARE_ASSIGN_PTR_TO_ARRAY(SubSysName,pPtr){	\
	SARE_EXTERN_SUBSYS(SubSysName##_array_element);	 \
	SubSysName##_array_element->pAddr = pPtr; \
	SubSysName##_array_element->uFlags |= fPtrObj; \
}

#define SARE_ASSIGN_PTR_ADDRESS_TO_ARRAY(SubSysName,pPtr){	\
	SARE_EXTERN_SUBSYS(SubSysName##_array_element);	 \
	SubSysName##_array_element->pAddr = pPtr; \
	SubSysName##_array_element->uFlags |= fPtrPtrObj; \
}

#define SARE_EXTERN_SUBSYS(SubSysName) extern SARE_REC SubSysName[];


bool SaReLoadMain(PSARE_REC pRootRec, void *pBuf, UINT fileSize, UINT stageMask, bool bMonitor);
bool SaReSaveMain(PSARE_REC pRootRec, void *pBuf, UINT fileSize, UINT stageMask, bool bMonitor);
bool SaRePrepareSave(void *pBuf, UINT fileSize, UINT offset, bool bMonitor);
bool SaReCallbackOnly(PSARE_REC pRootRec, UINT stageMask);

BOOL CustomSubSystemStartWrite(UINT uNumVars);
BOOL CustomSubSystemContinueWrite(void *pVar, UINT uVarSize);
BOOL CustomSubSystemStopWrite();
BOOL CustomSubSystemStartRead();
BOOL CustomSubSystemContinueRead(void *pAddr, UINT uSize, BOOL& bFound, UINT *puSize = NULL);
BOOL CustomSubSystemStopRead();

PSARE_REC SaReFindFuncIdByPtr(PSARE_REC pStartRec, UINT *pFuncId, VOID *pFuncPtr);
PSARE_REC SaReFindFuncPtrById(PSARE_REC pStartRec, UINT uFuncId, VOID **ppFuncPtr);

typedef int (*SARE_CALLBACK_PTR)();
typedef int (*ClassCallBackSuspendResume)(void *pObj);
typedef int (*ClassCallBackSuspendResumeEx)(void *pObj,UINT actionMask);
typedef void* (*SareGetPtrType)(UINT);
typedef bool (*SareRestoreFpuType)(PSARE_REC, void *pReadFrom, UINT uSize);
typedef bool (*SareCustomRestoreType)(void *pWriteTo, void *pReadFrom, PSARE_REC pRec, PSARE_FILE_ITEM pFI);
typedef void (*SareGetDynDataForSaveFunc)(void *pObj, void **pData, UINT *pSize);
typedef void (*SareGetDynDataForRestoreFunc)(void *pPbj, void **pData, UINT FileSize, UINT *pRestoreSize);
typedef void (*SareFieldAfterRestoreFunc)(UINT i, void *pData, UINT uSize);

#endif //__SAREFILE_H__
