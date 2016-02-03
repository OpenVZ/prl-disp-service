///////////////////////////////////////////////////////////////////////////////
///
/// @file CVcmmdInterface
///
/// Implements class VcmmdInterface
///
/// @author mnestratov
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
#pragma once

#include <QString>
#include <QScopedPointer>
#include <QSharedPointer>
#include <libvcmmd/vcmmd.h>
#include <boost/utility/result_of.hpp>

/*
 * This is a helper class to interact with vcmmd daemon.
 * There are three use case:
 * 1. Registering
 * 2. Unregistering
 * 3. Updating
 *
 * Registering/unregistering is always a transaction. I.e.
 * corresponding preparation call should always be followed by
 * commit call in case of success or revert in case of failure.
 * For instance:
 *  - register(init->VM start OK->activate) or
 *  - register(init->VM start FAIL->deinit)
 *  - unregister(deactivate->stop VM OK->unregister) or
 *  - unregister(deactivate->stop VM fails->activate)
 * The class is written in such a way that it requires explicit
 * calling in case of VM operation succeeds, and all the cleanup
 * will be done implicitly in the class destructor.
 *
 */

namespace Vcmmd
{
///////////////////////////////////////////////////////////////////////////////
// struct Api

struct Api
{
	explicit Api(const QString& uuid_);

	bool init(quint64 limit_, quint64 guarantee_);
	bool update(quint64 limit_, quint64 guarantee_);
	void deinit();
	void activate();
	void deactivate();

private:
	static bool treat(int status_, const char* name_, int level_ = DBG_FATAL);

	QString m_uuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Active

struct Active: std::unary_function<Api, void>
{
	void operator()(argument_type api_)
	{
		api_.deactivate();
	}
	static void clean(argument_type api_)
	{
		api_.activate();
	}
	static void commit(argument_type api_)
	{
		api_.deinit();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Unregistered

struct Unregistered: std::unary_function<Api, bool>
{
	Unregistered(quint64 limit_, quint64 guarantee_):
		m_limit(limit_), m_guarantee(guarantee_)
	{
	}

	result_type operator()(argument_type api_)
	{
		return api_.init(m_limit, m_guarantee);
	}
	static void clean(argument_type api_)
	{
		api_.deinit();
	}
	static void commit(argument_type api_)
	{
		api_.activate();
	}

private:
	quint64 m_limit;
	quint64 m_guarantee;
};

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T, class Enabled = void>
struct Traits;

template<class T>
struct Traits<T, typename boost::enable_if<boost::is_same<void,
			typename boost::result_of<T(Api )>::type> >::type>
{
	static void bind(T flavor_, Api* api_)
	{
		if (NULL != api_)
			flavor_(*api_);
	}
};

template<class T>
struct Traits<T, typename boost::enable_if<boost::is_same<bool,
			typename boost::result_of<T(Api )>::type> >::type>
{
	static bool bind(T flavor_, Api* api_)
	{
		return NULL != api_ && flavor_(*api_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T>
struct Frontend
{
	explicit Frontend(const QString& uuid_): m_api(new Api(uuid_))
	{
	}
	~Frontend()
	{
		if (!m_api.isNull())
			T::clean(*m_api);
	}

	void commit()
	{
		if (!m_api.isNull())
		{
			QScopedPointer<Api> a(m_api.take());
			T::commit(*a);
		}
	}
	typename boost::result_of<T(Api )>::type operator()(T flavor_)
	{
		return Traits<T>::bind(flavor_, m_api.data());
	}

private:
	QScopedPointer<Api> m_api;
};

} // namespace Vcmmd

