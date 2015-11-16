/*
 * SysError.h: error reference crossplatform functions
 *
 * Copyright (C) 1999-2014 Parallels IP Holdings GmbH
 *
 * This file is part of Parallels SDK. Parallels SDK is free
 * software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */


#ifndef STD_SysError_h__
#define STD_SysError_h__

#include <QString>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <utility>
#include <prlsdk/PrlTypes.h>

namespace Prl
{
QString GetLastErrorAsString();
long GetLastError();

///////////////////////////////////////////////////////////////////////////////
// struct Expected

template <typename T, typename E>
struct Expected
{
	explicit Expected()
	{
	}

	Expected(const T& value_): m_data(value_)
	{
	}

	Expected(const E& error_): m_data(error_)
	{
	}

	bool isFailed() const
	{
		return !isSucceed();
	}

	bool isSucceed() const
	{
		return (m_data.which() == 0);
	}

	const T& value() const
	{
		return boost::get<T>(m_data);
	}

	T& value()
	{
		return boost::get<T>(m_data);
	}

	const E& error() const
	{
		return boost::get<E>(m_data);
	}

	E& error()
	{
		return boost::get<E>(m_data);
	}

private:
	boost::variant<T, E> m_data;
};

template <typename T>
struct Expected<T, T>;
template <>
struct Expected<void, void>;

template <typename E>
struct Expected<void, E>
{
	Expected()
	{
	}

	Expected(const E& error_): m_error(error_)
	{
	}

	bool isFailed() const
	{
		return !isSucceed();
	}

	bool isSucceed() const
	{
		return !m_error;
	}

	const E& error() const
	{
		return *m_error;
	}

	E& error()
	{
		return *m_error;
	}

private:
	boost::optional<E> m_error;
};

} // namespace Prl

#endif //STD_SysError_h__
