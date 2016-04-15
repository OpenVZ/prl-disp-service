///////////////////////////////////////////////////////////////////////////////
///
/// @file Ploop.h
///
/// Implementation for imagetool lib interface:
///  - ploop attach/detach logic
///
/// @author wolf
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
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

#ifndef __LOGIC_PLOOP_H__
#define __LOGIC_PLOOP_H__

#include <QString>
#include <QList>

#define PLOOPLIB "/usr/lib64/libploop.so"
#define PARTED "/sbin/parted"

/**
* This struct contain info about ploop.
*/
class Ploop
{
	public:
		Ploop(std::string szDiskPath);
		~Ploop();

		PRL_RESULT Init();
		PRL_RESULT Attach(bool bReadOnly);
		PRL_RESULT Detach();
		bool isAttached() { return (m_di != NULL); };
		std::string GetDevice() { return m_szPloopDeviceName; };

	private:
		std::string m_szPloopDiskPath;
		std::string m_szPloopDeviceName;
		struct ploop_disk_images_data *m_di;

	private:
		bool isLoaded();

};

namespace PloopImage
{

struct ImageData
{
	ImageData(const QString& guid_, const QString& file_)
		: guid(guid_), file(file_)
	{
	}
	QString guid;
	QString file;
};

struct SnapshotData
{
	SnapshotData(const QString& guid_, const QString& parent_guid_)
		: guid(guid_), parent_guid(parent_guid_)
	{
	}
	QString guid;
	QString parent_guid;
};

struct DiskDescriptor
{
	unsigned long long size;
	unsigned int heads;
	unsigned int cylinders;
	unsigned int sectors;
	unsigned int blocksize;
	int mode;
	QString top_guid;
	QList<ImageData> images;
	QList<SnapshotData> snapshots;
};

struct Image
{
	/**
	 * Constructor with params
	 * @param path - full path to a folder, which (will) contain DiskDescriptor.xml
	 */
	explicit Image(const QString& path);
	PRL_RESULT createDiskDescriptor(const QString& imagePath, unsigned long long size) const;
	PRL_RESULT createSnapshot(const QString& path = QString()) const;
	PRL_RESULT setActiveDeltaLimit(unsigned long long lim) const;
	PRL_RESULT getBaseDeltaFilename(QString& path) const;
	PRL_RESULT mount(QString& dev) const;
	PRL_RESULT umount() const;
	PRL_RESULT getMountedDevice(QString& dev) const;

private:
	void trace(const char *call, int res) const;

private:
	/** Full path to DiskDescriptor.xml */
	QString m_path;
};

} // namespace PloopImage

#endif // __LOGIC_LVM_H__
