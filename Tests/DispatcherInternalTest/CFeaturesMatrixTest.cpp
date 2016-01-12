///////////////////////////////////////////////////////////////////////////////
///
/// @file CFeaturesMatrixTest.cpp
///
/// Tests suite for features matrix Matrix class
///
/// @author sandro
/// @owner sergeym
///
/// Copyright (c) 1999-2015 Parallels IP Holdings GmbH
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

#include "CFeaturesMatrixTest.h"
#include <prlcommon/PrlCommonUtilsBase/CFeaturesMatrix.h>
#include <prlxmlmodel/Messaging/CVmBinaryEventParameter.h>

void CFeaturesMatrixTest::testInitialize()
{
	CFeaturesMatrix _matrix;
	_matrix.Initialize( QSet<PRL_FEATURES_MATRIX>()<<PFSM_SATA_HOTPLUG_SUPPORT );
	QVERIFY(_matrix.IsFeatureSupported( PFSM_SATA_HOTPLUG_SUPPORT ) == PRL_TRUE);
	QVERIFY(_matrix.IsFeatureSupported( PFSM_UNKNOWN_FEATURE ) == PRL_FALSE);
	QVERIFY(_matrix.IsFeatureSupported( (PRL_FEATURES_MATRIX)USHRT_MAX ) == PRL_FALSE);
}

void CFeaturesMatrixTest::testSerializeDeserializeEmptyMatrix()
{
	CFeaturesMatrix _matrix1, _matrix2;
	_matrix2.Initialize( QSet<PRL_FEATURES_MATRIX>()<<PFSM_SATA_HOTPLUG_SUPPORT );
	QVERIFY(_matrix2.IsFeatureSupported( PFSM_SATA_HOTPLUG_SUPPORT ) == PRL_TRUE);
	SmartPtr<CVmBinaryEventParameter> pEventParam( new CVmBinaryEventParameter( EVT_PARAM_PRL_FEATURES_MATRIX ) );
	_matrix1.Serialize(*pEventParam->getBinaryDataStream().getImpl());
	_matrix2.Deserialize(*pEventParam->getBinaryDataStream().getImpl());
	QVERIFY(_matrix2.IsFeatureSupported( PFSM_SATA_HOTPLUG_SUPPORT ) == PRL_FALSE);
}

void CFeaturesMatrixTest::testSerializeDeserializeInitializedMatrix()
{
	CFeaturesMatrix _matrix1, _matrix2;
	_matrix1.Initialize( QSet<PRL_FEATURES_MATRIX>()<<PFSM_SATA_HOTPLUG_SUPPORT );
	QVERIFY(_matrix2.IsFeatureSupported( PFSM_SATA_HOTPLUG_SUPPORT ) == PRL_FALSE);
	SmartPtr<CVmBinaryEventParameter> pEventParam( new CVmBinaryEventParameter( EVT_PARAM_PRL_FEATURES_MATRIX ) );
	_matrix1.Serialize(*pEventParam->getBinaryDataStream().getImpl());
	_matrix2.Deserialize(*pEventParam->getBinaryDataStream().getImpl());
	QVERIFY(_matrix2.IsFeatureSupported( PFSM_SATA_HOTPLUG_SUPPORT ) == PRL_TRUE);
}

