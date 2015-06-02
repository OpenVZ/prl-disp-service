///
/// Copyright (C) 2006 Parallels Inc. All Rights Reserved.
///		http://www.parallelssoft.com
///
/// MODULE:
///		CMultiEditMergeVmConfig.h
///
/// AUTHOR:
///		sergeyt
///
/// DESCRIPTION:
///	This class implements logic for simultaneously edit & merge Vm configuration
///
/// COMMENTS:
///	sergeyt
///
/////////////////////////////////////////////////////////////////////////////

#ifndef CMultiEditMergeVmConfig_H
#define CMultiEditMergeVmConfig_H

#include "CMultiEditMergeHelper.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"

class CMultiEditMergeVmConfig: public CMultiEditMergeHelper<CVmConfiguration>
{
protected:

	virtual bool merge(const SmartPtr<CVmConfiguration>& pPrev
		, SmartPtr<CVmConfiguration>& pNew
		, const SmartPtr<CVmConfiguration>& pCurr);

};

#endif
