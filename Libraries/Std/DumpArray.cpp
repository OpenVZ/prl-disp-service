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


#include "DumpArray.h"
#include <Libraries/Logging/Logging.h>

// Write array to logfile
void DumpArray(const void* Data, size_t Size)
{
	// Dump in format:
	// 0000addr 00 00 00 00 00 00 00 00 | 00 00 00 00 00 00 00 00  textrepresentat'

#define LINE_SIZE 16

	unsigned lines = (unsigned)((Size + LINE_SIZE - 1) / LINE_SIZE);
	unsigned Processed = LINE_SIZE;
	unsigned char lastLine[LINE_SIZE];
	unsigned char str[LINE_SIZE + 1];
	unsigned char* curData = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(Data));

	memset(lastLine, 0, LINE_SIZE);
	memset(str, 0, LINE_SIZE + 1);

	for(unsigned i = 0; i < lines;
		i++, curData += LINE_SIZE, Processed += LINE_SIZE)
	{
		// Last line?
		if (Processed > Size)
		{
			curData = lastLine;
			memcpy(curData, Data, Size % LINE_SIZE);
		}

		memcpy(str, curData, LINE_SIZE);

		for(int j = 0; j < LINE_SIZE; j++)
			// Convert all unprintable symbols to spaces
			if (str[j] < 0x20)
				str[j] = '.';

		WRITE_TRACE(DBG_FATAL, "%08x %02x %02x %02x %02x %02x %02x %02x %02x |"
						" %02x %02x %02x %02x %02x %02x %02x %02x %s",
						i * 16,
						curData[0], curData[1], curData[2], curData[3], curData[4],
						curData[5], curData[6], curData[7], curData[8], curData[9],
						curData[10], curData[11], curData[12], curData[13], curData[14],
						curData[15], str);
	}

#undef LINE_SIZE
}
