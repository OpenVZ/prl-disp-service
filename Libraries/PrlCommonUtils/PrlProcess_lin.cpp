//////////////////////////////////////////////////////////////////////////
///
/// @file PrlProcess.cpp
///
/// @brief PrlProcess type definition
///
/// @author shrike@
///
///
/// Copyright (c) 2011-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
//////////////////////////////////////////////////////////////////////////

#include "PrlProcess.h"
#include <errno.h>
#include <fcntl.h>
#include <QVector>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
#include <QByteArray>
#include <prlcommon/Logging/Logging.h>
#include "Libraries/Virtuozzo/CVzHelper.h"

namespace
{
	struct Descriptor
	{
		explicit Descriptor(int _handle): m_handle(_handle)
		{
		}
		bool copy(int _dst)
		{
			return 0 == ::close(_dst) &&
				::dup2(m_handle, _dst) >= 0;
		}
		bool nullify()
		{
			int null = ::open("/dev/null", O_RDWR);
			if (-1 == null)
				return false;

			Descriptor n(null);
			bool output = n.copy(m_handle);
			n.close();
			return output;
		}
		void close()
		{
			int b = errno;
			::close(m_handle);
			errno = b;
		}
	private:
		int m_handle;
	};
	QVector<QByteArray> parseCombinedArgString(const QString &program)
	{
		QString tmp;
		int quoteCount = 0;
		bool inQuote = false;
		QVector<QByteArray> output;

		for (int i = 0; i < program.size(); ++i)
		{
			if (program.at(i) == QLatin1Char('"'))
			{
				++quoteCount;
				if (quoteCount == 3)
				{
					quoteCount = 0;
					tmp += program.at(i);
				}
				continue;
			}
			if (quoteCount)
			{
				if (quoteCount == 1)
					inQuote = !inQuote;
				quoteCount = 0;
			}
			if (!inQuote && program.at(i).isSpace())
			{
				if (!tmp.isEmpty())
				{
					output.append(tmp.toUtf8());
					tmp.clear();
				}
			}
			else
				tmp += program.at(i);
		}
		if (!tmp.isEmpty())
			output.append(tmp.toUtf8());

		return output;
	}
	QVector<QByteArray> createEnvList(const QStringList &lstEnv)
	{
		QVector<QByteArray> envlist;

		// add PATH if necessary
		if (lstEnv.filter(QRegExp("^PATH=", Qt::CaseInsensitive)).isEmpty())
		{
			QByteArray path = qgetenv("PATH");
			if (!path.isEmpty()) {
				QString tmp = QString("PATH=%1").arg(QString::fromLocal8Bit(path));
				envlist.append(tmp.toUtf8());
			}
		}
		foreach(QString env, lstEnv)
		{
			envlist.append(env.toUtf8());
		}
		return envlist;
	}
}; // namespace

namespace Details
{
namespace PrlProcess
{
	ChildProcess::~ChildProcess()
	{
		Descriptor(m_channel).close();
	}
	bool ChildProcess::wait(int& _code, QProcess::ExitStatus& _status)
	{
		int p, s = 0;
		while ((p = ::waitpid(m_pid, &s, 0)) < 0 && errno == EINTR) ;
		if (p < 0 && errno == ECHILD)
		{
			_code = 0;
			_status = QProcess::NormalExit;
			return true;
		}
		if (p == m_pid)
		{
			_code = WEXITSTATUS(s);
			_status = WIFEXITED(s) ? QProcess::NormalExit :
						QProcess::CrashExit;
			return true;
		}
		return false;
	}
	qint64 ChildProcess::write(const char* _data)
	{
		return ::write(m_channel, _data, qstrlen(_data));
	}
}; // namespace PrlProcess
}; // namespace Details

void PrlProcess::start(const QString& _program)
{
	if (!m_child.isNull())
		return;

	m_error = QProcess::FailedToStart;
	m_exitCode = 0;
	m_exitStatus = QProcess::NormalExit;
	int w[2];
	if (0 > ::pipe(w))
	{
		WRITE_TRACE(DBG_FATAL, "pipe failed! error %d,", errno);
		return;
	}
	QVector<char*> a;
	QVector<QByteArray> qa = parseCombinedArgString(_program);
	for (int i = 0; i < qa.size(); a.append(qa[i++].data())) ;
	a.append(NULL);

	QVector<char*> e;
	QVector<QByteArray> qe = createEnvList(m_lstEnv);
	for (int i = 0; i < qe.size(); e.append(qe[i++].data())) ;
	e.append(NULL);

	int errno_ = 0;

	pid_t p = vfork();
	if (0 > p)
	{
		WRITE_TRACE(DBG_FATAL, "vfork failed! error %d,", errno);
		return;
	}
	if (0 == p)
	{
		errno_ = doChild(w, m_username, a.data(), e.data());
		::_exit(-1);
	}
	::close(w[0]);
	m_child.reset(new Details::PrlProcess::ChildProcess(p, w[1]));
	if (0 == errno_)
		m_error = QProcess::UnknownError;
	else
	{
		WRITE_TRACE(DBG_FATAL, "child failed! error %d,", errno_);
		waitForFinished(-1);
	}
};

int PrlProcess::kill()
{
	if (!m_child.isNull())
		return ::kill(m_child->getpid(), SIGKILL);
	return 0;
}

bool PrlProcess::waitForFinished(int )
{
	if (m_child.isNull())
		return true;

	bool output = m_child->wait(m_exitCode, m_exitStatus);
	if (output)
		m_child.reset();

	return output;
};

qint64 PrlProcess::write(const char* _data)
{
	return m_child.isNull() ? 0 : m_child->write(_data);
}

int PrlProcess::doChild(int _channel[2], const QString &username, char* const* _argv, char* const* _envp)
{
	if (!username.isEmpty()) {
		struct passwd *pResultUserInfo = NULL;
		struct passwd userInfo;
		QByteArray _passwd_strings_buf;
		_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

		::getpwnam_r(username.toUtf8().constData(), &userInfo, _passwd_strings_buf.data(), _passwd_strings_buf.size(),
				&pResultUserInfo );

		if (::initgroups(pResultUserInfo->pw_name, pResultUserInfo->pw_gid) != 0)
		{
			int nErrno = errno;
			WRITE_TRACE(DBG_FATAL, "initgroups() call failed with errno=%d", nErrno);
			return nErrno;
		}
		if (::setregid(pResultUserInfo->pw_gid, pResultUserInfo->pw_gid ) != 0)
		{
			int nErrno = errno;
			WRITE_TRACE(DBG_FATAL, "setregid() call failed with errno=%d", nErrno);
			return nErrno;
		}
		if (::setreuid(pResultUserInfo->pw_uid, pResultUserInfo->pw_uid ) != 0)
		{
			int nErrno = errno;
			WRITE_TRACE(DBG_FATAL, "setreuid() call failed with errno=%d", nErrno);
			return nErrno;
		}
	}
	Descriptor(_channel[1]).close();
	Descriptor c(_channel[0]);
	bool f = c.copy(STDIN_FILENO) &&
		Descriptor(STDOUT_FILENO).nullify() &&
		Descriptor(STDERR_FILENO).nullify();
	c.close();
	if (f)
		execve(_argv[0], _argv, _envp);
	return errno;
}

