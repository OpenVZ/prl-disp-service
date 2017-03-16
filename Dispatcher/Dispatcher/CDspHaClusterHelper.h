///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHaClusterHelper.cpp
///
/// Copyright (c) 2012-2017, Parallels International GmbH
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
/// HA cluster support for Paralles Cloud Server
/// @author krasnov
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspHaClusterHelper_H_
#define __CDspHaClusterHelper_H_

class CDspHaClusterHelper
{
public:
	CDspHaClusterHelper() {;}
	~CDspHaClusterHelper() {;}

public:
	PRL_UINT32 getStartCommandFlags(CProtoCommandPtr pCmd);
	PRL_RESULT addClusterResource(const QString & sName, CVmHighAvailability *ha, const QString & sPath);
	PRL_RESULT renameClusterResource(const QString & oldName,
			CVmHighAvailability *oldHa, const QString & newName, const QString & newPath);
	PRL_RESULT updateClusterResourceParams(const QString & sName,
			CVmHighAvailability *oldHa, CVmHighAvailability *newHa, const QString &newPath,
			PRL_VM_TYPE vmType = PVT_VM);
	PRL_RESULT removeClusterResource(const QString & sName, bool removeFromAllNodes = false);

	PRL_RESULT getHaClusterID(QString & sHaClusterID);
	PRL_RESULT moveFromClusterResource(const QString & sName, const QString & sRemoteNode);
	PRL_RESULT moveToClusterResource(const QString & sName, const QString & sRemoteNode);

	PRL_RESULT revokeLeases(const QString & sPath);
	static QFileInfoList getReport();
	PRL_RESULT updateClusterResourcePath(const QString& name, const QString& path);
private:
	PRL_RESULT runProgram(const QString & sPath, const QStringList & lstArgs, QProcess & proc);
	PRL_RESULT runHaman(const QStringList & args, QProcess & proc);
	QString getResourcePrefix(PRL_VM_TYPE vmType = PVT_VM);
};

#endif // __CDspHaClusterHelper_H_
