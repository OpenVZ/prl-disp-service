///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// http://www.parallelssoft.com
///
/// MODULE:
///		CMultiEditMergeVmConfig.cpp
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

#include "CMultiEditMergeVmConfig.h"
#include "CDspService.h"
#include "Libraries/Logging/Logging.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"

#include "CDspTestConfig.h"
#include "Build/Current-locale.ver"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


namespace
{
	void dumpConfigs( const QDateTime& dtTimeStamp
		, const QString& sNewSuffix
		, const SmartPtr<CVmConfiguration>& pPrev
		, SmartPtr<CVmConfiguration>& pNew
		, const SmartPtr<CVmConfiguration>& pCurr)
	{
	#ifndef EXTERNALLY_AVAILABLE_BUILD

		int nDumpEnabled = 0;
		if( !TestConfig::getParamByName( "MERGE_TEST.VMCONFIG.DUMP_ENABLED" ,nDumpEnabled ) || nDumpEnabled != 1 )
			return;

		QString sPrefix = "/tmp/";
		sPrefix += dtTimeStamp.toString( "MMdd-hhmmss_zzz." );

		QIODevice::OpenMode mode = QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text;
		QFile f1( sPrefix + "curr.xml" );
		if( f1.open( mode ) )
			f1.write( QSTR2UTF8(pCurr->toString()) );
		QFile f2( sPrefix +  QString("new.") + sNewSuffix + ".xml" );
		if( f2.open( mode ) )
			f2.write( QSTR2UTF8(pNew->toString()) );
		QFile f3( sPrefix + "prev.xml" );
		if( f3.open( mode ) )
			f3.write( QSTR2UTF8(pPrev->toString()) );
	#else
		Q_UNUSED(dtTimeStamp);
		Q_UNUSED(sNewSuffix);
		Q_UNUSED(pPrev);
		Q_UNUSED(pNew);
		Q_UNUSED(pCurr);
	#endif

	}
} // namespace

bool CMultiEditMergeVmConfig::merge(const SmartPtr<CVmConfiguration>& pPrev
		   , SmartPtr<CVmConfiguration>& pNew
		   , const SmartPtr<CVmConfiguration>& pCurr)
{

	PRL_ASSERT(pPrev);
	PRL_ASSERT(pNew);
	PRL_ASSERT(pCurr);

	QDateTime dt = QDateTime::currentDateTime();

	dumpConfigs( dt, "before-merge", pPrev, pNew, pCurr );

	int rc = pNew->mergeDocuments( pCurr.getImpl(), pPrev.getImpl(),
		CDspService::isServerMode() ? moEnableAllOptions : moEnableFixedFields );

	WRITE_TRACE( DBG_INFO, "Merge was finished with result %#x %s, '%s'"
		, rc, PRL_RESULT_TO_STRING(rc ), QSTR2UTF8(pNew->GetErrorMessage()) );

	dumpConfigs( dt, "after", pPrev, pNew, pCurr );

	return 0 == rc;
}
