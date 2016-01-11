///////////////////////////////////////////////////////////////////////////////
///
/// @file Precomp.h
///
/// @author sdmitry
///
/// Windows/Mac OS X precompiled header.
/// Please contact the authors before adding something to this file.
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
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
//////////////////////////////////////////////////////////////////////////
#ifndef Precomp_h__
#define Precomp_h__

#if defined(_WIN_)
#include <windows.h>
#endif

#include <QtCore/QtCore>
#include <QtXml/QtXml>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>

#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlsdk/PrlEnums.h>
#include <prlsdk/PrlErrorsValues.h>
#include <SDK/Wrappers/SdkWrap/SdkHandleWrap.h>

#endif //Precomp_h__
