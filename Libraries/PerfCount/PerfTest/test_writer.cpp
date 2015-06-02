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

/*
cl  /Zi Advapi32.lib test_xxx.cpp
*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>

#define BUF_SIZE 256
const char szName[] = "Global\\MyFileMappingObject" ;
const char szMsg[] = "Message from first process" ;

int writer()
{
   printf("Test writer.\n") ;

   HANDLE hMapFile;
   LPCTSTR pBuf;

   hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,    // use paging file
                 NULL,     // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size
                 BUF_SIZE,                // buffer size
                 szName);                 // name of mapping object

   if (hMapFile == NULL)
   {
      printf("Could not create file mapping object (%d).\n",
             GetLastError());
      return 1;
   }
   pBuf = (LPTSTR) MapViewOfFile(hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,
                        0,
                        BUF_SIZE);

   if (pBuf == NULL)
   {
      printf("Could not map view of file (%d).\n",
             GetLastError());
      return 2;
   }

   CopyMemory((PVOID)pBuf, szMsg, strlen(szMsg));
   printf("press any key to quit.\n") ;
   _getch();

   if (UnmapViewOfFile(pBuf)==0)
   {
      printf("Could not UnmapViewOfFile (%d).\n",
             GetLastError());
      return 1;
   }

   if (CloseHandle(hMapFile)==0)
   {
      printf("Could not CloseHandle (%d).\n",
             GetLastError());
      return 1;
   }


   return 0;
}

int reader()
{
   printf("Test reader.\n") ;

   HANDLE hMapFile;
   LPCTSTR pBuf;

   hMapFile = OpenFileMapping(
                   FILE_MAP_ALL_ACCESS,   // read/write access
                   FALSE,                 // do not inherit the name
                   szName);               // name of mapping object

   if (hMapFile == NULL)
   {
      printf("Could not open file mapping object (%d).\n",
             GetLastError());
      return 1;
   }

   pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
               FILE_MAP_ALL_ACCESS,  // read/write permission
               0,
               0,
               BUF_SIZE);

   if (pBuf == NULL)
   {
      printf("Could not map view of file (%d).\n",
             GetLastError());
      return 2;
   }

   printf("%s: %s", TEXT("Process2"), pBuf);

   UnmapViewOfFile(pBuf);

   CloseHandle(hMapFile);

   return 0;
}

int main(int argc, char **argv)
{
    if (argc<2)
        return reader() ;
    if (strcmp(argv[1], "writer")!=0)
    {
        printf("Usage: test.exe [writer]") ;
        return 1 ;
    }
    return writer() ;
}
