/*
 * Copyright (c) 2016-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QString>
#include <cxxabi.h>
#include "CDspVmStateMachine.h"
#include <boost/functional/factory.hpp>

namespace Vm
{
namespace State
{
namespace Details
{
QString demangle(const char* name_)
{
	const char* x = name_;
	int status = -1;
	char* y = abi::__cxa_demangle(x, NULL, NULL, &status);
	if (0 != status)
		return x;

	QString output = y;
	free(y);
	return output;
}

} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// struct Started

Started::Started(const editor_type& editor_, const factory_type& factory_):
	m_guard(), m_factory(factory_), m_incarnation(new QAtomicInt()),
	m_editor(new editor_type(editor_))
{
}

void Started::setTools(const tools_type& value_)
{
	m_guard = NULL;
	m_tools = value_;
}

void Started::connect_()
{
	if (!m_editor.isNull())
	{
		connect_(boost::bind(&connector_type::setNetwork, _1,
			boost::bind(boost::factory< ::Vm::Guest::Actor*>(),
				boost::cref(*m_editor))));
	}
}

void Started::connect_(const decorator_type& decorator_)
{
	if (m_incarnation.isNull() || m_factory.empty())
		return;

	m_incarnation.data()->ref();
	QScopedPointer<connector_type> x(m_factory(m_incarnation.toWeakRef()));
	if (x.isNull())
		return;

	m_guard = m_incarnation.data();
	if (!decorator_.empty())
		decorator_(*x);

	(*x)();
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

Frontend::Frontend(const QString& uuid_, const SmartPtr<CDspClient>& user_,
		const QSharedPointer< ::Network::Routing>& routing_):
	m_uuid(uuid_), m_service(CDspService::instance()), m_user(user_),
	m_routing(routing_)
{
	m_vnc = QSharedPointer<Vnc::Secure::Frontend>
		(new Vnc::Secure::Frontend(getConfigEditor(), *m_service));
}

const QString Frontend::getHome() const
{
	if (m_home)
		return m_home->absoluteFilePath();

	return QString();
}

void Frontend::setName(const QString& value_)
{
	m_name = value_;
}

boost::optional<CVmConfiguration> Frontend::getConfig() const
{
	SmartPtr<CVmConfiguration> x(new CVmConfiguration());

	PRL_RESULT r = getHome().isEmpty() ? PRL_ERR_FAILURE :
		// got it from CDspVmDirHelper::getVmConfigForDirectoryItem
		m_service->getVmConfigManager().loadConfig(
			x, getHome(), SmartPtr<CDspClient>(0), true, false);

	if (PRL_SUCCEEDED(r))
		return *x;
	return boost::none;
}

Config::Edit::Atomic Frontend::getConfigEditor() const
{
	return Config::Edit::Atomic(m_uuid, m_user, *m_service);
}

} // namespace State
} // namespace Vm
