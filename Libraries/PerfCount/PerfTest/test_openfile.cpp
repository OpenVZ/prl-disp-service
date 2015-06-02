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
        TEXT("(A;OICI;GA;;;BG)")     // Deny access to
                                     // built-in guests
        TEXT("(A;OICI;GA;;;AN)")     // Deny access to
                                     // anonymous logon
        TEXT("(A;OICI;GA;;;AU)")     // Allow
                                     // read/write/execute
                                     // to authenticated
                                     // users
        TEXT("(A;OICI;GA;;;BA)");    // Allow full control
                                     // to administrators

    if (NULL == pSA)
        return FALSE;

     return ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSD,
                SDDL_REVISION_1,
                &(pSA->lpSecurityDescriptor),
                NULL);
}

/*
HANDLE WINAPI CreateFile(
  __in      LPCTSTR lpFileName,
  __in      DWORD dwDesiredAccess,
  __in      DWORD dwShareMode,
  __in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  __in      DWORD dwCreationDisposition,
  __in      DWORD dwFlagsAndAttributes,
  __in_opt  HANDLE hTemplateFile
);
*/

#define FILE_OPER_TXT(flag) (cr_flags == CREATE_NEW ? "creating" : "opening")

int open_file(const char *name, const char *code)
{
    DWORD cr_flags = strchr(code, 'O') ? OPEN_EXISTING : 0 ;
    if (strchr(code, 'C'))
        cr_flags = CREATE_NEW ;


    printf("Do operations: '%s' for file '%s'.\n",
                code, name) ;

    if (strchr(code, 'D') && !DeleteFile(name))
    {
        printf("Deleting file '%s' error: %d\n", name, GetLastError()) ;
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
            return -1 ;
    }

    if (!cr_flags)
        return 0 ;

    SECURITY_ATTRIBUTES  sa;

    if (!CreateMyDACL(&sa))
    {
         // Error encountered; generate message and exit.
         printf("Failed CreateMyDACL, errno: %d\n", GetLastError());
         return -1 ;
    }

    SECURITY_ATTRIBUTES * sa_ptr = NULL ;
    if (strchr(code, 'S'))
        sa_ptr = &sa ;


    HANDLE hSharedFile = CreateFileA(name,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              sa_ptr,
                              cr_flags,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL) ;


    printf("File '%s' %s result: 0x%x, error: %d\n",
           name, FILE_OPER_TXT(cr_flag), hSharedFile, GetLastError()) ;

    if (hSharedFile == INVALID_HANDLE_VALUE)
        return -1 ;

    char data[1024] = "jdhsajkfdskdfjlasdkfhdskfhiuh" ;
    DWORD writen ;
    if (cr_flags == CREATE_NEW)
        WriteFile(hSharedFile, data, sizeof(data), &writen, NULL);
    else {
        ReadFile(hSharedFile, data, sizeof(data), &writen, NULL);
        printf("Readed data: %s", data) ;
    }



    CloseHandle(hSharedFile) ;

    if (strchr(code, 'P')) {
        printf("Press any key ...\n") ;
        _getch() ;
    }
    return 0 ;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: test.exe [<opcode>=]<file name>\n"\
                "opcodes:\tD : delete (before other call)\n\tC : create\n\t"\
                "O : open\n\tS : use secure\n\tP: pause (wait a key)\n") ;
        return 1 ;
    }
    for (int i=1; i<argc; ++i)
    {
        const char *name = argv[i] ;
        char code[10] = "" ;
        if (const char *p = strchr(name, '='))
        {
           strncpy(code, name, p-name) ;
           code[p - name] = 0 ;
           name = p+1 ;
        }
        open_file(name, code) ;
    }

    return 0 ;
}
