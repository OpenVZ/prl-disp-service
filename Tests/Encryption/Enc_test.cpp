/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QDir>
#include <QDirIterator>

#include <Libraries/Encryption/Encryption.h>
#include <Libraries/HostUtils/HostUtils.h>
#include <Libraries/PrlUuid/Uuid.h>

#include "Parser.h"

static bool CommonTest(const QString& dirName, ICrypt* Eng)
{
	QDirIterator dir(dirName, QDir::Files);
	TestPattern Pattern;
	ICryptInfo Info;

	if (PRL_FAILED(Eng->GetInfo(&Info)))
	{
		qCritical("Can't get information about cipher");
		return false;
	}

	if (!dir.hasNext())
	{
		qCritical("Empty directory with test data");
		return false;
	}

	// Iterate through all files
	while(dir.hasNext())
	{
		QString name = dir.next();

		printf("Test file: %s\n", qPrintable(name));

		if (!LoadTestFile(name, Pattern))
		{
			qWarning("Error parsing test file %s!", qPrintable(name));
			return false;
		}

		printf("The requested operation is: %s\n", Pattern.Encrypt?"ENCRYPT":"DECRYPT");

		// Print pattern:
		QList<TestData>::const_iterator it;
		PRL_UINT8_PTR pIV;

		for(it = Pattern.Data.begin(); it != Pattern.Data.end(); it++)
		{
			printf("Count: %u\n", it->Count);

			if (it->Key.size() != Info.KeySize)
			{
				qCritical("Key size mismatch in test!");
				return false;
			}

			Eng->SetKey((PRL_UINT8_PTR)it->Key.data());

			if (!it->IV.size())
			{
				pIV = NULL;
			}
			else
			{
				if (it->IV.size() != Info.BlockSize)
				{
					qCritical("IV size mismatch in test!");
					return false;
				}
				pIV = (PRL_UINT8_PTR)it->IV.data();
			}

			if (Pattern.Encrypt)
				Eng->Encrypt((void*)it->Plain.data(),
							it->Plain.size(), pIV);
			else
				Eng->Decrypt((void*)it->Cypher.data(),
							it->Cypher.size(), pIV);

			// Compare results
			if (it->Plain != it->Cypher)
			{
				qWarning("Iteration %u failed in file %s!", it->Count, qPrintable(name));
				HostUtils::DumpArray((PRL_UINT8_PTR)it->Plain.data(), it->Plain.size());
				HostUtils::DumpArray((PRL_UINT8_PTR)it->Cypher.data(), it->Cypher.size());
				return false;
			}
		}
	}

	return true;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		qWarning("Need to specify the directory for test patterns");
		return -1;
	}

//	__asm { int 3 } ;

	QDirIterator dir(argv[1], QDir::Dirs);

	if (!dir.hasNext())
	{
		qCritical("Empty directory with ciphers");
		return false;
	}

	QString dname, path;
	PRL_RESULT Err;
	Uuid Type;
	ICrypt* Eng;

	static const QString strAes128("AES128");
	static const PRL_GUID Aes128 = { 0xf8b1f190, 0xd246, 0x40f6,
		{ 0xaf, 0x6c, 0xc0, 0x89, 0xe3, 0x77, 0x36, 0xbc }
	};

	// Iterate through all files
	while(dir.hasNext())
	{
		dname = dir.next();

		if (dname.endsWith(".") ||
			(dname.endsWith("..")))
			continue;

		if (dname.endsWith(strAes128))
		{
			Type = Uuid::fromGuid(Aes128);
			goto Test;
		}

		qCritical("The test data not found in dir %s", qPrintable(dname));
		continue;

	Test:
		Eng = Encryption::CreateObject(Type, Err);

		if (!Eng)
		{
			qCritical("Can't create %s instance", qPrintable(dname));
			continue;
		}

		if (!CommonTest(dname, Eng))
			qCritical("%s test failed!", qPrintable(dname));
		else
			qCritical("%s test succeeded!", qPrintable(dname));

		Eng->Release();
	}

	return 0;
}
