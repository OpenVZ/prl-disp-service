///////////////////////////////////////////////////////////////////////////////
///
/// @file MetaObjectUtils.cpp
///
/// Helpers for working with Qt's meta-object stuff.
///
/// @author juram
/// @owner sergeym
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

#include "MetaObjectUtils.h"

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QBitArray>
#include <QtCore/QStringList>
#include <QtCore/QMetaObject>

#include <prlcommon/Logging/Logging.h>
#include "Interfaces/Qt4Compatibility.h"

QString MetaObjectUtils::methodNameFromSignature ( const QString& signature )
{
	QString signaturePatched = signature;
	// Remove additional index added by Qt
	if ( signaturePatched.startsWith( '1' ) || signaturePatched.startsWith( '2' ) )
		signaturePatched.remove( 0, 1 );

	return signaturePatched.left( signaturePatched.indexOf( '(' ) );
}

QString MetaObjectUtils::methodSignatureFromName ( const QObject* obj, const QString& methodName )
{
	if ( ! obj )
		return QString();

	QMetaMethod method = getMethodByName( obj, methodName );
	return method.methodIndex() == -1 ? QString() : methodSignature( method );
}

QMetaMethod MetaObjectUtils::getMethodByName ( const QObject* obj, const QString& methodName )
{
	if ( ! obj )
		return QMetaMethod();

	const int methodOffset = 2; // QObject:deleteLater() + QObject::destroyed()
	for( int i = methodOffset; i < obj->metaObject()->methodCount(); ++i )
	{
		QMetaMethod method = obj->metaObject()->method( i );

		QString name = methodNameFromSignature( methodSignature( method ) );
		if ( name == methodName )
			return method;
	}

	WRITE_TRACE( DBG_TRACE, "No method %s was found for object %s",
				methodName.toLatin1().constData(), obj->metaObject()->className() );

	return QMetaMethod();
}

QMetaMethod MetaObjectUtils::getMethodBySignature ( const QObject* obj, const QString& signature )
{
	if ( ! obj )
		return QMetaMethod();

	QString methodName = methodNameFromSignature( signature );
	if ( methodName.isEmpty() )
	{
		WRITE_TRACE( DBG_TRACE, "No method %s was found for object %s",
					signature.toLatin1().constData(), obj->metaObject()->className() );
		return QMetaMethod();
	}

	return getMethodByName( obj, methodName );
}

bool MetaObjectUtils::containsMethod ( const QObject* obj, const QString& methodName )
{
	if ( ! obj )
		return false;

	return obj->metaObject()->indexOfMethod( methodSignatureFromName( obj, methodName ).toLatin1().constData() ) != -1;
}

bool MetaObjectUtils::convertStringPropertyToBitArray ( QObject* o, const QMetaEnum& metaEnum, const QString& propName )
{
	if ( !o || ! metaEnum.isValid() )
		return false;

	QVariant flagVar = o->property( propName.toLatin1().constData() );
	if ( ! flagVar.isValid() )
		return false;

	QStringList flagList = flagVar.toString().split( ',' );
	if ( flagList.isEmpty() )
		return false;

	QBitArray flags( metaEnum.keyCount() - 1 );

	foreach ( QString flag, flagList )
	{
		if ( flag.isEmpty() )
			continue;

		int flagValue = metaEnum.keyToValue( flag.trimmed().toLatin1().constData() );
		if ( flagValue == -1 )
		{
			WRITE_TRACE( DBG_FATAL, "Invalid object availability flag: %s", qPrintable( flag ) );
			Q_ASSERT( false );
			return false;
		}

		flags.setBit( flagValue );
	}

	o->setProperty( propName.toLatin1().constData(), flags );

	return true;
}

QHash<QString, const QMetaObject*>& MetaObjectUtils::staticMetaObjectHash()
{
	static QHash<QString, const QMetaObject*> hash;
	return hash;
}

