///////////////////////////////////////////////////////////////////////////////
///
/// @file CDsaWrapTest.cpp
///
/// Class CDsaWrapTest implementation.
///
/// @owner lenkor@
/// @author dbrylev@
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
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

#include <Libraries/License/Activation/CDsaWrap.h>
#include "CDsaWrapTest.h"


class CSha256To160 : public CSha256
{
public:
	CSha256To160()
	{}
	virtual bool Final(QByteArray & baDigest)
	{
		bool bResult = CSha256::Final(baDigest);
		if (bResult)
			baDigest.truncate(20);
		return bResult;
	}
};

namespace
{

QString LoadFile(const QString & name, QByteArray & contents)
{
	QFile f(name);
	if (!f.open(QIODevice::ReadOnly))
		return QString("cannot open file %1").arg(name);
	contents = f.readAll();
	return "OK";
}

#define DECLARE_AND_LOAD_ARR(n, f)\
	QByteArray n;\
	{\
		QString qsRes = LoadFile(f, n);			\
		if (qsRes != QString("OK"))				\
			return qsRes;						\
	}


static QString DoVerifyTest(const QString & keyName, const QString & msgName,
							const QString & sigName, IHashAlgorithm * pHash)
{
	DECLARE_AND_LOAD_ARR(baPubKey, keyName);
	DECLARE_AND_LOAD_ARR(baMsg, msgName);
	DECLARE_AND_LOAD_ARR(baSignature, sigName);

	CDsaWrap dsa;
	if (!dsa.AssignPublicKeyPEM(baPubKey))
		return QString("AssignPublicKeyPEM() failed");

	bool bVerifyRes = false;
	if (!dsa.Verify(baMsg, pHash, baSignature, bVerifyRes))
		return QString("Verify() failed");
	if (!bVerifyRes)
		return QString("Wrong signature");

	return "OK";
}

static QString DoVerifyTest(const QString & suffix, IHashAlgorithm * pHash)
{
	return DoVerifyTest( QString("ssldata/pub_%1.key").arg(suffix),
						  QString("ssldata/file.txt"),
						  QString("ssldata/sig_%1.dat").arg(suffix),
						  pHash);
}

}


void CDsaWrapTest::init()
{}


void CDsaWrapTest::cleanup()
{}


void CDsaWrapTest::testSignVerify()
{
	const quint32 kKeyLength = 1024;
	const QByteArray kSeed("doiehcogbbbcdeui904704ho900478hdh930dh3dh98");
	
	CDsaWrap dsa;
	QVERIFY( dsa.GenerateKey(kKeyLength, kSeed) );

	CSha1 hash;
	QByteArray message1("123456"), message2("abcdef");
	QByteArray digest1, digest2;

	QVERIFY( dsa.Sign(message1, &hash, digest1) );
	QVERIFY( dsa.Sign(message2, &hash, digest2) );

	bool bVerifyRes = false;

	QVERIFY( dsa.Verify(message1, &hash, digest1, bVerifyRes) );
	QVERIFY( bVerifyRes );

	QVERIFY( dsa.Verify(message1, &hash, digest2, bVerifyRes) );
	QVERIFY( !bVerifyRes );

	QVERIFY( dsa.Verify(message2, &hash, digest1, bVerifyRes) );
	QVERIFY( !bVerifyRes );

	QVERIFY( dsa.Verify(message2, &hash, digest2, bVerifyRes) );
	QVERIFY( bVerifyRes );
}

void CDsaWrapTest::testSignVerifyTruncHash()
{
	const quint32 kKeyLength = 1024;
	const QByteArray kSeed("doiehcogbbbcdeui904704ho900478hdh930dh3dh98");
	
	CDsaWrap dsa;
	QVERIFY( dsa.GenerateKey(kKeyLength, kSeed) );

	CSha256To160 hash;
	QByteArray message1("123456"), message2("abcdef");
	QByteArray digest1, digest2;

	QVERIFY( dsa.Sign(message1, &hash, digest1) );
	QVERIFY( dsa.Sign(message2, &hash, digest2) );

	bool bVerifyRes = false;

	QVERIFY( dsa.Verify(message1, &hash, digest1, bVerifyRes) );
	QVERIFY( bVerifyRes );

	QVERIFY( dsa.Verify(message1, &hash, digest2, bVerifyRes) );
	QVERIFY( !bVerifyRes );

	QVERIFY( dsa.Verify(message2, &hash, digest1, bVerifyRes) );
	QVERIFY( !bVerifyRes );

	QVERIFY( dsa.Verify(message2, &hash, digest2, bVerifyRes) );
	QVERIFY( bVerifyRes );
}


void CDsaWrapTest::testLoadVerify()
{
	CSha1 sha1_hash;
	CSha256 sha256_hash;

	QCOMPARE(DoVerifyTest("1024_sha1", &sha1_hash), QString("OK"));
	QCOMPARE(DoVerifyTest("3072_sha1", &sha1_hash), QString("OK"));
	QCOMPARE(DoVerifyTest("1024_sha256", &sha256_hash), QString("OK"));
	QCOMPARE(DoVerifyTest("3072_sha256", &sha256_hash), QString("OK"));
	QCOMPARE(DoVerifyTest("ssldata/real_pub.key", "ssldata/real_msg.txt",
						  "ssldata/real_sig.dat", &sha256_hash),
			 QString("OK"));
}

