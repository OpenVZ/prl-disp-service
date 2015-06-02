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


#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <QFileInfo>
#include <Carbon/Carbon.h>
#include <QDir>
#include <QFile>

typedef void (TestFuncType)();
bool forktest( TestFuncType func);;

class Service
{
public:
   static FILE* helloWorld_init();
   static void testCarbon();
};


bool forktest();

int main( int argc, char** argv )
{

          // use fork & Test Carbon
	return forktest( &Service::testCarbon)?0:1;

}

void Service::testCarbon()
{
  FILE* f = Service::helloWorld_init();

  char  pCh[] = "/usr/local";

   QString path = "/usr/local";
   QFile   file( path );

   FSRef fref;

   if (FSPathMakeRef( (const UInt8 *) pCh,
                                      &fref, NULL) == noErr)
      fprintf ( f, "FSPathMakeRef11 success");
   else
      fprintf ( f, "FSPathMakeRef11 FAILED");

   if (FSPathMakeRef( (const UInt8 *) path.toUtf8().constData(),
                                      &fref, NULL) == noErr)
      fprintf ( f, "FSPathMakeRef1 success");
   else
      fprintf ( f, "FSPathMakeRef1 FAILED");

   if (FSPathMakeRef((const UInt8 *)QFile::encodeName(QDir::cleanPath( path ) ).data(),
                                &fref, NULL) == noErr)
      fprintf ( f, "FSPathMakeRef success");
   else
      fprintf ( f, "FSPathMakeRef FAILED");

}

bool forktest( TestFuncType func)
{

	pid_t t = ::fork();
	if (t < 0) {
		return false;
	}
	if (t != 0) {
		::_exit(0);
	}

	// Create session & set pgid
	::setsid();

	// Make sure that the child process cannot
	// acquire a controlling terminal
	::signal(SIGHUP, SIG_IGN);
	t = ::fork();
	if (t < 0) {
		return false;
	}
	if (t != 0) {
		::_exit(0);
	}

	// Close stdin, stdout and stderr
	for (int i = 0; i <= 2; ++i)
		::close(i);

	// Reopen stdin, stdout and stderr
	int fdstdin = ::open("/dev/null", O_RDWR);
	::dup(fdstdin);
	::dup(fdstdin);

	// Restrict permissions on files that are created by the service
	::umask(027);

	// Make a copy of the arguments, and make sure that argv[0]
	// contains an absolute path

	// Change the directory to /
	::chdir("/");


	//sleep to attach
	//printf (" ===== before sleep");
	sleep ( 20 );
	//printf (" ===== after sleep");


	func();

	return true;
}


FILE* Service::helloWorld_init()
{
FILE* f = NULL;

	char* fname = "/tmp/QtServiceTest.log";
	f = fopen (fname, "a+");
	Q_ASSERT ( f );

	fprintf ( f, "\n=====================\n" );
	fflush( f );
return f;
}
