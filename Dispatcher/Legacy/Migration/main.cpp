///////////////////////////////////////////////////////////////////////////////
///
/// @file main.cpp
///
/// Copyright (c) 2016 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#include <QCoreApplication>
#include <CVmMigrateTargetServer.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	ParallelsDirs::Init(PAM_SERVER);
	QFile f(ParallelsDirs::getDispatcherConfigFilePath());
	CDispatcherConfig c(&f);
	SetLogFileName(GetDefaultLogFilePath(), "prl-disp.log");
	WRITE_TRACE(DBG_FATAL, "file %s", qPrintable(ParallelsDirs::getDispatcherConfigFilePath()));
	if (c.getDispatcherSettings()->getCommonPreferences()->getDebug()->isVerboseLogEnabled())
		SetLogLevel(DBG_DEBUG);	
	
	CVmMigrateTargetServer s;
	int r = -1;
	if (s.connectToDisp())
		r = app.exec();

	// sleep is required because Vz6 vm_app doesn't instantly
	// disconnect it's slot from clientDisconnected signal.
	HostUtils::Sleep(500);
	return r;
}
