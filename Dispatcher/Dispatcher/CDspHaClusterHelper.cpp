///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHaClusterHelper.cpp
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
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
/// HA cluster support for Paralles Cloud Server
/// @author krasnov
///
/////////////////////////////////////////////////////////////////////////////////

#include "CDspService.h"
#include <prlcommon/PrlCommonUtilsBase/Common.h>
#include <prlsdk/PrlCommandsFlags.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include "CDspHaClusterHelper.h"

#define HAMAN_BIN "/usr/sbin/shaman"
#define HAMAN_START_TMO 30000 // 30 min to start
#define HAMAN_EXEC_TMO 120000 // 2 min to execution

#define PSTORAGE_BIN "/usr/bin/pstorage"

#define SHAMAN_CMD_ADD	"add"
#define SHAMAN_CMD_DEL	"del"
#define SHAMAN_CMD_SET	"set"
#define SHAMAN_CMD_DEL_EVERYWHERE	"del-everywhere"
#define SHAMAN_CMD_FROM	"move-from"
#define SHAMAN_CMD_TO	"move-to"
#define SHAMAN_CMD_INFO	"info"

QString CDspHaClusterHelper::getResourcePrefix(PRL_VM_TYPE vmType)
{
	switch (vmType) {
	case PVT_VM:
		return QString("vm-");
	case PVT_CT:
		return QString("ct-");
	}
	return "";
}

PRL_UINT32 CDspHaClusterHelper::getStartCommandFlags(CProtoCommandPtr pCmd)
{
	if (!pCmd->IsValid())
		return 0;

	if (pCmd->GetCommandId() != PVE::DspCmdVmStartEx)
		return 0;

	CProtoVmStartExCommand* pCmdEx =
		CProtoSerializer::CastToProtoCommand<CProtoVmStartExCommand>(pCmd);
	if ((PRL_VM_START_MODE)pCmdEx->GetStartMode() != PSM_VM_START)
		return 0;

	return pCmdEx->GetReservedParameter();
}

PRL_RESULT CDspHaClusterHelper::addClusterResource(const QString & sName,
		CVmHighAvailability *ha, const QString & sPath)
{
	PRL_ASSERT(ha);

	if (!ha->isEnabled())
		return PRL_ERR_SUCCESS;

	// handle only VM on shared FS - nfs, gfs, gfs2, pcs
	if (!CFileHelper::isSharedFS(sPath))
		return PRL_ERR_SUCCESS;

	QProcess proc;
	QStringList args;
	args += SHAMAN_CMD_ADD;
	args += getResourcePrefix() + sName;
	args += QString("--prio");
	args += QString("%1").arg(ha->getPriority());
	args += QString("--path");
	args += sPath;

	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "cluster resource '%s' registration error", QSTR2UTF8(sName));
	return res;
}

PRL_RESULT CDspHaClusterHelper::renameClusterResource(const QString & oldName,
		CVmHighAvailability *oldHa, const QString & newName, const QString & newPath)
{
	PRL_ASSERT(oldHa);

	if (!oldHa->isEnabled())
		return PRL_ERR_SUCCESS;

	PRL_RESULT res = addClusterResource(newName, oldHa, newPath);
	if (PRL_FAILED(res))
		return res;

	/* Ignore errors on remove, can't do anything with them. */
	removeClusterResource(oldName);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspHaClusterHelper::updateClusterResourceParams(const QString & sName,
		CVmHighAvailability *oldHa, CVmHighAvailability *newHa, const QString & newPath, PRL_VM_TYPE vmType)
{
	QProcess proc;
	QStringList args;
	QString cmd;

	PRL_ASSERT(oldHa);
	PRL_ASSERT(newHa);

	if (newHa->isEnabled() != oldHa->isEnabled())
		cmd = newHa->isEnabled() ? SHAMAN_CMD_ADD : SHAMAN_CMD_DEL;
	else if (newHa->getPriority() != oldHa->getPriority()) {
		cmd = SHAMAN_CMD_SET;
	} else {
		/* HA options are not changed */
		return PRL_ERR_SUCCESS;
	}

	/*
	 * Don't run shaman if HA is disabled in VM config and the command line
	 * doesn't contain '--ha-enable yes'.
	 */
	if ((newHa->isEnabled() == oldHa->isEnabled()) && !newHa->isEnabled())
		return PRL_ERR_SUCCESS;

	args += cmd;
	args += getResourcePrefix(vmType) + sName;

	if (cmd != SHAMAN_CMD_DEL) {
		/*
		 * Specify all parameters from the config when doing 'shaman add'.
		 * This is needed e.g. when registering an already existing VM - newly
		 * created cluster resource for this VM should contain all actual
		 * HA parameter values.
		 */
		if ((newHa->getPriority() != oldHa->getPriority()) || (cmd == SHAMAN_CMD_ADD)) {
			args += QString("--path");
			args += newPath;
			args += QString("--prio");
			args += QString("%1").arg(newHa->getPriority());
		}
	}

	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "cluster resource '%s' registration error", QSTR2UTF8(sName));
	return res;
}

PRL_RESULT CDspHaClusterHelper::removeClusterResource(const QString & sName, bool removeFromAllNodes)
{
	QProcess proc;
	QStringList args;
	args += removeFromAllNodes ? SHAMAN_CMD_DEL_EVERYWHERE : SHAMAN_CMD_DEL;
	args += getResourcePrefix() + sName;

	WRITE_TRACE(DBG_FATAL, "%s removing resource %s", removeFromAllNodes ? "recursively" : "", QSTR2UTF8(sName));

	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "cluster resource '%s' removing error", QSTR2UTF8(sName));
	return res;
}

PRL_RESULT CDspHaClusterHelper::moveFromClusterResource(const QString & sName, const QString & sRemoteNode)
{
	QProcess proc;
	QStringList args;
	args += SHAMAN_CMD_FROM;
	args += getResourcePrefix() + sName;
	args += sRemoteNode;

	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "cluster resource '%s' removing error", QSTR2UTF8(sName));
	return res;
}

PRL_RESULT CDspHaClusterHelper::moveToClusterResource(const QString & sName, const QString & sRemoteNode)
{
	QProcess proc;
	QStringList args;
	args += SHAMAN_CMD_TO;
	args += getResourcePrefix() + sName;
	args += sRemoteNode;

	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "cluster resource '%s' removing error", QSTR2UTF8(sName));
	return res;
}

PRL_RESULT CDspHaClusterHelper::runProgram(const QString & sPath, const QStringList & lstArgs, QProcess & proc)
{
	proc.start(sPath, lstArgs);
	if (!proc.waitForStarted(HAMAN_START_TMO)) {
		WRITE_TRACE(DBG_FATAL, "Failed to run %s : %s",
				QSTR2UTF8(sPath), QSTR2UTF8(proc.errorString()));
		return PRL_ERR_CLUSTER_RESOURCE_ERROR;
	}
	if (!proc.waitForFinished(HAMAN_EXEC_TMO)) {
		WRITE_TRACE(DBG_FATAL, "Failed to wait for finished %s : %s",
				QSTR2UTF8(sPath), QSTR2UTF8(proc.errorString()));
		proc.terminate();
		return PRL_ERR_CLUSTER_RESOURCE_ERROR;
	}
	if (proc.exitCode()) {
		WRITE_TRACE(DBG_FATAL, "%s failed : retcode : %d, stdout: [%s] stderr: [%s]",
			QSTR2UTF8(sPath),
			proc.exitCode(),
			proc.readAllStandardOutput().constData(),
			proc.readAllStandardError().constData()
			);
		return PRL_ERR_CLUSTER_RESOURCE_ERROR;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspHaClusterHelper::revokeLeases(const QString & sPath)
{
	QProcess proc;
	QStringList args;

	if ( !CDspService::isServerModePSBM() )
		return PRL_ERR_SUCCESS;

	QFileInfo fi(PSTORAGE_BIN);
	if (!fi.exists()) {
		WRITE_TRACE(DBG_FATAL, "Failed to run %s : file not found", QSTR2UTF8(fi.filePath()));
		return PRL_ERR_CLUSTER_RESOURCE_ERROR;
	}

	args += "revoke";
	args += "-R";
	args += sPath;

	return runProgram(fi.filePath(), args, proc);
}

PRL_RESULT CDspHaClusterHelper::runHaman(const QStringList & args, QProcess & proc)
{
	QStringList lstArgs(args);

	// checks
	if ( !CDspService::isServerModePSBM() )
		return PRL_ERR_SUCCESS;

	QFileInfo fi(HAMAN_BIN);
	if (!fi.exists())
		return PRL_ERR_SUCCESS;

	lstArgs.prepend("-i");
	lstArgs.prepend("-q");

	return runProgram(fi.filePath(), lstArgs, proc);
}

PRL_RESULT CDspHaClusterHelper::getHaClusterID(QString & sHaClusterID)
{
	QStringList args;
	QProcess proc;
	args += SHAMAN_CMD_INFO;

	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "can not get cluster info");
		return res;
	}

	QList<QByteArray> lstOut = proc.readAllStandardOutput().split('\n');
	QByteArray token = "ID :";
	foreach(QByteArray entry, lstOut) {
		if (entry.startsWith(token)) {
			sHaClusterID = QString(entry.right(entry.size() - token.size()).trimmed());
			break;
		}
	}

	return PRL_ERR_SUCCESS;
}

QFileInfoList CDspHaClusterHelper::getReport()
{
	QFileInfoList output;
	if (!CDspService::isServerModePSBM())
		return output;
	QFileInfo p("/usr/bin/pstorage-make-report");
	if (!p.exists())
		return output;
	QDir d("/etc/pstorage/clusters");
	if (!d.exists())
		return output;
	QStringList a = d.entryList(QDir::NoDotAndDotDot | QDir::Dirs);
	foreach (QString x, a)
	{
		QTemporaryFile t;
		t.setFileTemplate(QString("%1/pstorage.%2.XXXXXX.tgz")
			.arg(QDir::tempPath()).arg(x));
		if (!t.open())
		{
			WRITE_TRACE(DBG_FATAL, "QTemporaryFile::open() error: %s",
					QSTR2UTF8(t.errorString()));
			continue;
		}
		QString b, c = QString("%1 -f %2 \"%3\"").arg(p.filePath()).arg(t.fileName()).arg(x);
		if (!HostUtils::RunCmdLineUtility(c, b, -1) || t.size() == 0)
		{
			t.close();
			continue;
		}
		t.setAutoRemove(false);
		output.append(QFileInfo(t.fileName()));
		t.close();
	}
	return output;
}

PRL_RESULT CDspHaClusterHelper::updateClusterResourcePath(const QString& name,
	const QString& path)
{
	QStringList args;
	args += SHAMAN_CMD_SET;
	args += getResourcePrefix() + name;
	args += QString("--path");
	args += path;

	QProcess proc;
	PRL_RESULT res = runHaman(args, proc);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "failed to set path for resource '%s'", QSTR2UTF8(name));
	return res;
}
