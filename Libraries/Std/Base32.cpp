///////////////////////////////////////////////////////////////////////////////
///
/// @file Base32.cpp
///
/// Implement base32 encoding by Crockford design
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
///////////////////////////////////////////////////////////////////////////////

#include <prlsdk/PrlTypes.h>
#include <Libraries/Std/Base32.h>

// Base32 element size (bits per symbol)
#define BASE32_ELSIZE	5
#define BYTE_SIZE		8
// Nonvalue in Crockford
#define CROCK_NAN		128

// Table converting 0-32 to ascii (plus zero string end)
static const char s_Table[32 + 1] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";


// Table covering ASCII from 0 to z
static const unsigned char s_RevTable[75] = {
			// 0-9
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
			// :;<=>?@
			CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN,
			// A-Z
			10, 11, 12, 13, 14, 15, 16, 17, 1, 18, 19, 1, 20, 21, 0, 22, 23, 24, 25, 26,
			CROCK_NAN, 27, 28, 29, 30, 31,
			// [\]^_'
			CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN, CROCK_NAN,
			// a-z
			10, 11, 12, 13, 14, 15, 16, 17, 1, 18, 19, 1, 20, 21, 0, 22, 23, 24, 25, 26,
			CROCK_NAN, 27, 28, 29, 30, 31 };

/*
 * Encode array into base32 representation
 */
QString Base32::Encode(const QByteArray& In)
{
	QString Out;
	// Round up
	PRL_UINT32 Pad = BASE32_ELSIZE - In.size() % BASE32_ELSIZE;
	QByteArray Padded = In + QByteArray(Pad, 0);

	PRL_UINT8_PTR Data = (PRL_UINT8_PTR)Padded.data();
	// Bits count rounded up and divided to base32 element size
	PRL_UINT32 Count = Padded.size() / BASE32_ELSIZE;
	Out.reserve(Count * BYTE_SIZE);

	for (PRL_UINT32 i = 0; i < Count; i++)
	{
		PRL_UINT32 Base = i * BASE32_ELSIZE;
		PRL_UINT64 Tmp = 0;

		Tmp |= ((PRL_UINT64)Data[Base + 0]) << 32;
		Tmp |= ((PRL_UINT64)Data[Base + 1]) << 24;
		Tmp |= ((PRL_UINT64)Data[Base + 2]) << 16;
		Tmp |= ((PRL_UINT64)Data[Base + 3]) << 8;
		Tmp |= ((PRL_UINT64)Data[Base + 4]);

		Out.append(s_Table[(Tmp & 0xF800000000ull) >> 35]);
		Out.append(s_Table[(Tmp & 0x07C0000000ull) >> 30]);
		Out.append(s_Table[(Tmp & 0x003E000000ull) >> 25]);
		Out.append(s_Table[(Tmp & 0x0001F00000ull) >> 20]);
		Out.append(s_Table[(Tmp & 0x00000F8000ull) >> 15]);
		Out.append(s_Table[(Tmp & 0x0000007C00ull) >> 10]);
		Out.append(s_Table[(Tmp & 0x00000003E0ull) >> 5]);
		Out.append(s_Table[(Tmp & 0x000000001Full)]);
	}

	// Remove padded data from out string
	Out.chop((Pad * BYTE_SIZE) / BASE32_ELSIZE);

	return Out;
}

/*
 * Decode array from base32 representation
 */
QByteArray Base32::Decode(const QString& In)
{
	// Get size in 40 bits chunk (bytesize * base32_elsize)
	PRL_UINT32 Size = (In.size() + BYTE_SIZE - 1) / BYTE_SIZE;
	PRL_UINT32 Pad = Size * BYTE_SIZE - In.size();
	// Convert to bytes [(size * BASE32_ELSIZE * BYTE_SIZE) / BYTE_SIZE]
	Size = Size * BASE32_ELSIZE;
	QByteArray Decode = In.toLatin1() + QByteArray(Pad, '0');
	PRL_UINT32 Count = Decode.size() / BYTE_SIZE;
	PRL_UINT8_PTR Data = (PRL_UINT8_PTR)Decode.data();

	QByteArray Out;
	Out.reserve(Size);

	for (PRL_UINT32 i = 0; i < Count; i++)
	{
		PRL_UINT64 Tmp = 0;
		PRL_UINT32 Base = i * BYTE_SIZE;

		/*
		 * Checked version is a bit slow. Unchecked - dangerous.
		 */
		for(int j = 0; j < BYTE_SIZE; j++)
		{
			PRL_UINT8 Val = Data[Base + j];

			// Validate
			if ((Val < '0') && (Val > 'z'))
				return QByteArray();

			Val = s_RevTable[Val - '0'];

			if (Val == CROCK_NAN)
				return QByteArray();

			Tmp <<= BASE32_ELSIZE;
			Tmp |= Val;
		}

		// Put value to array
		Out.append((PRL_UINT8)((Tmp & 0xFF00000000ull) >> 32));
		Out.append((PRL_UINT8)((Tmp & 0x00FF000000ull) >> 24));
		Out.append((PRL_UINT8)((Tmp & 0x0000FF0000ull) >> 16));
		Out.append((PRL_UINT8)((Tmp & 0x000000FF00ull) >> 8));
		Out.append((PRL_UINT8)((Tmp & 0x00000000FFull)));
	}

	// Remove padding
	Out.truncate((In.size() * BASE32_ELSIZE + BASE32_ELSIZE - 1) / BYTE_SIZE);
	return Out;
}
