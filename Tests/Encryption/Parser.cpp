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

#include <QFile>
#include <QRegExp>
#include <QString>


#include "Parser.h"

static const QString s_Count = QString("COUNT");
static const QString s_Key = QString("KEY");
static const QString s_IV = QString("IV");
static const QString s_Cipher = QString("CIPHERTEXT");
static const QString s_Plain = QString("PLAINTEXT");

static QByteArray ParseBinary(QString& in)
{
	#define BYTESIZE 2

	QByteArray out;
	QString byte;
	int Size = in.size();
	bool ok = true;

	if (Size < BYTESIZE)
	{
		out.append((char)in.toUInt(&ok, 16));
		goto Exit;
	}

	Size /= BYTESIZE;

	for(int i = 0; ok && (i < Size); i++)
	{
		byte = in.mid(i * BYTESIZE, BYTESIZE);
		out.append((char)byte.toUInt(&ok, 16));
	}

Exit:
	if (!ok)
	{
		qCritical("Cant convert %s to plain array", qPrintable(in));
		out.clear();
	}

	return out;
}

static void InsertEntry(TestData& TD, TestPattern& Pattern)
{
	if (!TD.Key.size() ||
		!TD.Cypher.size() ||
		!TD.Plain.size())
	{
		// Not filled, this happens on second line
		return;
	}

	// All data is filled
	Pattern.Data.append(TD);

	// And cleanup
	TD.Key.clear();
	TD.IV.clear();
	TD.Cypher.clear();
	TD.Plain.clear();
	TD.Count = 0;
}

bool LoadTestFile(const QString& name, TestPattern& Out)
{
	QFile f(name);
	QRegExp parser("\\s*(\\S+)\\s*=\\s*(\\S+)\\s*");

	if (!f.open(QIODevice::ReadOnly))
	{
		qCritical("Can't open file %s", qPrintable(name));
		return false;
	}

	QString curLine = QString(f.readLine());

	if (curLine.contains("encrypt", Qt::CaseInsensitive))
		Out.Encrypt = true;
	else
		Out.Encrypt = false;

	QString Name;
	QString Data;

	// Prepare output
	Out.Data.clear();
	TestData TD;

	while(!f.atEnd())
	{
		curLine = QString(f.readLine());
		// Remove trash
		curLine = curLine.trimmed();

		if (curLine.isEmpty())
		{
			InsertEntry(TD, Out);
			continue;
		}

		if (parser.indexIn(curLine) == -1)
			continue;

		Name = parser.cap(1);
		Data = parser.cap(2);

		if (Name == s_Count)
		{
			TD.Count = Data.toUInt();
			continue;
		}

		if (Name == s_Key)
		{
			TD.Key = ParseBinary(Data);
			continue;
		}

		if (Name == s_IV)
		{
			TD.IV = ParseBinary(Data);
			continue;
		}

		if (Name == s_Cipher)
		{
			TD.Cypher = ParseBinary(Data);
			continue;
		}

		if (Name == s_Plain)
		{
			TD.Plain = ParseBinary(Data);
			continue;
		}

		// Nothing mathed?
		qCritical("Strange key in test file: %s", qPrintable(Name));
	}

	// The last entry may not be processed if there are no empty line
	InsertEntry(TD, Out);
	f.close();

	if (!Out.Data.size())
		return false;

	return true;
}
