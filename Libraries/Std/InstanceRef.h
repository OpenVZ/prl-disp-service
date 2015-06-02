////////////////////////////////////////////////////////////////////////////////
///
/// @file InstanceRef.h
///
/// Instances and references classes
///
/// @author andreyp, owner alexg
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
////////////////////////////////////////////////////////////////////////////////

#ifndef LIBRARIES_STS_INSTANCE_REF_INCLUDED
#define LIBRARIES_STS_INSTANCE_REF_INCLUDED

////////////////////////////////////////////////////////////////////////////////
// includes
#include <QMutex>
#include <QWaitCondition>


////////////////////////////////////////////////////////////////////////////////
// defines
#define INSTANCE_REF_DECLARE_INSTANCE(ClassName)						\
		public: static InstanceRef::Ref< ClassName > getRef()			\
		{ return InstanceRef::Ref< ClassName >(m_instance); }			\
		private: static InstanceRef::Instance< ClassName > m_instance;

#define INSTANCE_REF_IMPLEMENT_INSTANCE(ClassName)						\
		InstanceRef::Instance< ClassName > ClassName::m_instance;

#define INSTANCE_REF_SET_PTR(ptr)										\
		do { m_instance.setPtr((ptr)); } while (false)

#define INSTANCE_REF_GET_REF(ClassName, RefName)						\
		InstanceRef::Ref< ClassName > RefName(ClassName::getRef())


////////////////////////////////////////////////////////////////////////////////
// InstanceRef namespace

/**
* @namespace InstanceRef
*
* @brief Set of classed that helps to implement "reference on instance" idea
*
* The main goal of this set of classes is to valid have reference on instance,
* that will not be deleted or deinitialized. Once taken, reference will be
* valid and associated instance will not be deleted or deinitialized, while
* reference is in use.
*
* Usage is the following:
* 1. Add InstanceRef::Instance static member to private section of your class:
*    @code
*    class CMyTool
*    {
*    private:
*        static InstanceRef::Instance<CMyTool> m_instance;
*    };
*    @endcode
*
*    Or like this:
*    @code
*    class CMyTool
*    {
*        INSTANCE_REF_DECLARE_INSTANCE(CMyTool)
*    };
*    @endcode
*    Don't foreget to implement InstanceRef::Instance object in .cpp file.
*    You cant use INSTANCE_REF_IMPLEMENT_INSTANCE() macro for this
*
* 2. Implement public static method getRef() in your class:
*    @code
*    class CMyTool
*    {
*    public:
*        static InstanceRef::Ref<CMyTool> getRef()
*        {
*            return InstanceRef::Ref<CMyTool>(m_instance);
*        }
*    }
*    @endcode
*    Macros INSTANCE_REF_DECLARE_INSTANCE() / INSTANCE_REF_IMPLEMENT_INSTANCE()
*    will do it for you.
*
* 3. Use setPtr() method to make reference on your object available for public:
*    @code
*    class CMyTool
*    {
*    public:
*        CMyTool()
*        {
*		     // Initialization code here
*            // ...
*
*            m_instance.setPtr(this);
*        }
*        ~CMyTool()
*        {
*            m_instance.setPtr(0);
*
*		     // Deinitialization code here
*            // ...
*        }
*    }
*    @endcode
*    Or you can use INSTANCE_REF_SET_PTR() macro for this.
*
* 4. Now, anyone can get reference on object of your class in safe manner:
*    @code
*    const InstanceRef::Ref<CMyTool> myToolRef = CMyTool::getRef();
*    if (myToolRef)  // or (0 != myToolRef.ptr()) or (myToolRef.isValid())
*    {
*        // can use myToolRef, it will not be deleted or deinitialized!
*    }
*    // Or, using INSTANCE_REF_GET_REF() macro:
*    INSTANCE_REF_GET_REF(CMyTool, myToolRef);
*    if (myToolRef)
*    {
*        // can use myToolRef, it will not be deleted or deinitialized!
*    }
*    @endcode
*
* The advantage of this classes, that setPtr() method will be blocked, while
* someone has reference on instance. So, using setPtr() method, you can
* control when your object available for public, and when not.
*
* @sa InstanceRef::Instance, InstanceRef::Ref
*
* @author andreyp
*/
namespace InstanceRef
{


// Forward declaration is required to make Ref a friend of Instance class
template <class T> class Ref;


/**
* @class InstanceRef::Instance
*
* @brief Represents instance of object with reference counter
*
* @author andreyp
*/
template <class T>
class Instance
{
public:
	Instance(): m_ptr(0), m_ptrCount(0) {}
	~Instance() { setPtr(0); }

	void setPtr(T *const ptr)
	{
		QMutexLocker lock(&m_ptrLock);
		if (0 != m_ptr && 0 != m_ptrCount) m_ptrCountCondZero.wait(&m_ptrLock);
		m_ptr = ptr, m_ptrCount = 0;
	}

protected:
	T *retain()
	{
		QMutexLocker locker(&m_ptrLock);
		if (0 != m_ptr) ++m_ptrCount;
		return m_ptr;
	}

	void release()
	{
		QMutexLocker locker(&m_ptrLock);
		Q_ASSERT(0 != m_ptr && 0 != m_ptrCount);
		if (0 == --m_ptrCount) m_ptrCountCondZero.wakeOne();
	}

	friend class Ref<T>;

private:
	QMutex m_ptrLock;
	QWaitCondition m_ptrCountCondZero;
	T *m_ptr;
	unsigned m_ptrCount;
};


/**
* @class InstanceRef::Ref
*
* @brief Represents reference on object with reference counter
*
* @author andreyp
*/
template <class T>
class Ref
{
public:
	Ref(): m_inst(0), m_ptr(0) {}
	Ref(Instance<T> &inst): m_inst(&inst), m_ptr(inst.retain()) {}
	Ref(const Ref<T> &ref): m_inst(0), m_ptr(0) { if (0 != ref.m_ptr) attach(ref); }
	~Ref() { if (0 != m_ptr) m_inst->release(); }

	T *ptr() const { return m_ptr; }
	T &ref() const { return *m_ptr; }
	T *operator->() const { return m_ptr; }
	T &operator *() const { return *m_ptr; }
	bool isValid() const { return (0 != m_ptr); }
	operator bool() const { return (0 != m_ptr); }

	const Ref<T> &operator=(const Ref<T> &ref) {
		if (0 != m_ptr) { m_inst->release(); m_ptr = 0; }
		if (0 != ref.m_ptr) attach(ref);
		return ref;
	}

private:
	void attach(const Ref<T> &ref) {
		m_inst = ref.m_inst; m_ptr = ref.m_inst->retain();
	}

	Instance<T> *m_inst;
	T *m_ptr;
};



}	// end of namespace InstanceRef



#endif	// LIBRARIES_STS_INSTANCE_REF_INCLUDED
