///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTestConfig
///
///	To test config support
///
/// @author sergeyt
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////
#include <prlcommon/Interfaces/ParallelsQt.h>

#include "CDspTestConfig.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/Logging/Logging.h>

#include <QFile>
#include <QTextStream>
#include <QStringList>

QMutex TestConfig::ms_mtx( QMutex::Recursive );
QList< QPair<QString,QString> > TestConfig::m_lstPairs;

namespace
{
	//CDspLockedPointer<TestConfig> g_pTestConfig =  TestConfig::instance();
	//TestConfig* g_pTestConfig = TestConfig::instance();
}

TestConfig::TestConfig()
{
	if( isPresent() )
		reload();
}

TestConfig*  TestConfig::instance()
{
	static SmartPtr<TestConfig> p(0);
	if(!p)
		p = SmartPtr<TestConfig>( new TestConfig );
	return p.getImpl();
}

CDspLockedPointer<TestConfig> TestConfig::instance2()
{
	WRITE_TRACE(DBG_FATAL, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"  );

	static SmartPtr< TestConfig  > pTestConfig(0);

	CDspLockedPointer<TestConfig>	p( &ms_mtx, pTestConfig.getImpl() );
	if( p )
		return p;


	pTestConfig = SmartPtr<TestConfig>( new TestConfig() );
	return  CDspLockedPointer<TestConfig>( &ms_mtx, pTestConfig.getImpl() );
}

bool TestConfig::reload()
{
	QFile testerConfig( ParallelsDirs::getPathToDispatcherTesterConfig() );

	WRITE_TRACE(DBG_FATAL, "path=%s", QSTR2UTF8( ParallelsDirs::getPathToDispatcherTesterConfig()  ) );
	if( !testerConfig.exists() || ! testerConfig.open(QIODevice::ReadOnly) )
	{
		if( m_lstPairs.size() > 0 )
		{
			WRITE_TRACE(DBG_FATAL, "clean up tester config"  );
			m_lstPairs.clear();
		}
		return false;
	}

	QTextStream in( &testerConfig );
	while( !in.atEnd() )
	{
		QString line = in.readLine();
		if( line.size() == 0 )
			continue;
		if( line[0] == '#' )// skip comments
			continue;

		int pos = line.indexOf('=');
		if( pos == -1 )
			continue;

		QStringList lst = line.split("=");
		if(lst.size()>2)
		{
			WRITE_TRACE(DBG_FATAL, "Wrong format with =");
			continue;
		}

		m_lstPairs.append( qMakePair( lst[0].trimmed(), lst[1].trimmed() )  );
	}//while

	WRITE_TRACE(DBG_FATAL, "tester config contains %d valid entries:", m_lstPairs.size() );

	for(int i=0; i< m_lstPairs.size(); i++)
	{
		WRITE_TRACE(DBG_FATAL, "'%s' ==> '%s'"
			, QSTR2UTF8( m_lstPairs[i].first )
			, QSTR2UTF8( m_lstPairs[i].second)
		);
	}

	return true;
}

bool TestConfig::isPresent()
{
	 return QFile( ParallelsDirs::getPathToDispatcherTesterConfig() ).exists();
}

bool TestConfig::getParamByName( const QString& param, QString& out_val )
{
	for(int i=0; i<m_lstPairs.size(); i++)
	{
		if( m_lstPairs[i].first != param )
			continue;

		 out_val = m_lstPairs[i].second;
		 return true;
	}

	return false;
}

bool TestConfig::getParamByName( const QString& param, int& out_val )
{
	QString val;
	if( !TestConfig::getParamByName(param, val) )
		return false;

	bool ok=false;
	out_val = val.toInt(&ok);
	return ok;
}

bool TestConfig::getParamByName( const QString& param, QStringList& out_lst )
{
	QString str;
	if( !getParamByName( param, str) )
		return false;

	out_lst  = str.split( ",", QString::SkipEmptyParts );

	// remove white spaces:
	for( QStringList::iterator it = out_lst.begin(); it!=out_lst.end(); it ++)
	{
		*it = it->trimmed();
		if( it->isEmpty() )
			it = out_lst.erase(it);
	}

	if( out_lst.isEmpty() )
		return false;

	return true;
}

