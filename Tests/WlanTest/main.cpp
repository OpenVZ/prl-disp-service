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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Libraries/WifiHelper/CWifiHelper.h"
#include "Libraries/WifiHelper/CWifiStoreHelper.h"
#include <prlcommon/Interfaces/ParallelsQt.h>

using namespace Parallels;

void print_usage(const char *sAppName)
{
	fprintf(stderr, "Usage: %s command args\n"
					"Possible commands:\n"
					"find-adapter mac_addr(00-hh-hh-hh-hh-hh)\n"
					"list\n"
					"list-profiles mac_addr\n"
					"store-settings path-to-store\n"
					"apply-settings path-to-load\n"
					, sAppName);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		print_usage(argv[0]);
		return -1;
	}

	if ( !strcmp("find-adapter", argv[1])  )
	{
		if (argc < 3)
		{
			print_usage(argv[0]);
			return -1;
		}
		SmartPtr<TWifiIfaceInfo> pIfaceInfo = CWifiHelper::GetAdapterInfo(UTF8_2QSTR(argv[2]));
		if (!pIfaceInfo)
		{
			fprintf(stderr, "Adapter wasn't found\n");
			return -1;
		}
		fprintf(stdout, "adapter info:\nGUID='%s'\nDescription='%s'\nPhysical address: '%s'\n",
						QSTR2UTF8(pIfaceInfo->m_sGuid),
						QSTR2UTF8(pIfaceInfo->m_sDescription),
						QSTR2UTF8(pIfaceInfo->m_sPhysicalAddress));
	}
	else if ( !strcmp("list", argv[1]) )
	{
		QList<SmartPtr<TWifiIfaceInfo> > _lst = CWifiHelper::GetAdaptersList();
		quint32 nCounter = 1;
		foreach(const SmartPtr<TWifiIfaceInfo> &pIfaceInfo, _lst)
		{
			fprintf(stdout, "adapter %d info:\nGUID='%s'\nDescription='%s'\nPhysical address: '%s'\n",
						nCounter++,
						QSTR2UTF8(pIfaceInfo->m_sGuid),
						QSTR2UTF8(pIfaceInfo->m_sDescription),
						QSTR2UTF8(pIfaceInfo->m_sPhysicalAddress));
		}
	}
	else if ( !strcmp("list-profiles", argv[1])  )
	{
		if (argc < 3)
		{
			print_usage(argv[0]);
			return -1;
		}
		SmartPtr<TWifiIfaceInfo> pIfaceInfo = CWifiHelper::GetAdapterInfo(UTF8_2QSTR(argv[2]));
		if (!pIfaceInfo)
		{
			fprintf(stderr, "Adapter wasn't found\n");
			return -1;
		}
		fprintf(stdout, "adapter info:\nGUID='%s'\nDescription='%s'\nPhysical address: '%s'\n",
						QSTR2UTF8(pIfaceInfo->m_sGuid),
						QSTR2UTF8(pIfaceInfo->m_sDescription),
						QSTR2UTF8(pIfaceInfo->m_sPhysicalAddress));
		foreach( const SmartPtr<TWifiProfileInfo> &pProfile, pIfaceInfo->m_Profiles )
		{
			fprintf( stdout, "profile '%s' '%s':\n[%s]\n", QSTR2UTF8(pProfile->m_sName), QSTR2UTF8(pProfile->m_sType), QSTR2UTF8(pProfile->m_sContent) );
		}
	}
	else if ( !strcmp("store-settings", argv[1]) )
	{
		if (argc < 3)
		{
			print_usage(argv[0]);
			return -1;
		}
		QList<SmartPtr<TWifiIfaceInfo> > _lst = CWifiHelper::GetAdaptersList();
		if ( CWifiStoreHelper::StoreWifiSettings( _lst, UTF8_2QSTR(argv[2]) ) )
			fprintf( stdout, "Settings were successfully stored\n" );
		else
			fprintf( stderr, "Failed to store Wi-Fi settings\n" );
	}
	else if ( !strcmp("apply-settings", argv[1]) )
	{
		if (argc < 3)
		{
			print_usage(argv[0]);
			return -1;
		}

		QList<SmartPtr<TWifiIfaceInfo> > _lst = CWifiStoreHelper::LoadWifiSettings( UTF8_2QSTR(argv[2]) );
		if ( CWifiHelper::ApplySettings( _lst ) )
			fprintf( stdout, "Settings were successfully applied\n" );
		else
			fprintf( stderr, "Failed to apply Wi-Fi settings\n" );
	}
	else
		print_usage(argv[0]);

	return 0;
}

