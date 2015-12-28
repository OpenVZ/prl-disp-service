///////////////////////////////////////////////////////////////////////////////
///
/// @file UserFolder.cpp
///
/// The folders enumeration for user directory
///
/// @author vasilyz@
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
#include <QMap>
#include <QDir>
#include <QMutex>

#ifdef _WIN_
	#include <windows.h>
	#include <shlobj.h>
#elif defined _MAC_
	#include <sys/param.h>
	#include <CoreServices/CoreServices.h>
#elif defined _LIN_
#endif

#include <Interfaces/ParallelsQt.h>
#include <prlcommon/Logging/Logging.h>
#include <Libraries/PrlCommonUtils/UserFolder.h>

using namespace UserFolder;

#ifdef _WIN_
// The knownfolders.h Windows SDK header is available since Windows Vista, thus we define
// required constants ourselves

// {374DE290-123F-4565-9164-39C4925E467B}
const GUID FOLDERID_Downloads = {0x374de290, 0x123f, 0x4565, {0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b}};

// We also need SHGetKnownFolderPath function
struct Shell32 {
	Shell32() {
		m_hLib = LoadLibrary(L"shell32.dll");
		*(FARPROC *)&SHGetKnownFolderPath = GetProcAddress(m_hLib, "SHGetKnownFolderPath");
	}
	~Shell32() {
		FreeLibrary(m_hLib);
	}
	HMODULE m_hLib;
	HRESULT (__stdcall *SHGetKnownFolderPath)(const GUID *, DWORD, HANDLE, PWSTR *);
};
#endif

typedef QMap<Type, QString> Folders;
// Cached folders locations to speed up search
static Folders s_Folders;
// And its honour guard
static QMutex s_Guard;

#ifdef _MAC_
/////////////////////////////////////////////////////////////////////////////
static bool getSpecialFolder(OSType folderType, QString &folderPath)
{
	FSRef ref;
	OSErr err = FSFindFolder(
			kUserDomain,
			folderType,
			kCreateFolder,
			&ref);

	if (err)
	{
		WRITE_TRACE(DBG_WARNING, "FSFindFolder() failed, OSErr = %d", (int) err);
		return false;
	}

	UInt8 path[MAXPATHLEN];

	OSStatus os = FSRefMakePath(&ref, path, sizeof(path));

	if (os)
	{
		WRITE_TRACE(DBG_WARNING, "FSRefMakePath() failed, OSStatus = %d", (int) os);
		return false;
	}

	folderPath = UTF8_2QSTR((char *)path);

	return true;
}

/**
 * Get user shell path for Mac host
 */
static bool getMacSpecialFolder(Type nFolderId, QString &folderPath)
{
	OSType folderType;

	switch(nFolderId)
	{
		case DESKTOP:
			folderType = kDesktopFolderType;
			break;
		case MYDOCUMENTS:
			folderType = kDocumentsFolderType;
			break;
		case MYPICTURES:
			folderType = kPictureDocumentsFolderType;
			break;
		case MYMUSIC:
			folderType = kMusicDocumentsFolderType;
			break;
		case MYVIDEOS:
			folderType = kMovieDocumentsFolderType;
			break;
		case DOWNLOADS:
			folderType = kDownloadsFolderType;
			break;
		default:
			WRITE_TRACE(DBG_WARNING, "Invalid folder ID %u", (int)nFolderId);
			return false;
	}

	if (!getSpecialFolder(folderType, folderPath))
		return false;

	QString profileFolder;
	bool bRet = getSpecialFolder(kCurrentUserFolderType, profileFolder);
	bRet = bRet && folderPath.startsWith(profileFolder);

	if (bRet)
		folderPath.remove(0, profileFolder.length());

	return bRet;
}
#endif

#ifdef _WIN_
/*
 * Downloads folder get in special way
 */
static QString GetDownloadsPath()
{
	static Shell32 shell32;
	QString Out;

	if (!shell32.SHGetKnownFolderPath)
		return Out;

	PWSTR pstr;

	HRESULT hr = shell32.SHGetKnownFolderPath(&FOLDERID_Downloads, 0, 0, &pstr);

	if (FAILED(hr))
	{
		WRITE_TRACE(DBG_WARNING, "failed to obtain a path for the Downloads (0x%x)", hr);
		return Out;
	}

	Out = UTF16_2QSTR(pstr);
	CoTaskMemFree(pstr);
	return Out;
}

/*
 * Get folder path specified by ID
 */
static QString GetFolderPath(int Type)
{
	WCHAR lpszBuffer[MAX_PATH + 1];

	if (!SHGetSpecialFolderPathW(0, lpszBuffer, Type, TRUE))
	{
		WRITE_TRACE(DBG_WARNING, "failed to obtain a path for the folder %u, err = %u",
			Type, GetLastError());
		return QString();
	}

	return UTF16_2QSTR(lpszBuffer);
}

/**
 * Get user shell path for Windows host
 */
static bool getWinSpecialFolder(Type nFolderId, QString &folderPath)
{
	switch (nFolderId)
	{
		case DESKTOP:
			folderPath = GetFolderPath(CSIDL_DESKTOPDIRECTORY);
			break;
		case MYDOCUMENTS:
			folderPath = GetFolderPath(CSIDL_PERSONAL);
			break;
		case MYPICTURES:
			folderPath = GetFolderPath(CSIDL_MYPICTURES);
			break;
		case MYMUSIC:
			folderPath = GetFolderPath(CSIDL_MYMUSIC);
			break;
		case MYVIDEOS:
			folderPath = GetFolderPath(CSIDL_MYVIDEO);
			break;
		case DOWNLOADS:
			folderPath = GetDownloadsPath();
			break;
		default:
			WRITE_TRACE(DBG_WARNING, "wrong folder id: %u", nFolderId);
			return false;
	}

	if (folderPath.isEmpty())
		return false;

	LOG_MESSAGE(DBG_DEBUG, "path for the folder %d is \"%s\"", nFolderId, QSTR2UTF8(folderPath));

	QString profileFolder = QDir::toNativeSeparators(QDir::homePath());

	if (!folderPath.startsWith(profileFolder))
	{
		WRITE_TRACE(DBG_WARNING, "the folder \"%s\" is not a subfolder of the Home \"%s\"",
			QSTR2UTF8(folderPath), QSTR2UTF8(profileFolder));
		return false;
	}

	folderPath.remove(0, profileFolder.length());
	LOG_MESSAGE(DBG_DEBUG, "resulting path for the folder %d is \"%s\"", nFolderId, QSTR2UTF8(folderPath));
	return true;
}
#endif

#ifdef _LIN_
/**
 * Get user shell path for Linux host
 */
static bool getLinSpecialFolder(Type nFolderId, QString &folderPath)
{
	static const struct
	{
		Type Id;
		QString Name;
	} Folders[] =
		{
			// Shell folders are named as on Fedora linux
			{ DESKTOP, "Desktop" },
			{ MYDOCUMENTS, "Documents" },
			{ MYPICTURES, "Pictures" },
			{ MYMUSIC, "Music" },
			{ MYVIDEOS, "Videos" },
			{ DOWNLOADS, "Download" }
		};
	static const unsigned int Size = sizeof(Folders) / sizeof(Folders[0]);

	unsigned int i;

	for (i = 0; i < Size; i++)
	{
		if (Folders[i].Id == nFolderId)
			break;
	}

	if (i == Size)
	{
		WRITE_TRACE(DBG_WARNING, "wrong folder id: %u", nFolderId);
		return false;
	}

	QDir homeDir = QDir::home();
	if (homeDir == QDir::root())
	{
		WRITE_TRACE(DBG_WARNING, "Home path could not be retrieved");
		return false;
	}

	if (!homeDir.exists(Folders[i].Name) &&
		!homeDir.mkdir(Folders[i].Name))
	{
		WRITE_TRACE(DBG_WARNING, "failed to create the \"%s\" subdirectory in the Home directory (\"%s\")",
				QSTR2UTF8(Folders[i].Name),
				QSTR2UTF8(homeDir.absolutePath()));
		return false;
	}

	folderPath = '/' + Folders[i].Name;

	LOG_MESSAGE(DBG_DEBUG, "resulting path for the folder %d is \"%s\"",
				nFolderId, QSTR2UTF8(folderPath));
	return true;
}
#endif

/**
 * Obtains Users Shell Folder path
 * @param nFolderId[in] - protocol-defined shell folder Id
 * @param folderPath[out] - obtained path
 * @return true on success
 *
 * @note If bHome is true, then folderPath will be relative to user's home dir. If
 *		shell folder is not a subfolder of user's home dir, the function will fail.
 *		If bHome is false, folderPath will be absolute path of requested shell folder.
 *		If function fails, folderPath is undefined.
 */
static inline bool getSpecialFolder(Type Type, QString &Out)
{
#ifdef _MAC_
	return getMacSpecialFolder(Type, Out);
#elif defined _WIN_
	return getWinSpecialFolder(Type, Out);
#elif defined _LIN_
	return getLinSpecialFolder(Type, Out);
#else
	#error Unknown platform: you must define one of (_WIN_, _LIN_, _MAC_)
#endif
}

/*
 * Externally available function to get folder
 */
bool UserFolder::getFolder(UserFolder::Type Type, QString& Out, bool CacheOff)
{
	s_Guard.lock();
	Folders::iterator it = s_Folders.find(Type);

	if (!CacheOff && (it != s_Folders.end()))
	{
		Out = it.value();

		LOG_MESSAGE(DBG_INFO, "folder %d (\"%s\") has been read from the cache", (int)Type,
				QSTR2UTF8(Out));

		// The cached folder may be empty.
		goto Exit;
	}

	// The folder was not cached or must be updated. Get it and cache it.
	if (getSpecialFolder(Type, Out))
		s_Folders[Type] = Out;
	else
		Out.clear();

Exit:
	s_Guard.unlock();
	return !Out.isEmpty();
}
