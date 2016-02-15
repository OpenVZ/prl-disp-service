///////////////////////////////////////////////////////////////////////////////
///
/// @file Buse.h
///
/// BUSE filesystem helper
///
/// @author ibazhitov@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef _BUSE_LIB_H_
#define _BUSE_LIB_H_

#include <memory>
#include <QString>

#include <prlsdk/PrlTypes.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>

namespace Buse {

struct Failure
{
	explicit Failure(CVmEvent *e = NULL): m_code(PRL_ERR_SUCCESS), m_event(e)
	{
	}

	void setTarget(CVmEvent *e)
	{
		m_event = e;
	}

	Failure& code(PRL_RESULT code_);
	PRL_RESULT operator()();
	PRL_RESULT operator()(PRL_RESULT code_)
	{
		return code(code_)();
	}
	PRL_RESULT operator()(const QString& first_);
	PRL_RESULT operator()(PRL_RESULT code_, const QString& first_)
	{
		return code(code_)(first_);
	}

private:
	PRL_RESULT m_code;
	CVmEvent* m_event;
};

/** An entry in the BUSE filesystem */
struct Entry {
	/**
	 * Constructor with parameters
	 *
	 * @param name - entry name
	 * @param mp - BUSE filesystem mountpoint
	 * @param e - where to store error code and params
	 */
	Entry(const QString& name, const QString& mp, CVmEvent *e = NULL)
		: m_path(QDir(mp).filePath(name)), m_failure(e)
	{
	}

	QString name() const;
	QString content() const;

	PRL_RESULT setSource(const QString& path);
	PRL_RESULT setParams(const QString& params);
	PRL_RESULT setFormat(const QString& fmt);

private:
	PRL_RESULT get(const QString& file, QString& value);
	PRL_RESULT set(const QString& file, const QString& value);

private:
	/** Full path to the entry */
	QString m_path;
	/** Result of the last operation */
	Failure m_failure;
};

/**
 * BUSE filesystem helper
 *
 * BUSE (Backup in USErspace) filesystem contains simple two level directory hierarchy:
 * /-
 *  |- entry_name
 *      |- content      - backup content, i.e. raw disk image
 *      |- description  - internal BUSE description
 *      |- format       - backup format, implemented in a plugin, e.g. "tib"
 *      |- params       - BUSE parameters, that are needed to correctly handle the backup
 *      |- source       - full path to the backup file
 *      |- ...          - plugin-specific files
 *  |- entry_name
 *  ...
 *
 *  Root directory contains directories (entries), each of which represents a single
 *  mounted backup file (e.g. tib file). Each entry contains a set of at least 5 files
 *  described above. Entry is created using mkdir(2) and removed using rmdir(2).
 *  Initially, the 'content' file is empty. Writing the path to a backup file
 *  into the 'source' file will attach the backup contents to the 'content' file.
 *  After that, the backup contents could be read from the 'content' file using read(2).
 */
struct Buse {
	Buse(const QString& mountpoint = QString());
	Buse& setErrorEvent(CVmEvent *e);
	PRL_RESULT exists(const QString& name);
	Entry *create(const QString& name, CVmEvent *e = NULL);
	PRL_RESULT getEntryName(const QString& path, QString& name);
	PRL_RESULT remove(const QString& name);
	PRL_RESULT find(const QString& what, QStringList& found);
	static QString getEntryName(const QString& path);

	const QString& mountpoint() const
	{
		return m_mountpoint;
	}

private:
	static QString findMountpoint();

private:
	/** Where the BUSE filesystem is mounted */
	QString m_mountpoint;
	/** Result of the last operation */
	Failure m_failure;
};

/** Interface for backup format handler */
struct Format {
	virtual QString format() const = 0;
	virtual QString params() const = 0;
	virtual QString source(const QString& path) const = 0;
};

/* The following are basic backup formats */

/** Acronis tib format */
struct Tib : public Format {
	Tib(unsigned int idx, unsigned int pit, unsigned int head = 0, unsigned int tail = 0)
		: m_idx(idx), m_pit(pit), m_head(head), m_tail(tail)
	{
	}

	void setHead(unsigned int head)
	{
		m_head = head;
	}

	void setTail(unsigned int tail)
	{
		m_tail = tail;
	}

	QString params() const
	{
		return QString("idx=%1 pit=%2 head=%3 tail=%4")
			.arg(m_idx).arg(m_pit).arg(m_head).arg(m_tail);
	}

	QString format() const
	{
		return "tib";
	}

	QString source(const QString& path) const
	{
		return path;
	}

private:
	unsigned int m_idx;
	unsigned int m_pit;
	unsigned int m_head;
	unsigned int m_tail;
};

/* The following are the decorators for basic format types */

/** Backup residing on a locally mounted filesystem */
struct Local : public Format {
	/**
	 * Constructor with parameters
	 *
	 * @param format - basic backup format
	 * @param path - full path to the backups directory, e.g. /var/parallels/backups
	 */
	Local(Format& format, const QString& path) : m_format(format), m_path(path)
	{
	}

	QString format() const
	{
		return m_format.format();
	}

	QString params() const
	{
		return m_format.params();
	}

	QString source(const QString& path) const
	{
		return QFileInfo(m_path, m_format.source(path)).filePath();
	}

private:
	/** Basic backup format */
	Format& m_format;
	/** Full path to the backups directory */
	QString m_path;
};

/** Backup located on a remote server */
struct Remote : public Format {
	/**
	 * Constructor with parameters
	 *
	 * @param format - basic backup format
	 */
	Remote(Format& format) : m_format(format)
	{
	}

	/**
	 * Constructor with parameters
	 *
	 * @param format - basic backup format
	 * @param socket - path to a unix socket
	 * @param cookie - unique identifier
	 */
	Remote(Format& format, const QString& socket, const QString& cookie)
		: m_format(format), m_socket(socket), m_cookie(cookie)
	{
	}

	/**
	 * Set the path to a unix socket
	 *
	 * @param sock - path to a unix socket
	 * @return object
	 */
	Remote& setSocket(const QString& sock)
	{
		m_socket = sock;
		return *this;
	}

	/**
	 * Set the unique cookie
	 *
	 * @param cookie - unique cookie
	 * @return object
	 */
	Remote& setCookie(const QString& cookie)
	{
		m_cookie = cookie;
		return *this;
	}

	QString format() const
	{
		return QString("remote remote.ssl=on remote.socket=\"%1\" remote.cookie=\"%2\" %3")
			.arg(m_socket, m_cookie, m_format.format());
	}

	QString params() const
	{
		return m_format.params();
	}

	QString source(const QString& path) const
	{
		return m_format.source(path);
	}

private:
	/** Basic backup format */
	Format& m_format;
	/** Path to a unix socket */
	QString m_socket;
	/** Unique identifier for this entry */
	QString m_cookie;
};

} // namespace Buse

#endif // _BUSE_LIB_H_
