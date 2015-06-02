///////////////////////////////////////////////////////////////////////////////
///
/// @file MonitorStdTest.cpp
///
/// Tests fixture class for testing Monitor/Std/ functionality.
///
/// @author vtatarinov
/// @owner alexeyk
///
/// Copyright (c) 2007-2015 Parallels IP Holdings GmbH
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

#include "Tests/MonitorStdTest/MonitorStdTest.h"

#ifdef _WIN_
const size_t MonitorStdTest::s_dmmZoneSizes[DMM_ATTR_MAX] = { 800, 200 };
const char MonitorStdTest::s_zoneMarkers[DMM_ATTR_MAX] = { '0', '4' };
#endif // _WIN_

MonitorStdTest::MonitorStdTest()
#ifdef _WIN_
	: m_totalZonesSize(0)
#endif // _WIN_
{
#ifdef _WIN_
	for(size_t i = 0; i < sizeof(s_dmmZoneSizes) / sizeof(s_dmmZoneSizes[0]); ++i)
		m_totalZonesSize += s_dmmZoneSizes[i];
#endif // _WIN_
}

void MonitorStdTest::init()
{
#ifdef _WIN_
	// at first alloc buffers
	m_pBuf = malloc((m_totalZonesSize + 1) * PAGE_SIZE);
	m_pBufPtr[0] = (PVOID)(((ULONG_PTR)m_pBuf + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE);
	QVERIFY(m_pBuf);

	for(size_t i = 0, sz = 0; i < sizeof(s_dmmZoneSizes) / sizeof(s_dmmZoneSizes[0]); ++i)
	{
		m_aMonBuf[i].ppBuf = &m_pBufPtr[i];
		m_aMonBuf[i].Size.uSize = s_dmmZoneSizes[i] * PAGE_SIZE;
		m_pBufPtr[i] = (PCHAR)m_pBufPtr[0] + sz;
		m_pPtrs[i] = &m_aMonBuf[i];
		sz += m_aMonBuf[i].Size.uSize;

		// prepare buffers
		for(size_t j = 0; j < s_dmmZoneSizes[i]; ++j)
			((char*)m_pBufPtr[i])[ j * PAGE_SIZE ] = s_zoneMarkers[i];
	}

	for(size_t i = (s_dmmZoneSizes[DMM_ATTR_COMMON] - 100) / 2;
		i < (s_dmmZoneSizes[DMM_ATTR_COMMON] - 100) / 2 + 100; ++i)
		((char*)m_pBufPtr[DMM_ATTR_COMMON])[ i * PAGE_SIZE ] = s_zoneMarkers[DMM_ATTR_L4GB];

	// init dmm
	DmmInit(m_pPtrs, DMM_ATTR_MAX);

	srand(GetCpuCycles());
#endif // _WIN_
}

void MonitorStdTest::cleanup()
{
#ifdef _WIN_
	free(m_pBuf);
#endif // _WIN_
}

void MonitorStdTest::SimpleLinkedHashTest()
{
#ifdef _WIN_
	const size_t M = 10;
	const size_t N = M * M;
	SimpleLinkedHash<int, std::string, M, N> slh;
	slh.init("lh1");

	// fill in first half of table
	for(size_t i = 0, n = N / 2; i < n; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		QVERIFY(slh.insert(i, i, std::string(b)));
	}

	// find existing item
	std::string* pVal = slh.search(N / 4, N / 4);
	QVERIFY(pVal);

	// find non-existing item
	pVal = slh.search(N / 2, N / 2);
	QVERIFY(!pVal);

	// fill in second half of table
	for(size_t i = N / 2; i < N; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		QVERIFY(slh.insert(i, i, std::string(b)));
	}

	// add element that can't be added
	// there is no place for it
	char b[10];
	_itoa(N, b, 10);
	QVERIFY(!slh.insert(N, N, std::string(b)));

	// 		// remove non-existing element
	// 		QVERIFY(!slh.RemoveValue(N));
	//
	// 		// remove existing element
	// 		QVERIFY(slh.RemoveValue(N / 2));
	//
	// 		// add element in place of removed one
	// 		_itoa(N / 2, b, 10);
	// 		QVERIFY(slh.insert(N / 2, N / 2, std::string(b)));

	// find all added elements
	for(size_t i = 0; i < N; ++i)
		QVERIFY(slh.search(i, i));

	// remove all added elements
	// 		for(size_t i = 0; i < N; ++i)
	// 			QVERIFY(slh.RemoveValue(i));
	slh.clear();

	// try to find removed elements
	for(size_t i = 0; i < N; ++i)
		QVERIFY(!slh.search(i, i));

	// fill in hash
	for(size_t i = 0; i < N; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		QVERIFY(slh.insert(i, i, std::string(b)));
	}

	// chech values
	for(size_t i = 0; i < N; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		std::string* s = slh.search(i, i);
		QVERIFY(s);
		QVERIFY(!strcmp(s->c_str(), b));
	}

	// clear hash
	slh.clear();
#endif // _WIN_
}

#ifdef _WIN_
#pragma pack(push, 1)
/// Type for testing offsetof macro.
struct foo
{
	int m1;
	double m2;
	int* m3;
};
#pragma pack(pop)
#endif // _WIN_

#ifdef _WIN_
int VsnPrintfWrapper(char* pBuf, size_t size, const char* pFormat, ...)
{
	va_list Args;
	int iRet = 0;

	va_start(Args, pFormat);
	iRet = VsnPrintf(pBuf, size, pFormat, Args);
	va_end(Args);

	return iRet;
}
#endif

void MonitorStdTest::StdLibraryTest()
{
#ifdef _WIN_
	// test offsetof
	QVERIFY(offsetof(foo, m3) == sizeof(int) + sizeof(double));

	// test BMAP_SZ
	QVERIFY(BMAP_SZ(sizeof(int) * 8) == sizeof(int));

	// test BMAP_GET
	DWORD val1 = 0x4000;
	QVERIFY(BMAP_GET(&val1, 14));

	// test BMAP_SET
	DWORD val2 = 0;
	BMAP_SET(&val2, 14);
	QVERIFY(val2 == 0x4000);

	// test BMAP_CLR
	DWORD val3 = 0x4000;
	BMAP_CLR(&val3, 14);
	QVERIFY(!val3);

	// test BIT_SCAN_AND_CLR
	DWORD val4 = 0x4040;
	int bit = 0;
	BIT_SCAN_AND_CLR(val4, bit);
	QVERIFY(val4 == 0x4000 && bit == 6);

	// test pack macros
	BYTE byte1 = 0xff;
	BYTE byte2 = 0x00;
	WORD word1 = MAKEWORD(byte1, byte2);
	WORD word2 = MAKEWORD(byte2, byte1);
	QVERIFY(word1 == 0x00ff && word2 == 0xff00);
	LONG long1 = MAKELONG(word1, word2);
	LONG long2 = MAKELONG(word2, word1);
	QVERIFY(long1 == 0xff0000ff && long2 == 0x00ffff00);
	ULONG64 long64 = MAKELONG64(long1, long2);
	QVERIFY(long64 == 0x00ffff00ff0000ff);

	// test unpack macros
	QVERIFY(LOLONG(long64) == 0xff0000ff && HILONG(long64) == 0x00ffff00);
	QVERIFY(LOWORD(long1) == 0x00ff && HIWORD(long1) == 0xff00);
	QVERIFY(LOBYTE(word1) == 0xff && HIBYTE(word1) == 0x00);

	// test abs
	int iv1 = -5;
	int iv2 = 5;
	QVERIFY(abs(iv1) == abs(iv2) && abs(iv2) == abs(5));

	// test StrniCmp
	{
		const char strs[][7] =
		{
			"QWERTY",
			"qWeRtY",
			"QwErTy",
			"qWeRt^",
			"AWERTY",
			"QwERt"
		};
		const size_t strLen = sizeof(strs[0]) - 1;

		for(size_t k = strLen; k > 0; --k)
		{
			for(size_t i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
			{
				for(size_t j = 0; j < n; ++j)
				{
					QVERIFY(StrniCmp(strs[i], strs[j], k) == _strnicmp(strs[i], strs[j], k)
						|| !( (StrniCmp(strs[i], strs[j], k) > 0) ^ (_strnicmp(strs[i], strs[j], k) > 0) ));
				}
			}
		}
	}

	// test StrCmp
	{
		const char strs[][7] =
		{
			"qwerty",
			"qwerty",
			"qweRty",
			"QWerty",
			"werty",
			"qwert",
			"qw_rty",
			"QW_rty"
		};

		for(size_t i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
		{
			for(size_t j = 0; j < n; ++j)
			{
				QVERIFY(StrCmp(strs[i], strs[j]) == strcmp(strs[i], strs[j])
					|| !( (StrCmp(strs[i], strs[j]) > 0) ^ (strcmp(strs[i], strs[j]) > 0) ));
			}
		}
	}

	// test StrCpy
	{
		const char src[] = "qwerty";
		char* dest = new char[strlen(src) + 1];
		char* res = StrCpy(dest, src);
		QVERIFY(strcmp(src, dest) == 0 && res == dest);
		delete[] dest;
	}

	// test StrNCpy
	{
		const char src[] = "qwerty";
		unsigned int n = strlen(src) + 4;
		char* dest1 = new char[n];
		for(unsigned int i = 0, m = n + 1; i < m; ++i)
		{
			memset(dest1, CHAR_MAX, n);
			char* res = StrNCpy(dest1, src, i);
			QVERIFY(res == dest1);
			if(i <= strlen(src) + 1)
			{
				QVERIFY(dest1[i] == CHAR_MAX);
				for(unsigned int j = 0; j < i; ++j)
				{
					QVERIFY(dest1[j] == src[j]);
				}
			}
			else
			{
				QVERIFY(strcmp(dest1, src) == 0);
			}
		}
		delete[] dest1;
	}

	// test StrCat
	{
		const char strs[][7] =
		{
			"",
			"qwerty"
		};

		char dest1[20];
		char dest2[20];
		for(int i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
		{
			for(int j = 0; j < n; ++j)
			{
				memset(dest1, 0, sizeof(dest1));
				memset(dest2, 0, sizeof(dest2));
				strcpy(dest1,strs[i]);
				strcpy(dest2,strs[i]);
				char* res1 = StrCat(dest1, strs[j]);
				strcat(dest2, strs[j]);
				QVERIFY(res1 == dest1);
				QVERIFY(strcmp(dest1, dest2) == 0);
			}
		}
	}

	// test StrNCat
	{
		const char strs[][7] =
		{
			"",
			"qwerty"
		};

		char dest1[20];
		char dest2[20];
		for(int i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
		{
			for(int j = 0; j < n; ++j)
			{
				for(size_t k = 0; k < strlen(strs[j]) + 4; ++k)
				{
					memset(dest1, 0, sizeof(dest1));
					memset(dest2, 0, sizeof(dest2));
					strcpy(dest1,strs[i]);
					strcpy(dest2,strs[i]);
					char* res1 = StrNCat(dest1, strs[j], k);
					strncat(dest2, strs[j], k);
					QVERIFY(res1 == dest1);
					QVERIFY(strcmp(dest1, dest2) == 0);
				}
			}
		}
	}

	// test StrNCmp
	{
		const char strs[][7] =
		{
			"qwerty",
			"qwerty",
			"qweRty",
			"QWerty",
			"werty",
			"qwert",
			"qw_rty",
			"QW_rty"
		};
		const size_t strLen = sizeof(strs[0]) - 1;

		for(size_t k = strLen; k > 0; --k)
		{
			for(size_t i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
			{
				for(size_t j = 0; j < n; ++j)
				{
					QVERIFY(StrNCmp(strs[i], strs[j], k) == strncmp(strs[i], strs[j], k)
						|| !( (StrNCmp(strs[i], strs[j], k) > 0) ^ (strncmp(strs[i], strs[j], k) > 0) ));
				}
			}
		}
	}

	// test StrChr
	{
		const char str[] = "qVB*3_VQoP XX";
		const int chs[] = { 'q', 'X', '3', ' ', '_', 'v', 'O', '4'};
		for(int i = 0, n = sizeof(chs) / sizeof(chs[0]); i < n; ++i)
		{
			QVERIFY(StrChr(str, chs[i]) == strchr(str, chs[i]));
		}
	}

	// test StrRChr
	{
		const char str[] = "qVB*3_VQoP XX";
		const int chs[] = { 'q', 'X', '3', ' ', '_', 'v', 'O', '4'};
		for(int i = 0, n = sizeof(chs) / sizeof(chs[0]); i < n; ++i)
		{
			QVERIFY(StrRChr(str, chs[i]) == strrchr(str, chs[i]));
		}
	}

	// test StrLen
	{
		const char strs[][100] =
		{
			"",
			"askldjfklajskljdflkajsdf",
			"\t \r\n 9102"
		};

		for(int i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
		{
			QVERIFY(StrLen(strs[i]) == strlen(strs[i]));
		}
	}

	// test StrNLen
	{
		const char strs[][100] =
		{
			"",
			"askldjfklajskljdflkajsdf",
			"\t \r\n 9102",
			"kajsjfioqwoieurlkajsdlfjklajsdfjqoiwueriqwiojflajsdfoqiuwreiuqjalsjdf"
		};

		for(int i = 0, n = sizeof(strs) / sizeof(strs[0]); i < n; ++i)
		{
			for(size_t j = 0, m = sizeof(strs[i]); j < m; ++j)
			{
				if(j < strlen(strs[i]))
				{
					QVERIFY(StrNLen(strs[i], j) == j);
				}
				else
				{
					QVERIFY(StrNLen(strs[i], j) == strlen(strs[i]));
				}
			}
		}
	}

	// test StrSpn
	{
		const char str[][20] =
		{
			"QW#)_73+vbn",
			""
		};

		const char charSet[][20] =
		{
			"WQ3",
			"",
			"901(^"
		};

		for(int i = 0, n = sizeof(str) / sizeof(str[0]); i < n; ++i)
		{
			for(int j = 0, m = sizeof(charSet) / sizeof(charSet[0]); j < m; ++j)
			{
				QVERIFY(StrSpn(str[i], charSet[j]) == strspn(str[i], charSet[j]));
			}
		}
	}

	// test StrpBrk
	{
		const char str[][20] =
		{
			"QW#)_73+vbn",
			""
		};

		const char charSet[][20] =
		{
			"WQ3",
			"",
			"901(^"
		};

		for(int i = 0, n = sizeof(str) / sizeof(str[0]); i < n; ++i)
		{
			for(int j = 0, m = sizeof(charSet) / sizeof(charSet[0]); j < m; ++j)
			{
				QVERIFY(StrpBrk(str[i], charSet[j]) == strpbrk(str[i], charSet[j]));
			}
		}
	}

	// test StrTok
	{
		char str[] = "\t\naklsjdklfja  \tajsdkjfkas alskjdflaskj   askdf f  ";
		char str2[] = "\t\naklsjdklfja  \tajsdkjfkas alskjdflaskj   askdf f  ";
		const char delims[] = "\t\n ";
		char* pos = strtok(str, delims);
		char* pos2 = StrTok(str2, delims);
		QVERIFY(pos == pos2 || strcmp(pos, pos2) == 0);
		while(pos)
		{
			pos = strtok(NULL, delims);
			pos2 = StrTok(NULL, delims);
			QVERIFY(pos == pos2 || strcmp(pos, pos2) == 0);
		}
	}

	// test MemSet
	{
		char buf[20];
		void* ret = MemSet(buf, CHAR_MAX - 1, sizeof(buf));
		QVERIFY(ret == buf);
		for(int i = 0, n = sizeof(buf); i < n; ++i)
		{
			QVERIFY(buf[i] == CHAR_MAX - 1);
		}
	}

	// test MemCpy
	{
		const BYTE buff[] = {0xff, 0x45, 0x87, 0xec, 0x98, 0x23};
		BYTE dst[sizeof(buff)];
		void* ret = MemCpy(dst, buff, sizeof(buff));
		QVERIFY(ret == dst);
		for(int i = 0, n = sizeof(buff); i < n; ++i)
		{
			QVERIFY(dst[i] == buff[i]);
		}
	}

	// test LockMemCpy
	{
		// 1 byte
		{
			BYTE dst = 0;
			BYTE src = 1;
			BYTE svd = 1;
			QVERIFY(!LockMemCpy(&dst, &src, sizeof(BYTE), &svd));
			QVERIFY(dst != src);
			svd = 0;
			QVERIFY(LockMemCpy(&dst, &src, sizeof(BYTE), &svd));
			QVERIFY(dst == src);
		}

		// 2 bytes
		{
			WORD dst = 0;
			WORD src = 1;
			WORD svd = 1;
			QVERIFY(!LockMemCpy(&dst, &src, sizeof(WORD), &svd));
			QVERIFY(dst != src);
			svd = 0;
			QVERIFY(LockMemCpy(&dst, &src, sizeof(WORD), &svd));
			QVERIFY(dst == src);
		}

		// 4 bytes
		{
			DWORD dst = 0;
			DWORD src = 1;
			DWORD svd = 1;
			QVERIFY(!LockMemCpy(&dst, &src, sizeof(DWORD), &svd));
			QVERIFY(dst != src);
			svd = 0;
			QVERIFY(LockMemCpy(&dst, &src, sizeof(DWORD), &svd));
			QVERIFY(dst == src);
		}

		// 8 bytes
		{
			QWORD dst = 0;
			QWORD src = 1;
			QWORD svd = 1;
			QVERIFY(!LockMemCpy(&dst, &src, sizeof(QWORD), &svd));
			QVERIFY(dst != src);
			svd = 0;
			QVERIFY(LockMemCpy(&dst, &src, sizeof(QWORD), &svd));
			QVERIFY(dst == src);
		}
	}

	// test MemMove
	{
		const size_t size = 100;
		BYTE src[size];
		BYTE dst[size];
		memset(src, 0xff, size);
		memset(dst, 0x00, size);
		BYTE* ret = (BYTE*)MemMove(dst, src, size);
		QVERIFY(ret == dst);
		for(size_t i = 0; i < size; ++i)
		{
			QVERIFY(dst[i] == src[i]);
		}
	}

	// test MemCmp
	{
		BYTE buf1[][5] =
		{
			{ 0x20, 0x21, 0x22, 0x23, 0x24 },
			{ 0x20, 0x21, 0x22, 0x23, 0x24 },
			{ 0x20, 0x22, 0x23, 0x24, 0x24 }
		};

		QVERIFY(MemCmp(buf1[0], buf1[0], sizeof(buf1[0]))
			== memcmp(buf1[0], buf1[0], sizeof(buf1[0]))
			|| !( (MemCmp(buf1[0], buf1[0], sizeof(buf1[0])) > 0)
			^ (memcmp(buf1[0], buf1[0], sizeof(buf1[0])) > 0) ));
		QVERIFY(MemCmp(buf1[0], buf1[1], sizeof(buf1[0]))
			== memcmp(buf1[0], buf1[1], sizeof(buf1[0]))
			|| !( (MemCmp(buf1[0], buf1[1], sizeof(buf1[0])) > 0)
			^ (memcmp(buf1[0], buf1[1], sizeof(buf1[0])) > 0) ));
		QVERIFY(MemCmp(buf1[0], buf1[2], sizeof(buf1[0]))
			== memcmp(buf1[0], buf1[2], sizeof(buf1[0]))
			|| !( (MemCmp(buf1[0], buf1[2], sizeof(buf1[0])) > 0)
			^ (memcmp(buf1[0], buf1[2], sizeof(buf1[0])) > 0) ));
		QVERIFY(MemCmp(buf1[2], buf1[0], sizeof(buf1[0]))
			== memcmp(buf1[2], buf1[0], sizeof(buf1[0]))
			|| !( (MemCmp(buf1[2], buf1[0], sizeof(buf1[0])) > 0)
			^ (memcmp(buf1[2], buf1[0], sizeof(buf1[0])) > 0) ));
	}

	// test MemScan
	{
		BYTE buf[] = { 0x20, 0x21, 0x22, 0x23, 0x24 };
		for(size_t i = 0; i < sizeof(buf); ++i)
		{
			QVERIFY(MemScan(buf, buf[i], sizeof(buf)) == buf + i);
		}
		QVERIFY(MemScan(buf, 0x25, sizeof(buf)) == 0);
	}

	// test StrStr
	{
		char str[] = "qWer9t_Yuiop";
		char substr[][20] =
		{
			"qWer",
			"Yuiop",
			"r9t_",
			"qWer9t_Yuiop",
			"4ui)"
		};
		for(size_t i = 0; i < sizeof(substr) / sizeof(substr[0]); ++i)
		{
			QVERIFY(StrStr(str, substr[i]) == strstr(str, substr[i]));
		}
	}

	// test VsnPrintf
	{
		char buf[256];
		char str1[] = "1q2w3e";
		UINT uNumber	= 0x1f1f1f1f;
		UCHAR cNumber	= 0x2f;
		USHORT wNumber	= 0x3f3f;
		ULONG_PTR pNumber = (USHORT)&wNumber;
		int iRet;

		MemSet(buf, 0, sizeof(buf));

		try
		{

			iRet = VsnPrintfWrapper(buf, 7, "%u", uNumber);
			QVERIFY(iRet==-1);

			iRet = VsnPrintfWrapper(buf, 10, "%s", NULL );
			iRet = VsnPrintfWrapper(buf, 256, "%u %u %u %u", uNumber, cNumber, wNumber, pNumber);
			iRet = VsnPrintfWrapper(buf, 256, "%d %d %d %d", uNumber, cNumber, wNumber, pNumber);
			iRet = VsnPrintfWrapper(buf, 256, "%x %x %x %x", uNumber, cNumber, wNumber, pNumber);

			iRet = VsnPrintfWrapper(buf, 16, "%u %u %u %u", uNumber, cNumber, wNumber, pNumber);
			QVERIFY(iRet==-1);
			iRet = VsnPrintfWrapper(buf, 20, "%x %x %x %x", uNumber, cNumber, wNumber, pNumber);
			QVERIFY(iRet==-1);

			iRet = VsnPrintfWrapper(buf, 256, "%s", str1);
			QVERIFY(iRet == (StrLen(str1)));
			iRet = VsnPrintfWrapper(buf, 5, "%s", str1);
			QVERIFY(iRet==-1);
		}
		catch( ... )
		{
			QVERIFY(FALSE);
		}
	}

	// test Rand
	{
		for(UINT i = 1, n = 4096; i < n; ++i)
		{
			QVERIFY(Rand(i) < n);
		}
	}

	// test GetCpuCycles
	{
		for(int i = 0; i < 10000; ++i)
		{
			GetCpuCycles();
		}
	}
#endif // _WIN_
}

#ifdef _WIN_
namespace
{
	const int CACHE_LINES = 20;
}

CACHE_STD_DECLARE_STRUCT(CACHE_LINES, int, StdTestCache);
#endif // _WIN_

void MonitorStdTest::CacheLruTest()
{
#ifdef _WIN_
	UINT tags[] = { 13, 24, 57, 136, 89, 1024, 2024, 3024, 4024 };
	StdTestCache stc;

	// init cache
	CACHE_STD_INIT_LOOP_BEGIN(CACHE_LINES, StdTestCache, &stc)
	CACHE_STD_INIT_LOOP_END()

	// add non-existing elements in cache
	for(int i = 0, n = sizeof(tags) / sizeof(tags[0]); i < n; ++i)
	{
		CACHE_STD_USE_LOOP_BEGIN(StdTestCache, &stc, tags[i] % CACHE_LINES, tags[i])
			CACHE_STD_USE_LOOP_NOTFIND()
			pCacheElem->Descr = i;
		CACHE_STD_USE_LOOP_END()
	}

	// delete existing element from cache
	UINT tagToDel = 4024;
	CACHE_STD_REMOVE_LOOP_BEGIN(StdTestCache, &stc, tagToDel % CACHE_LINES, tagToDel)
	CACHE_STD_REMOVE_LOOP_END()

	// delete non-existing element from cache
	tagToDel = 435;
	CACHE_STD_REMOVE_LOOP_BEGIN(StdTestCache, &stc, tagToDel % CACHE_LINES, tagToDel)
	CACHE_STD_REMOVE_LOOP_END()

// 	// add existing element in cache
// 	UINT tagToAdd = 13;
// 	CACHE_STD_USE_LOOP_BEGIN(StdTestCache, &stc, tagToAdd % CACHE_LINES, tagToAdd)
// 	CACHE_STD_USE_LOOP_NOTFIND()
// 		pCacheElem->Descr = tagToAdd;
// 	CACHE_STD_USE_LOOP_END()

	// reset cache
	CACHE_STD_RESET_LOOP_BEGIN(CACHE_LINES, StdTestCache, &stc)
	CACHE_STD_RESET_LOOP_END()
#endif // _WIN_
}

void MonitorStdTest::DirectHashTableTest()
{
#ifdef _WIN_
	const size_t M = 10;
	const size_t N = 10;
	const size_t hashSize = M * N;
	DirectHashTable<int, std::string, M, N> dht;
	dht.Init("ht1");

	// fill in first half of table
	for(size_t i = 0, n = hashSize / 2; i < n; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		QVERIFY(dht.AddValue(i, std::string(b)));
	}

	// find existing item
	std::string* pVal = dht.GetValuePtr(hashSize / 4);
	QVERIFY(pVal);

	// find non-existing item
	pVal = dht.GetValuePtr(hashSize / 2);
	QVERIFY(!pVal);

	// fill in second half of table
	for(size_t i = hashSize / 2; i < hashSize; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		QVERIFY(dht.AddValue(i, std::string(b)));
	}

	// add element that can't be added
	// there is no place for it
	char b[10];
	_itoa(hashSize, b, 10);
	QVERIFY(!dht.AddValue(hashSize, std::string(b)));

	// remove non-existing element
	QVERIFY(!dht.RemoveValue(hashSize));

	// remove existing element
	QVERIFY(dht.RemoveValue(hashSize / 2));

	// add element in place of removed one
	_itoa(hashSize / 2, b, 10);
	QVERIFY(dht.AddValue(hashSize / 2, std::string(b)));

	// find all added elements
	for(size_t i = 0; i < hashSize; ++i)
		QVERIFY(dht.GetValuePtr(i));

	// remove all added elements
	for(size_t i = 0; i < hashSize; ++i)
		QVERIFY(dht.RemoveValue(i));

	// try to find removed elements
	for(size_t i = 0; i < hashSize; ++i)
		QVERIFY(!dht.GetValuePtr(i));

	// fill in table
	for(size_t i = 0; i < hashSize; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		QVERIFY(dht.AddValue(i, std::string(b)));
	}

	// chech values
	for(size_t i = 0; i < hashSize; ++i)
	{
		char b[10];
		_itoa(i, b, 10);
		std::string* s = dht.GetValuePtr(i);
		QVERIFY(s);
		QVERIFY(!strcmp(s->c_str(), b));
	}

	// clear hash
	dht.Clear();
#endif // _WIN_
}

void MonitorStdTest::DmmTest()
{
#ifdef _WIN_
	const int typeCnt = 4;
	const int maxLim = 40;

	try
	{

	for(int i = 0; i < 10; ++i)
	{
		void** ptrs = new void*[m_totalZonesSize];
		memset(ptrs, 0, sizeof(ptrs));

		for(int k = 0; k < typeCnt; ++k)
			DmmSetLowLimit(k, rand() % maxLim);

		int cntsRealSum = 0;

		const int maxCnts[4] = { 120, 20, 20, 20 };
		const int cntsMaxSum = maxCnts[0] + maxCnts[1] + maxCnts[2] + maxCnts[3];
		int realCnts[4] = { 0, 0, 0, 0 };

		while(cntsRealSum < cntsMaxSum)
		{
			int size = rand() % 4;
			if(realCnts[size] < maxCnts[size])
			{
				ptrs[cntsRealSum] = DmmAllocPages(size + 1, (DMM_PRIORITY)(rand() % DMM_PRIOR_FREE),
					DMM_ATTR_L4GB, &ptrs[cntsRealSum], TestDmmDestructor, rand() % typeCnt);
				QVERIFY(ptrs[cntsRealSum]);
				++realCnts[size];
				++cntsRealSum;
			}
		}

		const int maxCnts2[4] = { 263, 60, 50, 40 };
		const int cntsMaxSum2 = cntsMaxSum + maxCnts2[0] + maxCnts2[1] + maxCnts2[2] + maxCnts2[3];
		int realCnts2[4] = { 0, 0, 0, 0 };

		while(cntsRealSum < cntsMaxSum2)
		{
			int size = rand() % 4;
			if(realCnts2[size] < maxCnts2[size])
			{
				ptrs[cntsRealSum] = DmmAllocPages(size + 1, (DMM_PRIORITY)(rand() % DMM_PRIOR_FREE),
					DMM_ATTR_COMMON, &ptrs[cntsRealSum], TestDmmDestructor, rand() % typeCnt);
				QVERIFY(ptrs[cntsRealSum]);
				++realCnts2[size];
				++cntsRealSum;
			}
		}

		for(int k = 0; k < cntsRealSum / 2; ++k)
		{
			int j = rand() % cntsRealSum;
			if(ptrs[j] != 0)
			{
				DmmReferencePages(ptrs[j]);
			}
		}

		DmmCheckSafe();

		for(int k = 0; k < cntsRealSum; ++k)
		{
			if(ptrs[k] != 0)
			{
				DmmFreePages(ptrs[k]);
				ptrs[k] = 0;
			}
		}

		delete[] ptrs;
	}

	}
	catch( int i )
	{
		LOG_MESSAGE( DBG_FATAL, "Dmm test failed(%u)", i );
		QVERIFY( 0 );
	}
#endif // _WIN_
}

#ifdef _WIN_
void MonitorStdTest::TestDmmDestructor(PVOID pOwner, PVOID pPage, UINT)
{
	static int count = 0;
	if(*(void**)pOwner)
		*(void**)pOwner = 0;
	DmmFreePages( pPage );
}
#endif // _WIN_

void MonitorStdTest::HashTreeTest()
{
#ifdef _WIN_
	HASH_TREE ht;
	const size_t itemsPerNode = PAGE_SIZE / sizeof(HASH_TREE_ELEM);
	const size_t valsPerNode = itemsPerNode / 2;
	const size_t marg = (itemsPerNode - valsPerNode) / 2;
	const size_t valsCount = itemsPerNode * 2;
	const size_t nodes = valsCount / valsPerNode;
	QVERIFY(!HashTreeInit(&ht, itemsPerNode, -1));

	std::string* items[valsCount];
	for(size_t i = 0; i < nodes; ++i)
	{
		for(size_t j = 0; j < valsPerNode; ++j)
		{
			char b[10];
			_itoa(itemsPerNode * i + marg + j, b, 10);
			items[valsPerNode * i + j] = new std::string(b);
			HashTreeSet(&ht, itemsPerNode * i + marg + j, items[valsPerNode * i + j]);
		}
	}

	// find existing items
	for(size_t i = 0; i < nodes; ++i)
	{
		for(size_t j = 0; j < valsPerNode; ++j)
		{
			char b[10];
			_itoa(itemsPerNode * i + marg + j, b, 10);
			std::string* pstr = (std::string*)(HashTreeGet(&ht, itemsPerNode * i + marg + j)->pData);
			QVERIFY(pstr == items[valsPerNode * i + j]);
		}
	}

	// find non existing items
	for(size_t i = 0; i < nodes; ++i)
	{
		for(size_t j = 0; j < marg; ++j)
		{
			QVERIFY(HashTreeGet(&ht, itemsPerNode * i + 0 + j)->u64Data == -1);
			QVERIFY(HashTreeGet(&ht, itemsPerNode * i + valsPerNode + marg + j)->u64Data == -1);
		}
	}

	// remove item
	HashTreeElemClear(&ht, HashTreeGet(&ht, 160));

	// find removed item
	QVERIFY(HashTreeGet(&ht, 160)->u64Data == -1);

	// set item
	HashTreeSet(&ht, 160, items[32]);

	// find set item
	QVERIFY(HashTreeGet(&ht, 160)->pData == items[32]);
#endif // _WIN_
}

void MonitorStdTest::ListMemManTest()
{
#ifdef _WIN_
	typedef double LMMElemType;
	LIST_MEMORY_MANAGER<LMMElemType> mm;
	typedef LIST_MEM_MAN_ITEM<LMMElemType> MemManItem;
	const size_t itemCount = 256;
	LMMElemType items[itemCount];
	LMMElemType* pItems[itemCount];

	// init list
	// this include call of ListMemMan_InitEx
	ListMemMan_Init(&mm, items, itemCount);

	// allocate itemCount items
	for(size_t i = 0; i < itemCount; ++i)
		pItems[i] = ListMemMan_Allocate(&mm);
	// no one item can be NULL
	for(size_t i = 0; i < itemCount; ++i)
		QVERIFY(pItems[i] != NULL);
	// there are no equal items
	for(size_t i = 0, n = itemCount - 1; i < n; ++i)
		for(size_t j = i + 1; j < itemCount; ++j)
			QVERIFY(pItems[i] != pItems[j]);

	// try to allocate one more item
	LMMElemType* p1 = ListMemMan_Allocate(&mm);
	QVERIFY(p1 == NULL);

	// free even items
	for(size_t i = 0; i < itemCount; i += 2)
		ListMemMan_Free(&mm, pItems[i]);

	// allocate even items
	for(size_t i = 0; i < itemCount; i += 2)
		pItems[i] = ListMemMan_Allocate(&mm);

	// try to allocate one more item
	p1 = ListMemMan_Allocate(&mm);
	QVERIFY(p1 == NULL);

	// free odd items
	for(size_t i = 1; i < itemCount; i += 2)
		ListMemMan_Free(&mm, pItems[i]);

	// allocate odd items
	for(size_t i = 1; i < itemCount; i += 2)
		pItems[i] = ListMemMan_Allocate(&mm);

	// try to allocate one more item
	p1 = ListMemMan_Allocate(&mm);
	QVERIFY(p1 == NULL);

// 		// free NULL item
// 		ListMemMan_Free(&mm, (LMMElemType*)NULL);

	// free all items
	for(size_t i = 0; i < itemCount; ++i)
		ListMemMan_Free(&mm, pItems[i]);

	// allocate itemCount items
	for(size_t i = 0; i < itemCount; ++i)
		pItems[i] = ListMemMan_Allocate(&mm);
#endif // _WIN_
}

void MonitorStdTest::LookasideTest()
{
#ifdef _WIN_
	int j, i;

	UINT aPagesOffs[] =
	{
		offsetof( ValOffs, ptr1 ),
		offsetof( ValOffs, ptr2 ),
		offsetof( ValOffs, ptr3 )
	};


	LOOKASIDE_LIST lal;

	try{

	DmmInitLookasideList(&lal, sizeof(QWORD), DMM_PRIOR_NORMAL, 1,
		(DESTRUCT_ELEM)TestLookasideDestructor, 64, FALSE, FALSE, DMM_ATTR_COMMON, 16 );

	for(j = 0; j < 10; ++j)
	{
		const int reqCount = 1000;
		QWORD* elems[reqCount];
		memset(elems, 0, sizeof(reqCount));

		for(i = 0; i < reqCount; ++i)
		{
			elems[i] = (QWORD*)DmmAllocFromLookasideList(&lal);
			QVERIFY(elems[i]);
			if( ((ULONG_PTR)elems[i] % 16 ) != 0 )
				Abort("Unaligned ptr");
			*elems[i] = (QWORD)i;
		}

		int cnt = reqCount;
		while(cnt > 0)
		{
			size_t j = rand() % reqCount;
			if(elems[j])
			{
				if( *elems[j] != (QWORD)j )
					Abort("Incoherence %u vs %u", j, (UINT)*elems[j] );
				DmmFreeToLookasideList(elems[j]);
				elems[j] = 0;
				--cnt;
			}
		}

		for(i = 0; i < reqCount; ++i)
		{
			if( elems[i] == NULL )
				elems[i] = (QWORD*)DmmAllocFromLookasideList(&lal);
			else
				Abort("Null element");
			QVERIFY(elems[i]);
		}

		for(i = 0; i < reqCount / 2; ++i)
			DmmReferenceLookasideList(elems[rand() % reqCount]);

		DmmLockEntryLookasideList(elems[0]);

		DmmCheckSafe();

		DmmUnlockEntryLookasideList(elems[0]);

		DmmForEachInLookasideList(&lal, TestLookasideElemAction);

		DmmResetLookasideList(&lal);
	}

	DmmInitLookasideListEx(&lal, sizeof(ValOffs), sizeof(aPagesOffs) / sizeof(aPagesOffs[0]),
		aPagesOffs, DMM_PRIOR_NORMAL, 2, (DESTRUCT_ELEM)TestLookasideDestructor);

	for(j = 0; j < 10; ++j)
	{
		const int reqCount = 100;
		ValOffs* elems[reqCount];
		memset(elems, 0, sizeof(reqCount));

		for(i = 0; i < reqCount; ++i)
		{
			elems[i] = (ValOffs*)DmmAllocFromLookasideList(&lal);
			QVERIFY(elems[i]);
			elems[i]->v = i;
			MemSet( elems[i]->ptr1, (char)i, PAGE_SIZE );
		}

		int cnt = reqCount;
		while(cnt > 0)
		{
			size_t j = rand() % reqCount;
			if(elems[j])
			{
				if( ( elems[j]->v != j ) || ((PUCHAR)elems[j]->ptr1)[80] != j )
					Abort("Incoherence in paged lookaside");
				DmmFreeToLookasideList(elems[j]);
				elems[j] = 0;
				--cnt;
			}
		}
		for(i = 0; i < reqCount; ++i)
		{
			elems[i] = (ValOffs*)DmmAllocFromLookasideList(&lal);
			QVERIFY(elems[i]);
		}

		for(i = 0; i < reqCount / 2; ++i)
			DmmReferenceLookasideList(elems[rand() % reqCount]);

		DmmLockEntryLookasideList(elems[0]);

		DmmCheckSafe();

		DmmUnlockEntryLookasideList(elems[0]);

		DmmForEachInLookasideList(&lal, TestLookasideElemAction);

		DmmResetLookasideList(&lal);
	}


    DmmInitLookasideListEx(&lal, sizeof(ValOffs), sizeof(aPagesOffs) / sizeof(aPagesOffs[0]),
        aPagesOffs, DMM_PRIOR_NORMAL, 2, (DESTRUCT_ELEM)TestLookasideDestructor,
        14, TRUE, FALSE, DMM_ATTR_COMMON, 32 );

    for(j = 0; j < 10; ++j)
    {
        const int reqCount = 100;
        ValOffs* elems[reqCount];
        memset(elems, 0, sizeof(reqCount));

        for(i = 0; i < reqCount; ++i)
        {
            elems[i] = (ValOffs*)DmmAllocFromLookasideList(&lal);
            QVERIFY(elems[i]);
            if( ULONG_PTR(elems[i]) % 32 != 0 )
                Abort("Unaligned 2. %p %% 32", elems[i]  );
            elems[i]->v = i;
            MemSet( elems[i]->ptr1, (char)i, PAGE_SIZE );
        }

        int cnt = reqCount;
        while(cnt > 0)
        {
            size_t j = rand() % reqCount;
            if(elems[j])
            {
                if( ( elems[j]->v != j ) || ((PUCHAR)elems[j]->ptr1)[80] != j )
                    Abort("Incoherence in paged lookaside");
                DmmFreeToLookasideList(elems[j]);
                elems[j] = 0;
                --cnt;
            }
        }
        for(i = 0; i < reqCount; ++i)
        {
            elems[i] = (ValOffs*)DmmAllocFromLookasideList(&lal);
            QVERIFY(elems[i]);
        }

        for(i = 0; i < reqCount / 2; ++i)
            DmmReferenceLookasideList(elems[rand() % reqCount]);

        DmmLockEntryLookasideList(elems[0]);

        DmmCheckSafe();

        DmmUnlockEntryLookasideList(elems[0]);

        DmmForEachInLookasideList(&lal, TestLookasideElemAction);

        DmmResetLookasideList(&lal);
    }

	}
	catch( int i )
	{
		LOG_MESSAGE( DBG_FATAL, "Failed Lookaside (%u)", i );
		QVERIFY( 0 );
	}
#endif // _WIN_
}

#ifdef _WIN_
void MonitorStdTest::TestLookasideDestructor( ValOffs* pEntry, UINT )
{
	DmmFreeToLookasideList(pEntry);
}
#endif // _WIN_

#ifdef _WIN_
void MonitorStdTest::TestLookasideElemAction( void * pV )
{
	if( pV == NULL )
		Abort("Null elem %s", __FUNCTION__);
}
#endif // _WIN_

void MonitorStdTest::MemManTest()
{
#ifdef _WIN_
	typedef double MMElemType;
	const size_t itemCount = 256;
	const size_t elemSize = sizeof(MMElemType);
	DECLARE_MEMORY_MANAGER(itemCount) mm;
	char buffer[elemSize * itemCount];
	// init list
	MemMan_Init(&mm, buffer, elemSize, itemCount);
	QVERIFY(MemMan_IsFull(&mm) == 0);

	PVOID pItems[itemCount];

	// allocate itemCount items
	for(size_t i = 0; i < itemCount; ++i)
		pItems[i] = MemMan_AllocateMem(&mm);
	// no one item can be NULL
	for(size_t i = 0; i < itemCount; ++i)
		QVERIFY(pItems[i] != NULL);
	// there are no equal items
	for(size_t i = 0, n = itemCount - 1; i < n; ++i)
		for(size_t j = i + 1; j < itemCount; ++j)
			QVERIFY(pItems[i] != pItems[j]);
	// memory manager's buffer is full
	QVERIFY(MemMan_IsFull(&mm) == 1);

	// try to allocate one more item
	PVOID p1 = MemMan_AllocateMem(&mm);
	QVERIFY(p1 == NULL);

	// free even items
	for(size_t i = 0; i < itemCount; i += 2)
		MemMan_FreeMem(&mm, pItems[i]);

	// allocate even items
	for(size_t i = 0; i < itemCount; i += 2)
		pItems[i] = MemMan_AllocateMem(&mm);
	QVERIFY(MemMan_IsFull(&mm) == 1);

	// try to allocate one more item
	p1 = MemMan_AllocateMem(&mm);
	QVERIFY(p1 == NULL);

	// free odd items
	for(size_t i = 1; i < itemCount; i += 2)
		MemMan_FreeMem(&mm, pItems[i]);

	// allocate odd items
	for(size_t i = 1; i < itemCount; i += 2)
		pItems[i] = MemMan_AllocateMem(&mm);
	QVERIFY(MemMan_IsFull(&mm) == 1);

	// try to allocate one more item
	p1 = MemMan_AllocateMem(&mm);
	QVERIFY(p1 == NULL);

	// free NULL item
	MemMan_FreeMem(&mm, NULL);

	// free all items
	for(size_t i = 0; i < itemCount; ++i)
		MemMan_FreeMem(&mm, pItems[i]);
	QVERIFY(MemMan_IsFull(&mm) == 0);

	// allocate itemCount items
	for(size_t i = 0; i < itemCount; ++i)
		pItems[i] = MemMan_AllocateMem(&mm);

	// reset memory manager
	MemMan_Reset(&mm);
	QVERIFY(MemMan_IsFull(&mm) == 0);
#endif // _WIN_
}

QTEST_MAIN(MonitorStdTest)
