////////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirt_p.h
///
/// Private interfaces of the VM configuration validation.
///
/// @author dandreev
///
/// Copyright (c) 2015-2015 Parallels IP Holdings GmbH
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
////////////////////////////////////////////////////////////////////////////////

#ifndef _VM_VALIDATE_CONFIG_P_H_
#define _VM_VALIDATE_CONFIG_P_H_

#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include <QSet>
#include <QMap>
#include <boost/optional/optional.hpp>
#include <boost/mpl/vector.hpp>

class CSavedStateTree;
class CDspClient;

namespace Validation
{
////////////////////////////////////////////////////////////////////////////////
// struct

struct Error
{
	QStringList getParams() const
	{
		return m_params;
	}

protected:
	QStringList m_params;
};

////////////////////////////////////////////////////////////////////////////////
// struct VmNameEmpty

struct VmNameEmpty: Error
{
};

////////////////////////////////////////////////////////////////////////////////
// struct VmNameInvalidSymbols

struct VmNameInvalidSymbols: Error
{
	explicit VmNameInvalidSymbols(const QString& name_)
	{
		m_params << name_;
	}
};

////////////////////////////////////////////////////////////////////////////////
// struct VmNameLength

struct VmNameLength: Error
{
	explicit VmNameLength(const int len_)
	{
		m_params << QString::number(len_);
	}
};

////////////////////////////////////////////////////////////////////////////////
// struct Traits

template <class T>
struct Traits
{
	typedef boost::optional<T> check_type;

	static check_type check(const CVmConfiguration& vm_);
	static QSet<QString> getIds(const CVmConfiguration& vm_);
	static PRL_RESULT getError();
};

////////////////////////////////////////////////////////////////////////////////
// struct Sink

struct Sink
{
	Sink(CVmConfiguration& config_,  QList<PRL_RESULT>& results_,
		QMap<int, QStringList>& paramsMap_, QMap<int, QSet<QString> >& idsMap_)
	: m_config(&config_), m_results(&results_), m_paramsMap(&paramsMap_), m_idsMap(&idsMap_)
	{
	}
	
	template <class T>
	void operator()(const T&);

private:

	CVmConfiguration* m_config;
	QList<PRL_RESULT>* m_results;
	QMap<int, QStringList>* m_paramsMap;
	QMap<int, QSet<QString> >* m_idsMap;
};

} // namespace Validation

#endif // _VM_VALIDATE_CONFIG_P_H_
