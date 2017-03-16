///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlProcess.h
///
/// PrlProcess type declaration
///
/// @author shrike@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __DISK_TOOL_H__
#define __DISK_TOOL_H__

#include <QProcess>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#ifdef _LIN_

namespace Details
{
	namespace PrlProcess
	{
		struct ChildProcess
		{
			ChildProcess(pid_t _pid, int _channel): m_pid(_pid),
						m_channel(_channel)
			{
			}
			~ChildProcess();

			bool wait(int& _code, QProcess::ExitStatus& _status);
			qint64 write(const char* _data);
			pid_t getpid() { return m_pid; }
		private:
			pid_t m_pid;
			int m_channel;
		};
	};
};

struct PrlProcess
{
	PrlProcess(): m_exitCode(0), m_exitStatus(QProcess::NormalExit),
			m_error(QProcess::UnknownError), m_username(QString()),
			m_lstEnv(QStringList())
	{
	}
	~PrlProcess()
	{
		waitForFinished(-1);
	}
	void start(const QString& _program);
	bool waitForStarted(int = 0) const
	{
		return !m_child.isNull();
	};
	bool waitForFinished(int );
	int exitCode() const
	{
		return m_exitCode;
	};
	QProcess::ExitStatus exitStatus() const
	{
		return m_exitStatus;
	}
	QProcess::ProcessError error() const
	{
		return m_error;
	}
	qint64 write(const char* _data);
	void setUser(const QString &username)
	{ m_username = username; }
	int kill();
	void setEnvironment(const QStringList &lstEnv)
	{ m_lstEnv = lstEnv; }
private:
	static int doChild(int _channel[2], const QString &username,
			char* const* _argv, char* const* _envp);

	int m_exitCode;
	QProcess::ExitStatus m_exitStatus;
	QProcess::ProcessError m_error;
	QString m_username;
	QStringList m_lstEnv;
	QScopedPointer<Details::PrlProcess::ChildProcess> m_child;
};

#else
typedef QProcess PrlProcess;
#endif // _LIN_

#endif // __DISK_TOOL_H__

