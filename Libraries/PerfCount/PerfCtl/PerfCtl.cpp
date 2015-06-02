//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCtl.cpp
///
/// @brief Performance counters utility
///
/// @author maximk
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
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
//////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <algorithm>

#ifdef _WIN_
	#include <windows.h>
	#define sleep(x) Sleep(x*1000)
#else
    #include <unistd.h>
#endif

#include "PerfLib/PerfCountersOut.h"
#include "Build/Current.ver"


/**
 * Print help to the utility in console
 */
void help(const char* reason = 0)
{
    if ( reason )
        printf( "Error: %s\n", reason );
    printf( "Usage: prl_perf_ctl -a | -s <filter> | -c <filter> [-l] [-n] [-d]\n" );
    printf( "\t--help (-h)\tprint this help message\n" );
    printf( "\t--all (-a)\tdump all storages and counters\n" );
    printf( "\t--storage (-s) <filter>\tfilter storages by substring name\n" );
    printf( "\t--counter (-c) <filter>\tfilter counters by substring name\n" );
    printf( "\t--loop (-l) <sec>\toutput values in the loop with (sec) interval\n" );
	printf( "\t--nozero (-n)\tdo not output zero values (much easier to read in the loop)\n" );
	printf( "\t--digits (-d)\toutput digits in groups like 123,456,789 (instead of 123456789)\n" );
    printf( "Example:\n" );
    printf( "\t<prl_perf_ctl> -c vcpu\t-\tfilter all vcpu-related counters\n" );
    printf( "\t<prl_perf_ctl> -s XP\t-\tshow only storages with 'XP' symbols in name\n" );
}


/**
 * Get argument value from the vector, by 2 possible
 * parameter values (normally - short and long forms)
 * @return parameter value, or empty string
 */
string GET_ARG( vector<string>& args, string str1, string str2 )
{
    string ret = "";
    vector<string>::iterator pos;

    pos = std::find( args.begin(), args.end(), string(str1) );
    if (pos != args.end())
    {
        pos++;
        if (pos != args.end())
            ret = *pos;
    }
    if (ret != "")
        return ret;

    pos = std::find( args.begin(), args.end(), string(str2) );
    if (pos != args.end())
    {
        pos++;
        if (pos != args.end())
            ret = *pos;
    }
    if (ret != "")
        return ret;

    return "";
}


/**
 * Main entry point to the application.
 * This is a simple console utility that allows to
 * dump the contents of performance storages locally.
 * @param arguments count
 * @param console arguments
 * @return unix-style return status
 */
int main(int argc, const char *argv[])
{
    vector<string> args;

    printf( "Performance Counters View Utility v%s\n", VER_PRODUCTVERSION_STR );
    printf( "%s\n", VER_COPYRIGHT_STR );
	printf( "\n" );

    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);

    #define HAS_ARG( str ) \
        (std::find( args.begin(), args.end(), string(str) ) != args.end())

    if ( args.empty() )
    {
        help();
    }
    else if ( HAS_ARG("--help") || HAS_ARG("-h") )
    {
        help();
    }
    else if ( HAS_ARG("--all") || HAS_ARG("-a") ||
              HAS_ARG("--counter") || HAS_ARG("-c") ||
              HAS_ARG("--storage") || HAS_ARG("-s") )
    {
        PerfCountersOut counters;

        string storage = GET_ARG(args, "--storage", "-s");
        string counter = GET_ARG(args, "--counter", "-c");

        bool loop = HAS_ARG("--loop") || HAS_ARG("-l");
		bool nozero = HAS_ARG("--nozero") || HAS_ARG("-n");
		bool digits = HAS_ARG("--digits") || HAS_ARG("-d");

        counters.Fill();

        if ( loop )
        {
            string sdelay = GET_ARG(args, "--loop", "-l");
            int delay = atoi( sdelay.c_str() );
            if (delay == 0)
                delay = 1;

            while (1)
            {
				// counters.Fill() destroy and re-initialize all counters, so
				// need to sleep between Fill() and Dump()
				sleep(delay);

                counters.Dump(storage, counter, true, nozero, digits);

				// configuration may change
				counters.Fill();
            }
        }
        else
        {
            counters.Dump(storage, counter, false, nozero, digits);
        }
    }
    else
    {
        help(NULL);
    }

    return 0;
}
