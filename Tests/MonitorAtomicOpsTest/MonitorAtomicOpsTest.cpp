///////////////////////////////////////////////////////////////////////////////
///
/// @file MonitorAtomicOpsTest.cpp
///
/// @brief Tests for different atomic operations implemented in monitor.
///
/// @author denisla
///
/// Copyright 2013-2013. Parallels IP Holdings GmbH.
/// All Rights Reserved.
///
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <time.h>

#include "MonitorAtomicOpsTest.h"
#include "Libraries/Std/AtomicOps.h"


#define TestGen(argtype) \
	argtype	TestAtomicIncGen(argtype* c)								{ return AtomicInc(c);				} \
	argtype	TestAtomicDecGen(argtype* c)								{ return AtomicDec(c);				} \
	argtype	TestAtomicDecAndTestGen(argtype* c)							{ return AtomicDecAndTest(c);		} \
	argtype	TestAtomicIncAndTestGen(argtype* c)							{ return AtomicIncAndTest(c);		} \
	argtype	TestAtomicAddGen(argtype* c, argtype b)						{ return AtomicAdd(c,b);			} \
	argtype	TestAtomicOrGen(argtype* c, argtype b)						{ return AtomicOr(c,b);				} \
	argtype	TestAtomicXorGen(argtype* c, argtype b)						{ return AtomicXor(c,b);			} \
	argtype	TestAtomicAndGen(argtype* c, argtype b)						{ return AtomicAnd(c,b);			} \
	argtype	TestAtomicSwapGen(argtype* c, argtype b)					{ return AtomicSwap(c,b);			} \
	argtype	TestAtomicCompareSwapGen(argtype* c, argtype b, argtype d)	{ return AtomicCompareSwap(c,b,d);	} \
	argtype	TestAtomicReadGen(const argtype* c)							{ return AtomicRead(c);				} \
	void	TestAtomicWriteGen(argtype* c, argtype b)					{ AtomicWrite(c,b);					}

TestGen(int);
TestGen(unsigned int);
TestGen(atomicInt64);
TestGen(unsigned atomicInt64);


#define INSTANTIATE_TEMPLATE(name, param) \
	template void name<int>(param); \
	template void name<unsigned int>(param); \
	template void name<atomicInt64>(param); \
	template void name<unsigned atomicInt64>(param)

#define DEFINE_MEMBERS_PARAM(name, template_func, param) \
	void CMonitorAtomicOpsTest::name##I()		{ template_func<int>					(param); } \
	void CMonitorAtomicOpsTest::name##UI()		{ template_func<unsigned int>			(param); } \
	void CMonitorAtomicOpsTest::name##I64()		{ template_func<atomicInt64>			(param); } \
	void CMonitorAtomicOpsTest::name##UI64()	{ template_func<unsigned atomicInt64>	(param); }

#define NULL_PARAM
#define DEFINE_MEMBERS(name) DEFINE_MEMBERS_PARAM(name, name##Exec, NULL_PARAM)


/**
 * Basic thread-safe fixed-size stack implementation
 */
template<typename T>
class CStaticStack
{
public:
	CStaticStack(): m_uiTop(0) {}
	~CStaticStack()
	{
		if( !isEmpty() )
		{
			printf("WARNING: stack is not empty\n");
			do
			{
				T temp;
				pop(&temp);
			}
			while( !isEmpty() );
		}
	}

	bool isEmpty() const { return !m_uiTop; }

	bool push(T f_val)
	{
		if(m_uiTop >= sizeof(m_data) / sizeof(*m_data))
		{
			printf("ERROR: stack overflow\n");
			return false;
		}

		m_mutex.lock();
		m_data[m_uiTop++] = f_val;
		m_mutex.unlock();

		return true;
	}
	bool pop(T* f_val)
	{
		if(!m_uiTop)
		{
			printf("ERROR: stack underflow\n");
			return false;
		}

		m_mutex.lock();
		*f_val = m_data[--m_uiTop];
		m_mutex.unlock();

		return true;
	}

private:
	T m_data[CMonitorAtomicOpsTest::m_nThreads];
	unsigned int m_uiTop;

private:
	QMutex m_mutex;
};


/**
 * Syncronization object to achieve the following behavior:
 *	1. Create multiple threads.
 *	2. Each thread does some initialization and blocks.
 *	2. Wait until all the threads are blocked.
 *	3. Resume all the threads at once.
 */
class CThreadStartBarrier
{
public:
	bool init(unsigned int f_nThreads)
	{
		if(!m_start.available())
		{
			m_nThreads = f_nThreads;

			m_start.release(m_nThreads);
			m_exec.acquire( m_exec.available() );
			return true;
		}
		else
			return false;
	}
	void waitAllStartedAndRun()
	{
		while(m_start.available()) {}
		m_exec.release(m_nThreads);
	}

	void setStartedAndWait()
	{
		m_start.acquire(1);
		m_exec.acquire(1);
	}

private:
	unsigned int m_nThreads;

	QSemaphore m_start;
	QSemaphore m_exec;
} g_barrier;


/**
 * Thread management function that implements the scenario
 * described at CThreadStartBarrier definition.
 */
static void runThreads(QThread* *f_apThreads, unsigned int f_nThreads)
{
	unsigned int i;

	if( !g_barrier.init(f_nThreads) )
	{
		printf("ERROR: busy\n");
		return;
	}

	for(i = 0; i < f_nThreads; i++)
		f_apThreads[i]->start();
	g_barrier.waitAllStartedAndRun();

	for(i = 0; i < f_nThreads; i++)
		f_apThreads[i]->wait();
}


/**
 * Type independent random function.
 * Stdlib rand() generates a number from 0 to RAND_MAX only.
 * Combine multiple rand() results to get a type-independent value.
 */
template<typename T>
static T Rand(T)
{
	T r = 0;
	for(unsigned int n = (unsigned int)sizeof(r); n; n--)
	{
		r <<= 8;
		r |= rand() & 0xFF;
	}
	return r;
}


/**
 * Atomic Inc/Dec Tests:
 *	Each thread performs inc/dec on the shared variable.
 *	Compare the result with the manually calculated one.
 */
template<typename T>
class CTestAtomicIncDec : public QThread
{
public:
	CTestAtomicIncDec(CStaticStack<T>* f_pStack, T* f_pSharedRegister, bool f_bInc):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister),
		m_bInc(f_bInc)
	{}

	void run()
	{
		g_barrier.setStartedAndWait();
		if(m_pSharedRegister)
		{
			T val;
			if(m_bInc)
				val = TestAtomicIncGen(m_pSharedRegister);
			else
				val = TestAtomicDecGen(m_pSharedRegister);
			m_pStack->push(val);
		}
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
	bool m_bInc;
};


template<typename T>
static void testIncDec(bool f_bInc)
{
	CStaticStack<T> stack;

	bool bPassed = false;
	T offset = CMonitorAtomicOpsTest::m_nThreads / 2;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T sharedRegister = (f_bInc ? -1 : 1) * offset;

		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			aThreads[j] = new CTestAtomicIncDec<T>(&stack, &sharedRegister, f_bInc);

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		unsigned int mask = ((1 << CMonitorAtomicOpsTest::m_nThreads) - 1) << (f_bInc ? 0 : 1);
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;

			if( stack.pop(&val) )
				mask &= ~(1 << (val + offset));
		}

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(!mask && sharedRegister == ((f_bInc ? 1 : -1) * offset))
			bPassed = true;
		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testIncDec, bool);

DEFINE_MEMBERS_PARAM(testInc, testIncDec, true);
DEFINE_MEMBERS_PARAM(testDec, testIncDec, false);


/**
 * XXX:
 *	initialization of static variables is not thread-safe on some compilers
 *	(i.e. VS <= 2010) so use global declaration.
 */
QMutex criticalSection;

/**
 * Atomic Inc/Dec and Test Tests:
 *	Each thread performs inc/dec until on
 *	the shared variable until it equals to zero.
 *	Compare the result with the manually calculated one.
 */
template<typename T>
class CTestAtomicIncDecAndTest : public QThread
{
public:
	CTestAtomicIncDecAndTest(CStaticStack<T>* f_pStack, T* f_pSharedRegister, bool f_bInc):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister),
		m_bInc(f_bInc)
	{}

	void run()
	{
		static bool s_bActive;

		// The same initialization is done by every thread but it doesn't matter here
		criticalSection.lock();
		s_bActive = true;
		criticalSection.unlock();

		g_barrier.setStartedAndWait();

		if(m_pSharedRegister)
		{
			T nLoops = 0;

			while(1)
			{
				bool doBreak = false;

				bool bStop = m_bInc ? TestAtomicIncAndTestGen(m_pSharedRegister) : TestAtomicDecAndTestGen(m_pSharedRegister);
				nLoops++;

				criticalSection.lock();
				if(bStop)
				{
					s_bActive = false;
					doBreak = true;
				}
				else if(!s_bActive)
					doBreak = true;
				criticalSection.unlock();

				if(doBreak)
					break;
			}

			m_pStack->push(nLoops);
		}
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
	bool m_bInc;
};


template<typename T>
static void testIncDecAndTest(bool f_bInc)
{
	CStaticStack<T> stack;
	T unused = 0;

	bool bPassed = false;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T counter = (f_bInc ? -1 : 1) * ((Rand(unused) & 0xFFFF) + CMonitorAtomicOpsTest::m_nThreads);
		T sharedRegister = counter;

		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			aThreads[j] = new CTestAtomicIncDecAndTest<T>(&stack, &sharedRegister, f_bInc);

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;
			if( stack.pop(&val) )
				counter += (f_bInc ? 1 : -1) * val;
		}
		counter -= sharedRegister;

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(!counter)
			bPassed = true;
		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testIncDecAndTest, bool);

DEFINE_MEMBERS_PARAM(testIncAndTest, testIncDecAndTest, true);
DEFINE_MEMBERS_PARAM(testDecAndTest, testIncDecAndTest, false);


/**
 * Atomic Or/Xor/And/Add:
 *	Each thread performs or/xor/and/add on
 *	the shared variable.
 *	Compare the result with the manually calculated one.
 */
enum TestLogicType { TLOr, TLXor, TLAnd, TLExAdd };

template<typename T>
class CTestAtomicLogicAdd: public QThread
{
public:
	CTestAtomicLogicAdd(CStaticStack<T>* f_pStack, T* f_pSharedRegister, TestLogicType f_type):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister),
		m_type(f_type)
	{}

	void run()
	{
		T val = 0;
		m_pStack->pop(&val);

		g_barrier.setStartedAndWait();
		if(m_pSharedRegister)
		{
			switch(m_type)
			{
				case TLOr:		TestAtomicOrGen(m_pSharedRegister,  val); break;
				case TLXor:		TestAtomicXorGen(m_pSharedRegister, val); break;
				case TLAnd:		TestAtomicAndGen(m_pSharedRegister, val); break;
				case TLExAdd:	TestAtomicAddGen(m_pSharedRegister, val); break;
				default:;
			}
		}

		// Save back for checking
		m_pStack->push(val);
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
	TestLogicType m_type;
};


template<typename T>
static void testLogicAdd(TestLogicType f_type)
{
	CStaticStack<T> stack;
	T unused = 0;

	bool bPassed = false;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T sharedRegister = 0;
		T res = 0;

		switch(f_type)
		{
			case TLOr:
			case TLXor:
			case TLExAdd:
				break;
			case TLAnd:
				sharedRegister--;
				res = sharedRegister;
				break;
			default:
				QVERIFY(!"ERROR: unsupported logic test\n");
				return;
		}

		// Gen extra seed
		T s = Rand(unused);
		T seed = 0;
		for(unsigned int j = 0; j < sizeof(seed); j++)
			seed |= (1 << (8 * j)) << (s >> (8 * j) & 0x07);

		// Prepare and start threads
		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			aThreads[j] = new CTestAtomicLogicAdd<T>(&stack, &sharedRegister, f_type);
			T val = Rand(unused);
			switch(f_type)
			{
				case TLOr:	val &= ~seed; break;
				case TLAnd:	val |=  seed; break;
				default:;
			}
			stack.push(val);
		}

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		// Check
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;

			if( stack.pop(&val) )
			{
				switch(f_type)
				{
					case TLExAdd:	sharedRegister	-= val; break;
					case TLOr:		res				|= val; break;
					case TLXor:		sharedRegister	^= val; break;
					case TLAnd:		res				&= val; break;
					default:;
				}
			}
		}

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(sharedRegister == res)
			bPassed = true;
		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testLogicAdd, TestLogicType);

DEFINE_MEMBERS_PARAM(testOr , testLogicAdd, TLOr);
DEFINE_MEMBERS_PARAM(testXor, testLogicAdd, TLXor);
DEFINE_MEMBERS_PARAM(testAnd, testLogicAdd, TLAnd);
DEFINE_MEMBERS_PARAM(testAdd, testLogicAdd, TLExAdd);


/**
 * Atomic Swap:
 *	Each thread swaps random value with the shared variable.
 *	All the values must be regrouped but
 *	have the same values after that.
 */
template<typename T>
class CTestAtomicSwap : public QThread
{
public:
	CTestAtomicSwap(CStaticStack<T>* f_pStack, T* f_pSharedRegister):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister)
	{}

	void run()
	{
		T val = 0;
		m_pStack->pop(&val);

		g_barrier.setStartedAndWait();
		if(m_pSharedRegister)
			m_pStack->push( TestAtomicSwapGen(m_pSharedRegister, val) );
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
};


template<typename T>
static void testSwapExec()
{
	CStaticStack<T> stack;
	T unused = 0;

	bool bPassed = false;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T sharedRegister = Rand(unused);

		T aVals[CMonitorAtomicOpsTest::m_nThreads + 1/*shared reg*/];
		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			aVals[j] = Rand(unused);
			stack.push(aVals[j]);
			aThreads[j] = new CTestAtomicSwap<T>(&stack, &sharedRegister);
		}
		aVals[CMonitorAtomicOpsTest::m_nThreads] = sharedRegister;

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		unsigned int mask = (1 << (CMonitorAtomicOpsTest::m_nThreads + 1/*shared reg*/)) - 1;
		unsigned int k;
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;

			if( stack.pop(&val) )
			{
				for(k = 0; k < sizeof(aVals) / sizeof(*aVals); k++)
				{
					if(val == aVals[k])
					{
						mask &= ~(1 << k);
						break;
					}
				}
			}
		}
		if( !(mask & (mask - 1)) ) // only one bit left
		{
			for(k = 0; k < CMonitorAtomicOpsTest::m_nThreads; k++)
			{
				if(mask & (1 << k))
					break;
			}
			if(k < CMonitorAtomicOpsTest::m_nThreads && aVals[k] == sharedRegister)
				mask &= ~(1 << k);
		}

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(!mask)
			bPassed = true;
		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testSwapExec, NULL_PARAM);

DEFINE_MEMBERS(testSwap);


template<typename T>
/**
 * Atomic Compare Swap:
 *	Each thread tries to swaps random value with the shared variable.
 *	Only one thread will succeed and get the initial shared variable value;
 *	all other threads will fail the comparison and get the same new value
 *	written by the first succeeded thread.
 */
class CTestAtomicCompareSwap : public QThread
{
public:
	CTestAtomicCompareSwap(CStaticStack<T>* f_pStack, T* f_pSharedRegister):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister)
	{}

	void run()
	{
		T val = 0;
		m_pStack->pop(&val);

		criticalSection.lock();
		T sharedRegister = *m_pSharedRegister;
		criticalSection.unlock();

		g_barrier.setStartedAndWait();
		if(m_pSharedRegister)
			m_pStack->push( TestAtomicCompareSwapGen(m_pSharedRegister, sharedRegister, val) );
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
};


template<typename T>
static void testCompareSwapExec()
{
	CStaticStack<T> stack;
	T unused = 0;

	bool bPassed = false;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T sharedRegister = Rand(unused);

		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val;
			do { val = Rand(unused); } while(val == sharedRegister);
			stack.push(val);
			aThreads[j] = new CTestAtomicCompareSwap<T>(&stack, &sharedRegister);
		}

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		unsigned int nEquals = 0;
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;

			if( stack.pop(&val) )
			{
				if(val == sharedRegister)
					nEquals++;
			}
		}

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(nEquals == CMonitorAtomicOpsTest::m_nThreads - 1)
			bPassed = true;
		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testCompareSwapExec, NULL_PARAM);

DEFINE_MEMBERS(testCompareSwap);


/**
 * Atomic Read:
 *	Each thread reads the same shared variable concurently.
 *	All the read values must be the same after that.
 */
template<typename T>
class CTestAtomicRead : public QThread
{
public:
	CTestAtomicRead(CStaticStack<T>* f_pStack, T* f_pSharedRegister):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister)
	{}

	void run()
	{
		g_barrier.setStartedAndWait();
		if(m_pSharedRegister)
			m_pStack->push( TestAtomicReadGen(m_pSharedRegister) );
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
};


template<typename T>
static void testReadExec()
{
	CStaticStack<T> stack;
	T unused = 0;

	bool bPassed = false;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T sharedRegister;
		do { sharedRegister = Rand(unused); } while(!sharedRegister);

		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			aThreads[j] = new CTestAtomicRead<T>(&stack, &sharedRegister);

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		unsigned int j;
		for(j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;

			if( stack.pop(&val) )
			{
				if(sharedRegister != val)
					break;
			}
		}
		if(j == CMonitorAtomicOpsTest::m_nThreads)
			bPassed = true;

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testReadExec, NULL_PARAM);

DEFINE_MEMBERS(testRead);


/**
 * Atomic Read:
 *	Each thread writes to the same shared variable concurently.
 *	Shared variable must have the latest written value after that.
 */
template<typename T>
class CTestAtomicWrite : public QThread
{
public:
	CTestAtomicWrite(CStaticStack<T>* f_pStack, T* f_pSharedRegister):
		m_pStack(f_pStack),
		m_pSharedRegister(f_pSharedRegister)
	{}

	void run()
	{
		T val = 0;
		m_pStack->pop(&val);

		g_barrier.setStartedAndWait();
		if(m_pSharedRegister)
			TestAtomicWriteGen(m_pSharedRegister, val);

		m_pStack->push(val);
	}

private:
	CStaticStack<T>* m_pStack;
	T* m_pSharedRegister;
};


template<typename T>
static void testWriteExec()
{
	CStaticStack<T> stack;
	T unused = 0;

	bool bPassed = false;

	for(unsigned int i = 0; i < CMonitorAtomicOpsTest::m_nIterations; i++)
	{
		bPassed = false;

		T sharedRegister = 0;

		QThread* aThreads[CMonitorAtomicOpsTest::m_nThreads];
		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val;
			do { val = Rand(unused); } while(!val);
			stack.push(val);

			aThreads[j] = new CTestAtomicWrite<T>(&stack, &sharedRegister);
		}

		runThreads(aThreads, CMonitorAtomicOpsTest::m_nThreads);

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
		{
			T val = 0;

			if( stack.pop(&val) )
			{
				if(sharedRegister == val)
					bPassed = true; // don't break to clear stack
			}
		}

		for(unsigned int j = 0; j < CMonitorAtomicOpsTest::m_nThreads; j++)
			delete aThreads[j];

		if(!bPassed)
			break;
	}

	QVERIFY(bPassed);
}

INSTANTIATE_TEMPLATE(testWriteExec, NULL_PARAM);

DEFINE_MEMBERS(testWrite);


void CMonitorAtomicOpsTest::initTestCase() { srand( time(0) ); }

QTEST_MAIN(CMonitorAtomicOpsTest)
