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

#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <sddl.h>
#include <TCHAR.H>

#define DEF_BUF_SIZE 1024

#define BUF_SIZE 256

//const char szGlobalName[] = "Global\\MyFileMappingObject" ;
const char szGlobalName[] = "Session\\0\\MyFileMappingObject" ;
const char szFileName[] = "shm_test_FileMappingObject" ;
const char szMsg[]= "Message from first process started by user: " ;


#define FULL_RIGHTS(for) TEXT( "(A;OICI;GARCSDFA;;;" #for ")" )

BOOL CreateMyDACL(SECURITY_ATTRIBUTES * pSA)
{
     pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
     pSA->bInheritHandle = FALSE;

     // Define the SDDL for the DACL. This example sets
     // the following access:
     //     Built-in guests are denied all access.
     //     Anonymous logon is denied all access.
     //     Authenticated users are allowed
     //     read/write/execute access.
     //     Administrators are allowed full control.
     // Modify these values as needed to generate the proper
     // DACL for your application.
     TCHAR * szSD = TEXT("D:")       // Discretionary ACL
        FULL_RIGHTS(BG)     // Deny access to
                                     // built-in guests
        FULL_RIGHTS(AN)     // Deny access to
                                     // anonymous logon
        FULL_RIGHTS(AU)     // Allow
                                     // read/write/execute
                                     // to authenticated
                                     // users
        FULL_RIGHTS(BA);    // Allow full control
                                     // to administrators
    if (NULL == pSA)
        return FALSE;

     return ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSD,
                SDDL_REVISION_1,
                &(pSA->lpSecurityDescriptor),
                NULL);
//    return TRUE ;
}

const char* get_user_name(char *buff)
{
   DWORD not_used = DEF_BUF_SIZE ;
   if (!GetUserName(buff, &not_used))
        sprintf(buff, "could not get  user name, error %d", GetLastError()) ;
   return buff ;
}


int mem_writer(HANDLE hMapFile)
{
   LPCTSTR pBuf;

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

   char msg_buff[DEF_BUF_SIZE] ;
   strcpy(msg_buff, szMsg) ;
   get_user_name(msg_buff + strlen(szMsg) - 1) ;

   CopyMemory((PVOID)pBuf, msg_buff, strlen(msg_buff));
   printf("mem content: '%s'.\n", msg_buff) ;
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


const char* get_shared_file_name(char *buff)
{
    //GetTempPath(DEF_BUF_SIZE, buff) ;
    strcpy(buff, "C:\\temp\\") ;
    strcpy(buff + strlen(buff), szFileName) ;
    return buff ;
}

int file_shared_writer()
{
   char file_name[DEF_BUF_SIZE] ;
   get_shared_file_name(file_name) ;

   printf("Test file shared writer '%s'.\n", file_name) ;


   HANDLE hMapFile;
   if (!DeleteFile(file_name))
   {
       printf("Deleting file '%s' error: %d\n", file_name, GetLastError()) ;
       if (GetLastError() != ERROR_FILE_NOT_FOUND)
           return -1 ;
   }

    SECURITY_ATTRIBUTES  sa;

    if (!CreateMyDACL(&sa))
    {
         // Error encountered; generate message and exit.
         printf("Failed CreateMyDACL, errno: %d\n", GetLastError());
         return -1 ;
    }

   HANDLE hSharedFile = CreateFileA(file_name,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    &sa,
                                    CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL) ;

   if (hSharedFile == INVALID_HANDLE_VALUE)
   {
      printf("Could not create file '%s' for mapping , error: %d.\n", szFileName, GetLastError());
      return 1 ;
   }
   hMapFile = CreateFileMappingA(hSharedFile,             // file to be mapped
                 &sa,     // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size
                 BUF_SIZE,                // buffer size
                 NULL);                 // name of mapping object

   if (hMapFile == NULL)
   {
      printf("Could not create file mapping object (%d).\n",
             GetLastError());
      return 1;
   }
   CloseHandle(hSharedFile) ;

   return mem_writer(hMapFile) ;
}

int global_shared_writer()
{
   printf("Test global shared writer.\n") ;

   HANDLE hMapFile;

    SECURITY_ATTRIBUTES  sa;

    if (!CreateMyDACL(&sa))
    {
         // Error encountered; generate message and exit.
         printf("Failed CreateMyDACL, errno: %d\n", GetLastError());
         return -1 ;
    }

   hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,    // use paging file
                 &sa,    // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size
                 BUF_SIZE,                // buffer size
                 szGlobalName) ;                 // name of mapping object

   if (hMapFile == NULL)
   {
      printf("Could not create file mapping object (%d).\n",
             GetLastError());
      return 1;
   }

   return mem_writer(hMapFile) ;
}

int read_mem( HANDLE hMapFile, const char * szName)
{
   if (hMapFile == NULL)
   {
      printf("Could not open file mapping object '%s', error: %d.\n",
             szName, GetLastError());
      return 1;
   }

   LPCTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
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

int file_reader()
{
   char file_name[DEF_BUF_SIZE] ;
   char buff[DEF_BUF_SIZE] ;

   printf("Test file shared mem '%s' reader by user: %s.\n",
                get_shared_file_name(file_name), get_user_name(buff) ) ;

    HANDLE hSharedFile = CreateFileA(file_name,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, //&sa,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL) ;


    printf("File '%s' opening result: 0x%x, error: %d\n",
           file_name, hSharedFile, GetLastError()) ;

    if (hSharedFile == INVALID_HANDLE_VALUE)
        return -1 ;

   int res = read_mem(CreateFileMappingA(hSharedFile,             // file to be mapped
                 NULL, //&sa,     // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size
                 BUF_SIZE,                // buffer size
                 NULL), file_name);                 // name of mapping object

   CloseHandle(hSharedFile) ;

   return res ;
}

int global_reader()
{
   char msg_buff[DEF_BUF_SIZE] ;

   printf("Test global shared mem reader by user: %s.\n", get_user_name(msg_buff) ) ;

   const char * szName = szGlobalName ;
   HANDLE hMapFile;
   LPCTSTR pBuf;

   return read_mem(OpenFileMappingA(
                           FILE_MAP_ALL_ACCESS,   // read/write access
                           FALSE,                 // do not inherit the name
                           szName), szName);               // name of mapping object
}


int main(int argc, char **argv)
{
    if (argc>1) {
      if (strcmp(argv[1], "writer")==0)
        return global_shared_writer() ;

      if (strcmp(argv[1], "file_writer")==0)
        return file_shared_writer() ;
    }
    if (argc<2 || strstr(argv[1], "reader"))
        if (argc>1 && strstr(argv[1], "file"))
            return file_reader() ;
        else
            return global_reader() ;


    printf("Usage: test.exe [reader|file_reader|writer|file_writer]") ;
    return 1 ;
}
