///////////////////////////////////////////////////////////////////////////////
///
/// @file CGuestOsesHelperTest.cpp
///
/// Tests suite for guest OSes helper class
///
/// @author sandro
/// @owner sergeym
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CGuestOsesHelperTest.h"
#include "Libraries/PrlCommonUtilsBase/CGuestOsesHelper.h"

void CGuestOsesHelperTest::testOsesMatrix()
{
	TOpaqueTypeList<PRL_UINT8> _oses_types = CGuestOsesHelper::GetSupportedOsesTypes(PHO_UNKNOWN);

	COsesMatrix _matrix;
	foreach(PRL_UINT8 nOsType, _oses_types.GetContainer())
		_matrix.AddOsType(PHO_UNKNOWN, nOsType);

	QVERIFY(_matrix.GetSupportedOsesTypes().GetContainer() == CGuestOsesHelper::GetSupportedOsesTypes(PHO_UNKNOWN).GetContainer());
	QVERIFY(_matrix.GetSupportedOsesVersions(PVS_GUEST_TYPE_WINDOWS).GetContainer() == CGuestOsesHelper::GetSupportedOsesVersions(PHO_UNKNOWN, PVS_GUEST_TYPE_WINDOWS).GetContainer());
	QCOMPARE(_matrix.GetDefaultOsVersion(PVS_GUEST_TYPE_WINDOWS), CGuestOsesHelper::GetDefaultOsVersion(PVS_GUEST_TYPE_WINDOWS));
}

void CGuestOsesHelperTest::testOperatorEquals()
{
	COsesMatrix _matrix1, _matrix2;

	_matrix1.AddOsType(PHO_UNKNOWN, PVS_GUEST_TYPE_WINDOWS);
	_matrix2.AddOsType(PHO_UNKNOWN, PVS_GUEST_TYPE_WINDOWS);

	_matrix1.AddOsType(PHO_UNKNOWN, PVS_GUEST_TYPE_LINUX);
	_matrix2.AddOsType(PHO_UNKNOWN, PVS_GUEST_TYPE_LINUX);

	_matrix1.AddOsType(PHO_UNKNOWN, PVS_GUEST_TYPE_MACOS);
	_matrix2.AddOsType(PHO_UNKNOWN, PVS_GUEST_TYPE_MACOS);

	QVERIFY(_matrix1 == _matrix2);
}

void CGuestOsesHelperTest::testSerializeDeserialize()
{
	COsesMatrix _matrix1, _matrix2;

	//Prepare initial object
	TOpaqueTypeList<PRL_UINT8> _oses_types = CGuestOsesHelper::GetSupportedOsesTypes(PHO_UNKNOWN);
	foreach(PRL_UINT8 nOsType, _oses_types.GetContainer())
		_matrix1.AddOsType(PHO_UNKNOWN, nOsType);

	//Serialize initial object
	QByteArray _byte_array;
	QBuffer _buffer(&_byte_array);
	QVERIFY(_buffer.open(QIODevice::ReadWrite));
	QDataStream _data_stream(&_buffer);
	_data_stream.setVersion(QDataStream::Qt_4_0);
	_matrix1.Serialize(_data_stream);

	//Deserialize target object
	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now
	_matrix2.Deserialize(_data_stream);

	//Compare result objects
	QVERIFY(_matrix1 == _matrix2);
}

