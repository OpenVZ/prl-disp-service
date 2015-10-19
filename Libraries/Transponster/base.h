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

#ifndef __BASE_H__
#define __BASE_H__
#include <QString>
#include <QDomNode>
#include <boost/mpl/at.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_fundamental.hpp>

namespace mpl = boost::mpl;

const QString& getText(quint32 );

namespace Libvirt
{
namespace Name
{
///////////////////////////////////////////////////////////////////////////////
// struct Any

struct Any
{
	static bool consume(const QString& )
	{
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Strict

template<unsigned N>
struct Strict
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Scoped

template<unsigned N, unsigned S>
struct Scoped
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T>
struct Traits;

template<unsigned N>
struct Traits<Strict<N> >
{
	static bool consume(const QDomNode& node_)
	{

		QString x = node_.localName();
		if (x.isEmpty())
			x = node_.nodeName();

		return getText(N) == x;
	}
	static const QDomAttr produceAttribute(QDomElement& dst_)
	{
		return dst_.ownerDocument().createAttribute(getText(N));
	}
	static const QDomElement produceElement(QDomDocument& dst_)
	{
		return dst_.createElement(getText(N));
	}
};

template<unsigned N, unsigned S>
struct Traits<Scoped<N, S> >
{
	static bool consume(const QDomNode& node_)
	{
		return node_.localName() == getText(N) &&
			node_.namespaceURI() == getText(S);
	}
	static const QDomAttr produceAttribute(QDomElement& dst_)
	{
		return dst_.ownerDocument().createAttributeNS(getText(S), getText(N));
	}
	static const QDomElement produceElement(QDomDocument& dst_)
	{
		return dst_.createElementNS(getText(S), getText(N));
	}
};

} // namespace Name

///////////////////////////////////////////////////////////////////////////////
// struct Traits forward declaration

template<class T, class Enabled = void>
struct Traits;

///////////////////////////////////////////////////////////////////////////////
// struct Validatable

template<class T>
struct Validatable: mpl::false_
{
};

BOOST_MPL_HAS_XXX_TRAIT_DEF(group_type);
BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type);

namespace Details
{
namespace Group
{
///////////////////////////////////////////////////////////////////////////////
// struct Detect

template<class T, class Enabled = void>
struct Detect: mpl::false_
{
};

template<class T>
struct Detect<T, typename boost::enable_if<has_value_type<T> >::type>: mpl::true_
{
	static const bool value = true;
};

template<class T>
struct Detect<T, typename boost::enable_if<has_group_type<T> >::type>
{
	typedef typename T::group_type m_type;
	typedef typename mpl::end<m_type>::type e_type;
	typedef typename mpl::find_if<m_type, Detect<mpl::_> >::type p_type;

	static const bool value = !boost::is_same<e_type, p_type>::value;
};

///////////////////////////////////////////////////////////////////////////////
// struct Scan

template<class T>
struct Scan
{
	typedef typename mpl::end<T>::type e_type;
	typedef typename mpl::find_if<T, Detect<mpl::_> >::type type;
	BOOST_MPL_ASSERT((mpl::not_<boost::is_same<type, e_type> >));
	typedef typename mpl::iterator_range<typename mpl::next<type>::type, e_type>::type s_type;
	typedef typename mpl::find_if<s_type, Detect<mpl::_> >::type b_type;
	BOOST_MPL_ASSERT((boost::is_same<b_type, e_type>));
};

///////////////////////////////////////////////////////////////////////////////
// struct Index

template<class T>
struct Index
{
	typedef typename mpl::distance<
			typename mpl::begin<T>::type,
			typename Scan<T>::type>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Point

template<class T>
struct Point
{
	typedef typename mpl::deref<typename Scan<T>::type>::type type;
};

} // namespace Group

namespace Value
{
///////////////////////////////////////////////////////////////////////////////
// struct Resolve forward declaration

template<class T>
struct Resolve;

///////////////////////////////////////////////////////////////////////////////
// struct Dig

template<class T>
struct Dig
{
	typedef typename Resolve<typename Details::Group::Point
			<typename T::group_type>::type>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Extract

template<class T>
struct Grab
{
	typedef typename T::value_type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Refine

template<class T>
struct Refine
{
	typedef mpl::eval_if<has_group_type<T>, Dig<T>, mpl::identity<T> > g_type;
	typedef typename mpl::eval_if<has_value_type<T>, Grab<T>, g_type>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Branch

template<class T>
struct Branch: mpl::or_<boost::is_fundamental<T>, boost::is_same<T, QString> >
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Resolve

template<class T>
struct Resolve
{
	typedef typename mpl::eval_if<Branch<T>, mpl::identity<T>, Refine<T> >::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Identity

template<class T>
struct Identity
{
	typedef T value_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Bin

template<class T>
struct Bin: Identity<T>
{
	typedef T value_type;

	Bin(): m_value()
	{
	}

	const T& getValue() const
	{
		return m_value;
	}

protected:
	void setValue(const T& value_)
	{
		m_value = value_;
	}

private:
	T m_value;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ancestor

template<class T>
struct Ancestor
{
	typedef Bin<typename Resolve<T>::type> type;
};

} // namespace Value
} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// struct Value

template<class T>
struct Value
{
	typedef typename Details::Value::Resolve<T>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Access

template<class T, class Enabled = void>
struct Access: Details::Value::Ancestor<T>::type
{
	typedef typename Details::Value::Ancestor<T>::type base_type;

	using base_type::setValue;
};

template<int I>
struct Access<mpl::int_<I> >
{
};

template<class T>
struct Access<T, typename boost::enable_if<Validatable<T> >::type>:
	Details::Value::Ancestor<T>::type
{
	typedef typename Details::Value::Ancestor<T>::type base_type;

	bool setValue(const typename base_type::value_type& value_)
	{
		bool output = Validatable<T>::validate(value_);
		if (output)
			base_type::setValue(value_);

		return output;
	}
};

} // namespace Libvirt

#endif // __BASE_H__

