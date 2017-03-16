/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

/**
 * CAuth class - test program.
 */

#include <cstdio>
#include <iostream>

#include <QString>
#include <QDataStream>
#include <QIODevice>
#include <QTextStream>

#include <Libraries/CAuth/CAuth.h>

using namespace std;

enum
{
	ACT_EXIT = 0,
    ACT_USER = 1,
	ACT_USER_SSPI = 2,
    ACT_FILE = 3,
};

// Menu output
int menu() {

    QTextStream cons(stdin, QIODevice::ReadWrite);
    int act;

    cout << "Select action:" << endl;
    cout << "[" << ACT_USER			<< "] Check user" << endl;
#ifdef _WIN_
	cout << "[" << ACT_USER_SSPI	<< "] Check user using SSPI" << endl;
#endif
    cout << "[" << ACT_FILE			<< "] Check file" << endl;
    cout << "[" << ACT_EXIT			<< "] Exit" << endl;

    cons >> act;

    return act;
}

// Check user function
static void readUserPass(QString& domain, QString& user, QString& pass)
{
	QTextStream cons(stdin, QIODevice::ReadWrite);

#ifdef _WIN_
	QString domain_user;

	cout << "Enter user name [<domain>\\]<user> : ";
	domain_user = cons.readLine();

	cout << "Enter password: ";
	pass = cons.readLine();

	// Simple
	int iPos = domain_user.indexOf('\\');
	if (-1 == iPos)
	{
		domain.clear();
		user = domain_user;
	}
	else
	{
		domain = domain_user.left(iPos);
		user = domain_user.right(domain_user.length() - iPos - 1);
	}
#else
	domain.clear();

	cout << "Enter user name : ";
	user = cons.readLine();

	cout << "Enter password: ";
	pass = cons.readLine();
#endif
}

// Check user function
static void actUser()
{
    QString domain, user, pass;

	// Read user/pass from stdin
	readUserPass(domain, user, pass);

    CAuth Auth;
    if (Auth.AuthUser(user, pass, domain))
	{
        printf("Auth ok!\n");
    }
	else
	{
        printf("Auth failed!\n");
    }
}

#ifdef _WIN_
// Check user function
static void actUserSSPI()
{
	QString domain, user, pass;

	// Read user/pass from stdin
	readUserPass(domain, user, pass);

	CAuth Auth;
	if (Auth.AuthUserSSPI(user, pass, domain))
	{
		printf("Auth SSPI ok!\n");
	}
	else
	{
		printf("Auth SSPI failed!\n");
	}
}
#endif

// Check file function
static void actFile()
{
    unsigned long mask;
    QString user, file;
    CAuth Auth;
    QTextStream cons(stdin, QIODevice::ReadWrite);

    cout << "Enter username: ";
    user = cons.readLine();

    cout << "Enter filename: ";
	file = cons.readLine();

    if ((mask = Auth.CheckFile(user, file)))
	{
        printf("File: %s (user: %s) perms: [%s][%s][%s]\n",
                file.toAscii().data(), user.toAscii().data(),
                ((mask & CAuth::fileMayRead) ? "R" : " "),
                ((mask & CAuth::fileMayWrite) ? "W" : " "),
                ((mask & CAuth::fileMayExecute) ? "X" : " "));
    }
	else
	{
        printf("Can't check file permissions\n");
    }
}

// Main function
int main()
{
    int act;

    while((act = menu()) != ACT_EXIT)
	{
        switch(act)
		{
            case ACT_USER:
                actUser();
                break;
#ifdef _WIN_
			case ACT_USER_SSPI:
				actUserSSPI();
				break;
#endif
            case ACT_FILE:
                actFile();
                break;
            default:
                printf("Unknown action.\n");
                break;
        }
    }

    return 0;
}
