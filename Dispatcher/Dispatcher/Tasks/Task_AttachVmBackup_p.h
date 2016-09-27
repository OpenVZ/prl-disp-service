///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_AttachVmBackup_p.h
///
/// Private declarations for backup as a block device attaching tasks
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

#ifndef __Task_AttachVmBackup_p_H_
#define __Task_AttachVmBackup_p_H_

#include <memory>
#include <QtCore>
#include <QLocalServer>
#include <boost/variant.hpp>

#include "CDspClient.h"
#include "CDspTaskHelper.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include "Libraries/Buse/Buse.h"
#include "prlcommon/IOService/IOCommunication/IOClient.h"

namespace Attach {

namespace Wrap {

struct Mixin
{
	Mixin()
	{
	}

	Mixin(const Mixin& src)
	{
		operator=(src);
	}

	Mixin& operator=(const Mixin& src)
	{
		if (this != &src) {
			m_path = src.m_path;
			m_ploop = src.m_ploop;
		}
		return *this;
	}

	Hdd *getResult(const QString& uuid, PRL_VM_TYPE type);

protected:
	QString m_path;
	mutable std::auto_ptr<Ploop> m_ploop;
};

namespace Init {

struct Ct
{
	explicit Ct(const CVmHardDisk& disk) : m_disk(disk)
	{
	}

	QString getPloop() const
	{
		return m_disk.getUserFriendlyName();
	}

	QString getPath() const
	{
		return m_disk.getUserFriendlyName();
	}

private:
	const CVmHardDisk& m_disk;
};

struct Vm
{
	Vm(const CVmHardDisk& disk, const QString& home);
	QString getPloop() const;

	QString getPath() const
	{
		return m_path;
	}

private:
	QString m_home;
	QString m_path;
};

template<class T>
struct Builder : Mixin
{
	Builder(T policy) : m_policy(policy)
	{
	}

	PRL_RESULT setPath();
	PRL_RESULT setPloop();
	PRL_RESULT prepareDevice();

private:
	T m_policy;
};

}

namespace Create {

struct Common: Mixin
{
	Common(const QString& path, const QString& image)
		: m_image(image), m_auth(NULL)
	{
		m_path = path;
	}

	PRL_RESULT setPloop();
	PRL_RESULT prepareDevice();

protected:
	CAuthHelper *getAuth() const
	{
		return m_auth;
	}

	void setAuth(CAuthHelper& auth)
	{
		m_auth = &auth;
	}

	void setWrapper(const QString& value)
	{
		m_wrapper = value;
	}

private:
	QString m_image;
	QString m_wrapper;
	CAuthHelper *m_auth;
};

struct Ct: Common
{
	Ct(const QString& path, const QString& image)
		: Common(path, image)
	{
	}

	PRL_RESULT setPath()
	{
		setWrapper(m_path);
		return PRL_ERR_SUCCESS;
	}
};

struct Vm: Common
{
	Vm(const QString& path, const QString& image, CAuthHelper& auth)
		: Common(path, image)
	{
		setAuth(auth);
	}

	PRL_RESULT setPath();
};

} // namespace Create

} // namespace Wrap

namespace Source {

/** Data source flavours */
struct Flavor {
	typedef boost::variant<Buse::Tib, Buse::Qcow> format_type;

	Flavor(Buse::Entry *entry, const BackupInfo& backup);
	PRL_RESULT attachLocal(const QString& path);
	PRL_RESULT attachRemote(SmartPtr<IOClient> client, CDspTaskHelper *task);
	void mangle();

private:
	/** BUSE entry to which a backup will be connected */
	Buse::Entry *m_entry;
	/** path to a backup file */
	QString m_path;
	/** base backup format handler */
	format_type m_format;
};

namespace Visitor
{

///////////////////////////////////////////////////////////////////////////////
// struct Local

struct Local : boost::static_visitor<PRL_RESULT>
{
	Local(Buse::Entry& entry_, const QString& path_)
		: m_entry(entry_), m_path(path_)
	{
	}

	template <class T>
	PRL_RESULT operator()(const T& format_) const;

private:
	Buse::Entry& m_entry;
	QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mangle

struct Mangle : boost::static_visitor<void>
{
	template <class T>
	void operator()(T& format_) const;
};

} // namespace Visitor
} // namespace Source

} // namespace Attach

#endif // __Task_AttachVmBackup_p_H_
