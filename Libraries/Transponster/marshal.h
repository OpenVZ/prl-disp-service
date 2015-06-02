/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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

#ifndef __MARSHAL_H__
#define __MARSHAL_H__
#include "base.h"
#include "enum.h"
#include <boost/type_traits/is_enum.hpp>

namespace Libvirt
{
namespace Details
{
///////////////////////////////////////////////////////////////////////////////
// struct Marshal

struct Marshal
{
	template<class T, class U>
	static typename boost::enable_if<Validatable<T>, int>::type
		assign(const U& value_, Access<T>& dst_)
	{
		if (dst_.setValue(value_))
			return 1;

		return -1;
	}

	template<class T, class U>
	static typename boost::disable_if<Validatable<T>, int>::type
		assign(const U& value_, Access<T>& dst_)
	{
		dst_.setValue(value_);
		return 1;
	}

};

} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// struct Marshal

template<class T, class Enabled = void>
struct Marshal
{
	static QString getString(const Access<T>& access_)
	{
		return Traits<T>::generate(access_.getValue());
	}
	static int setString(QString value_, Access<T>& access_)
	{
		typename Details::Value::Grab<Access<T> >::type x;
		if (!Traits<T>::parse(value_, x))
			return -1;

		return Details::Marshal::assign(x, access_);
	}
};

template<int I>
struct Marshal<mpl::int_<I> >
{
	static QString getString(const Access<mpl::int_<I> >& )
	{
		return getText(I);
	}
	static int setString(QString value_, Access<mpl::int_<I> >& access_)
	{
		if (getString(access_) == value_.simplified())
			return 1;

		return -1;
	}
};

template<class T>
struct Marshal<T, typename boost::enable_if<boost::is_same<QString, typename Value<T>::type> >::type>
{
	static QString getString(const Access<T>& access_)
	{
		return access_.getValue();
	}
	static int setString(const QString& value_, Access<T>& access_)
	{
		return Details::Marshal::assign(value_, access_);
	}
};

template<class T>
struct Marshal<T, typename boost::enable_if<boost::is_enum<T> >::type>
{
private:
	typedef typename Enum<T>::data_type data_type;
	typedef typename Enum<T>::name_type name_type;

	static data_type s_data;

public:
	static QString getString(const Access<T>& access_)
	{
		return s_data.left.at(access_.getValue());
	}
	static int setString(name_type value_, Access<T>& access_)
	{
		typename data_type::right_map::const_iterator p =
				s_data.right.find(value_.simplified());
		if (s_data.right.end() == p)
			return -1;

		access_.setValue(p->second);
		return 1;
	}
};

template<class T>
typename Marshal<T, typename boost::enable_if<boost::is_enum<T> >::type>::data_type
Marshal<T, typename boost::enable_if<boost::is_enum<T> >::type>::s_data =
	Enum<T>::getData();

} // namespace Libvirt

#endif // __MARSHAL_H__

