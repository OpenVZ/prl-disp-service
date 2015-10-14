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

#ifndef __PATTERNS_H__
#define __PATTERNS_H__
#include "base.h"
#include <QStack>
#include <QDomNode>
#include "marshal.h"
#include <QBitArray>
#include <boost/none.hpp>
#include <boost/mpl/inherit.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <boost/optional/optional.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Pattern

struct Pattern
{
};

template<class T>
struct Traits<T, typename boost::enable_if<boost::is_base_of<Pattern, T> >::type>
{
	static int parse(T& dst_, QStack<QDomElement>& dom_)
	{
		return dst_.consume(dom_);
	}
	static int generate(const T& src_, QDomElement& dom_)
	{
		return src_.produce(dom_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Empty

struct Empty: Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		int output = -1;
		QDomElement t = stack_.pop();
		if (!stack_.top().isNull())
		{
			output *= !stack_.top().text().trimmed().isEmpty();
		}
		stack_.push(t);
		return output;
	}
	int produce(QDomElement& ) const
	{
		return 0;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Fragment forward declaration

template<class T, class Enabled = void>
struct Fragment;

///////////////////////////////////////////////////////////////////////////////
// struct Element forward declaration

template<class T, class N>
struct Element;

namespace Details
{
namespace Group
{
///////////////////////////////////////////////////////////////////////////////
// struct Body forward declaration

template<class T>
struct Body;

///////////////////////////////////////////////////////////////////////////////
// struct Cons

template<class T, unsigned I>
struct Cons
{
	static const unsigned index = I;
	T value;

	Cons(): value()
	{
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Wrap

template<class T, class F>
struct Wrap
{
	typedef typename mpl::distance<F, T>::type distance_type;
	typedef Cons<typename mpl::deref<T>::type, distance_type::value> type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Fold

template<class T>
struct Fold
{
	typedef typename mpl::begin<T>::type b_type;
	typedef typename mpl::iter_fold<
			T,
			mpl::vector<>,
			mpl::push_back<mpl::_1, Wrap<mpl::_2, b_type> >
		>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Composite

template<class T>
struct Composite
{
	typedef typename mpl::inherit_linearly<typename Fold<T>::type,
                                mpl::inherit<mpl::_1, 
					mpl::identity<mpl::_2> > >::type type;
};

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Context

template<class D>
struct Context
{
	typedef D dom_type;

	Context(dom_type& dom_): m_dom(&dom_), m_result()
	{
	}

	dom_type* getDom() const
	{
		return m_dom;
	}	
	int getResult() const
	{
		return m_result;
	}
	bool update(int result_)
	{
		if (0 > result_)
		{
			m_dom = NULL;
			m_result = result_;
			return false;
		}
		m_result += result_;
		return true;
	}

private:
	dom_type* m_dom;
	int m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T>
struct Traits;

template<>
struct Traits<QStack<QDomElement> >
{
	template<class T>
	struct apply
	{
		typedef Body<T> type;
	};

	template<class P>
	static int do_(QStack<QDomElement>& dom_, P& pattern_)
	{
		return pattern_.consume(dom_);
	}
};

template<>
struct Traits<QDomElement>
{
	template<class T>
	struct apply
	{
		typedef const Body<T> type;
	};

	template<class P>
	static int do_(QDomElement& dom_, const P& pattern_)
	{
		return pattern_.produce(dom_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T, class D>
struct Unit
{
	typedef Context<D> context_type;
	typedef typename mpl::apply<Traits<D>, T>::type group_type;

	Unit(group_type& group_, context_type& context_):
		m_group(&group_), m_context(&context_)
	{
	}
	explicit Unit(group_type& group_): m_group(&group_), m_context()
	{
	}

	void setContext(context_type* context_)
	{
		m_context = context_;
	}
	template<class U, quint32 I>
	bool operator()(const Cons<U, I>& cons_)
	{
		if (NULL == m_context)
			return false;

		D* d = m_context->getDom();
		if (NULL == d)
			return false;

		(void)cons_;
		return m_context->update(Traits<D>::do_(*d, m_group->template get<I>()));
	}

private:
	group_type* m_group;
	context_type* m_context;
};

} // namespace Visitor

///////////////////////////////////////////////////////////////////////////////
// struct Body

template<class T>
struct Body: private Composite<T>::type
{
	typedef T group_type;

	template<unsigned I>
	typename mpl::at_c<T, I>::type& get()
	{
		typedef Cons<typename mpl::at_c<T, I>::type, I> bin_type;
		return static_cast<bin_type& >(*this).value;
	}
	template<unsigned I>
	typename mpl::at_c<T, I>::type const& get() const
	{
		typedef Cons<typename mpl::at_c<T, I>::type, I> bin_type;
		return static_cast<bin_type const& >(*this).value;
	}
	int produce(QDomElement& dst_) const
	{
		Visitor::Context<QDomElement> c(dst_);
		mpl::for_each<typename Fold<T>::type>(Visitor::Unit<T, QDomElement>(*this, c));
		return c.getResult();
	}
};

namespace Unordered
{
///////////////////////////////////////////////////////////////////////////////
// struct Consumer

class Consumer
{
	typedef Details::Group::Visitor::Context<QStack<QDomElement> > step_type;

public:
	explicit Consumer(step_type::dom_type& point_):
		m_best(-1), m_step(~0U), m_point(point_), m_dom(&point_)
	{
	}

	bool update(quint32 step_, const step_type& parsing_)
	{
		int p = parsing_.getResult();
		if (p <= getResult())
			return false;

		m_best = p;
		m_step = step_;
		*m_dom = *parsing_.getDom();
		return true;
	}
	quint32 getStep() const
	{
		return m_step;
	}
	int getResult() const
	{
		return m_best;
	}
	const step_type::dom_type& getPoint() const
	{
		return m_point;
	}
private:
	int m_best;
	quint32 m_step;
	step_type::dom_type m_point, *m_dom;
};

///////////////////////////////////////////////////////////////////////////////
// class Visitor

template<class T>
class Visitor: Details::Group::Visitor::Unit<T, QStack<QDomElement> >
{
	typedef Details::Group::Visitor::Unit<T, QStack<QDomElement> > base_type;

public:
	Visitor(typename base_type::group_type& group_, const QBitArray& mask_, Consumer& consumer_):
		base_type(group_), m_mask(mask_), m_consumer(&consumer_)
	{
	}

	template<class U, quint32 I>
	bool operator()(Cons<U, I> cons_)
	{
		if (!m_mask.testBit(I))
			return false;

		QStack<QDomElement> d = m_consumer->getPoint();
		Details::Group::Visitor::Context<QStack<QDomElement> > c(d);
		base_type::setContext(&c);
		base_type::operator()(cons_);
		base_type::setContext(NULL);
		return m_consumer->update(I, c);
	}
private:
	QBitArray m_mask;
	Consumer* m_consumer;
};

} // namespace Unordered
} // namespace Group

namespace Choice
{
///////////////////////////////////////////////////////////////////////////////
// struct Wrap

template<class T>
struct Wrap
{
	typedef Access<T> type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Body

template<class T>
struct Body: Access<typename boost::make_variant_over<
			typename mpl::transform<T, Wrap<mpl::_1> >::type>::type>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Consumer

struct Consumer
{
	explicit Consumer(QStack<QDomElement>& stack_):
		m_result(-1), m_point(stack_), m_stack(&stack_)
	{
	}
	
	template<class T>
	bool do_(Fragment<T>& pattern_)
	{
		QStack<QDomElement> s = m_point;
		int x = pattern_.consume(s);
		if (x > m_result)
		{
			m_result = x;
			*m_stack = s;
			return true;
		}
		return false;
	}
	int getResult() const
	{
		return m_result;
	}

private:
	int m_result;
	QStack<QDomElement> m_point;
	QStack<QDomElement> *m_stack;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

template<class T>
struct Visitor
{
	Visitor(Body<T>& access_, Consumer& consumer_):
		m_access(&access_), m_consumer(&consumer_)
	{
	}

	template<class U>
	void operator()(U )
	{
		Fragment<U> u;
		if (m_consumer->do_(u))
			m_access->setValue(u);
	}

private:
	Body<T>* m_access;
	Consumer* m_consumer;
};

///////////////////////////////////////////////////////////////////////////////
// struct Producer

template<class T>
struct Producer: boost::static_visitor<int>
{
	Producer(QDomElement& dom_): m_dom(&dom_)
	{
	}

	template<class U>
	int operator()(const Access<U>& access_) const
	{
		Fragment<U> f;
		f.setValue(access_.getValue());
		return f.produce(*m_dom);
	}

private:
	QDomElement* m_dom;
};

} // namespace Choice

namespace Optional
{
///////////////////////////////////////////////////////////////////////////////
// struct Test

template<class T>
struct Test: mpl::false_
{
};

template<class T>
struct Test<boost::optional<T> >: mpl::true_
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Decorate

template<class T>
struct Decorate
{
	typedef typename Details::Value::Grab<T>::type v_type;
	typedef typename mpl::if_<Test<v_type>, v_type, boost::optional<v_type> >::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Body

template<class T, class Enabled = void>
struct Body
{
	int consume(QStack<QDomElement>& stack_)
	{
		int output = Fragment<T>().consume(stack_);
		if (0 > output)
			output = 0;

		return output;
	}
	int produce(QDomElement& dst_) const
	{
		return Fragment<T>().produce(dst_);
	}
};

template<class T>
struct Body<T, typename boost::disable_if<
			typename mpl::eval_if<has_value_type<T>,
				boost::is_same<typename Details::Value::Grab<Access<T> >::type, Empty>,
				mpl::true_>::type>::type>:
		Access<Details::Value::Identity<typename Decorate<T>::type> >
{
private:
	typedef typename Details::Value::Grab<T>::type wild_type;

public:
	int consume(QStack<QDomElement>& stack_)
	{
		Fragment<T> f;
		this->setValue(boost::none);
		int output = f.consume(stack_);
		if (0 > output)
			output = 0;
		else
			this->setValue(f.getValue());

		return output;
	}
	int produce(QDomElement& dst_) const
	{
		if (!this->getValue())
			return 0;

		Fragment<T> f;
		f.setValue(release(Test<wild_type>()));
		return f.produce(dst_);
	}

private:
	const wild_type& release(mpl::true_) const
	{
		return this->getValue();
	}
	const wild_type& release(mpl::false_) const
	{
		return this->getValue().get();
	}
};

template<class T>
struct Body<Element<Empty, T> >: Access<bool>
{
	int consume(QStack<QDomElement>& stack_)
	{
		Element<Empty, T> e;
		this->setValue(false);
		int output = e.consume(stack_);
		if (0 < output)
			this->setValue(true);
		else if (0 > output)
			output = 0;

		return output;
	}
	int produce(QDomElement& dst_) const
	{
		if (!this->getValue())
			return 0;

		Element<Empty, T> e;
		return e.produce(dst_);
	}
};

} // namespace Optional

namespace Value
{
///////////////////////////////////////////////////////////////////////////////
// struct Envelope

struct Envelope
{
	template<class T>
	static
	typename boost::enable_if<boost::is_same<T, typename Grab<Access<T> >::type>
		>::type collapse(const T& src_, Access<T>& dst_)
	{
		return dst_.setValue(src_);
	}
	template<class T, class U>
	static void collapse(const Bin<T>& src_, Access<U>& dst_)
	{
		dst_.setValue(src_.getValue());
	}
	template<class T, class U>
	static void collapse(const Details::Group::Body<T>& src_, Access<U>& dst_)
	{
		return collapse(src_.template get<Details::Group::Index<T>::type::value>(), dst_);
	}
	template<class T>
	static
	typename boost::enable_if<boost::is_same<T, typename Grab<Access<T> >::type>
		>::type expand(const Access<T>& src_, T& dst_)
	{
		dst_ = src_.getValue();
	}
	template<class T, class U>
	static void expand(const Access<T>& src_, Access<U>& dst_)
	{
		dst_.setValue(src_.getValue());
	}
	template<class T, class U>
	static void expand(const Access<T>& src_, Details::Group::Body<U>& dst_)
	{
		expand(src_, dst_.template get<Details::Group::Index<U>::type::value>());
	}
};

} // namespace Value
} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// struct Attribute

template<class T, class N>
struct Attribute: Access<T>, Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		QDomElement t = stack_.pop();
		if (stack_.top().isNull())
		{
			stack_.push(t);
			return -1;
		}
		QDomAttr a;
		QDomNamedNodeMap m = stack_.top().attributes();
		for (uint i = m.length(); i-- > 0;)
		{
			QDomAttr b = m.item(i).toAttr();
			if (Name::Traits<N>::consume(b))
			{
				a = b;
				break;
			}
		}
		stack_.push(t);
		if (a.isNull())
			return -1;

		int x = Marshal<T>::setString(a.value(), *this);
		return -1 * (-1 == x);
	}
	int produce(QDomElement& dst_) const
	{
		QDomAttr a = Name::Traits<N>::produceAttribute(dst_);
		if (a.isNull())
			return -1;

		a.setValue(Marshal<T>::getString(*this));
		dst_.setAttributeNode(a);
		return 0;
	}
};

template<class T, class N>
struct Validatable<Attribute<T, N> >: Validatable<T>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Text

template<class T = QString>
struct Text: Access<T>, Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		int output = -1;
		QDomElement t = stack_.pop();
		if (!stack_.top().isNull())
			output = Marshal<T>::setString(stack_.top().text(), *this);

		stack_.push(t);
		return output;
	}
	int produce(QDomElement& dst_) const
	{
		QString v = Marshal<T>::getString(*this);
		QDomText t = dst_.ownerDocument().createTextNode(v);
		if (t.isNull())
			return -1;

		if (dst_.appendChild(t).isNull())
			return -1;

		return 1;
	}
};

template<class T>
struct Validatable<Text<T> >: Validatable<T>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Fragment

template<class T, class Enabled>
struct Fragment: Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		T x;
		return Traits<T>::parse(x, stack_);
	}
	int produce(QDomElement& dst_) const
	{
		T x;
		return Traits<T>::generate(x, dst_);
	}
};

template<class T>
struct Fragment<T, typename boost::enable_if<has_value_type<Access<T> > >::type>:
	Access<T>, Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		T x;
		int output = Traits<T>::parse(x, stack_);
		if (0 <= output)
			Details::Value::Envelope::collapse(x, *this);

		return output;
	}
	int produce(QDomElement& dst_) const
	{
		T x;
		Details::Value::Envelope::expand(*this, x);
		return Traits<T>::generate(x, dst_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Element

template<class T, class N>
struct Element: Fragment<T>
{
	int consume(QStack<QDomElement>& stack_)
	{
		QDomElement e = stack_.top();
		if (e.isNull())
			return -1;

		if (!Name::Traits<N>::consume(e))
			return -1;

		stack_.push(e.firstChildElement());
		int x = Fragment<T>::consume(stack_);
		if (0 > x)
			return x;

		QDomElement t = stack_.pop();
		if (!t.isNull())
			return -1;

		(void)stack_.pop();
		stack_.push(e.nextSiblingElement());
		return 1;
	}
	int produce(QDomElement& dst_) const
	{
		QDomDocument d = dst_.ownerDocument();
		return hang(Name::Traits<N>::produceElement(d), dst_);
	}
	int produce(QDomDocument& dst_) const
	{
		return hang(Name::Traits<N>::produceElement(dst_), dst_);
	}

private:
	int hang(QDomElement dom_, QDomNode& dst_) const
	{
		if (dom_.isNull())
			return -1;

		int x = Fragment<T>::produce(dom_);
		if (0 > x)
			return x;

		if (dst_.appendChild(dom_).isNull())
			return -1;

		return 1;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Optional

template<class T>
struct Optional: Details::Optional::Body<T>, Pattern
{
};

///////////////////////////////////////////////////////////////////////////////
// struct ZeroOrMore

template<class T>
struct ZeroOrMore: Access<Details::Value::Identity<QList<typename Value<T>::type> > >, Pattern
{
	typedef typename Details::Value::Grab<ZeroOrMore<T> >::type list_type;

	int consume(QStack<QDomElement>& stack_)
	{
		int output = 0;
		list_type v;
		while (true)
		{
			Fragment<T> x;
			int y = x.consume(stack_);
			if (0 >= y)
				break;

			v << x.getValue();
			output += y;
		}
		this->setValue(v);
		return output;
	}
	int produce(QDomElement& dst_) const
	{
		int output = 0;
		foreach (typename list_type::const_reference v, this->getValue())
		{
			Fragment<T> x;
			x.setValue(v);
			int s = x.produce(dst_);
			if (0 > s)
				return s;

			output += s;
		}
		return output;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct OneOrMore

template<class T>
struct OneOrMore: ZeroOrMore<T>
{
	int consume(QStack<QDomElement>& stack_)
	{
		int output = ZeroOrMore<T>::consume(stack_);
		if (0 == output)
			output = -1;

		return output;
	}
	int produce(QDomElement& dst_) const
	{
		if (this->getValue().isEmpty())
			return -1;

		return ZeroOrMore<T>::produce(dst_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Ordered

template<class T>
struct Ordered: Details::Group::Body<T>, Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		Details::Group::Visitor::Context<QStack<QDomElement> > c(stack_);
		mpl::for_each<typename Details::Group::Fold<T>::type>
			(Details::Group::Visitor::Unit<T, QStack<QDomElement> >(*this, c));
		return c.getResult();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Unordered

template<class T>
struct Unordered: Details::Group::Body<T>, Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		int output = 0;
		QBitArray v(mpl::size<T>::value, true);
		while (0 < v.count(true))
		{
			Details::Group::Unordered::Consumer x(stack_);
			mpl::for_each<typename Details::Group::Fold<T>::type>
				(Details::Group::Unordered::Visitor<T>(*this, v, x));

			int y = x.getResult();
			if (0 > y)
				return y;

			output += y;
			v.toggleBit(x.getStep());
		}
		return output;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Choice

template<class T>
struct Choice: Details::Choice::Body<T>, Pattern
{
	int consume(QStack<QDomElement>& stack_)
	{
		Details::Choice::Consumer x(stack_);
		mpl::for_each<T>(Details::Choice::Visitor<T>(*this, x));
		return x.getResult();
	}
	int produce(QDomElement& dst_) const
	{
		if (this->getValue().empty())
			return -1;

		return boost::apply_visitor(Details::Choice::Producer<T>(dst_),
				this->getValue());
	}
};

} // namespace Libvirt

#endif // __PATTERNS_H__

