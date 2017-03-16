///////////////////////////////////////////////////////////////////////////////
///
/// @file MetaObjectUtils.h
///
/// Helpers for working with Qt's meta-object stuff.
///
/// @author juram
/// @owner sergeym
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef META_OBJECT_UTILS_H
#define META_OBJECT_UTILS_H

class QObject;

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QMetaMethod>

namespace MetaObjectUtils
{
	QString methodNameFromSignature ( const QString& );
	QString methodSignatureFromName ( const QObject*, const QString& );
	QMetaMethod getMethodByName ( const QObject*, const QString& );
	QMetaMethod getMethodBySignature ( const QObject*, const QString& );
	bool containsMethod ( const QObject*, const QString& methodName );
	bool convertStringPropertyToBitArray ( QObject*, const QMetaEnum&, const QString& );

	QHash<QString, const QMetaObject*>& staticMetaObjectHash();

	template<typename T> class AddStaticMetaObject
	{
	public:
		AddStaticMetaObject( const char* name )
		{
			addMetaObjectToHash( name, &T::staticMetaObject );
		}

		static void addMetaObjectToHash( const char* name, const QMetaObject* obj )
		{
			staticMetaObjectHash().insert( name, obj );
		}
	};

	template<class T>
	T* newInstance( QGenericArgument val0 = QGenericArgument( 0 ),
					QGenericArgument val1 = QGenericArgument(),
					QGenericArgument val2 = QGenericArgument(),
					QGenericArgument val3 = QGenericArgument(),
					QGenericArgument val4 = QGenericArgument(),
					QGenericArgument val5 = QGenericArgument(),
					QGenericArgument val6 = QGenericArgument(),
					QGenericArgument val7 = QGenericArgument(),
					QGenericArgument val8 = QGenericArgument(),
					QGenericArgument val9 = QGenericArgument() )
	{
		const QMetaObject* metaObject = staticMetaObjectHash().value( T::staticMetaObject.className() );

		if ( metaObject )
			return qobject_cast<T*>( staticMetaObjectHash().value( T::staticMetaObject.className() )->
									 newInstance( val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 ) );
		else
			return 0;
	}
}

#define REGISTER_META_OBJECT( className ) \
	MetaObjectUtils::AddStaticMetaObject<className> addMetaObject_##className( #className );

#define REGISTER_META_OBJECT_WITH_NAME( className, stringName ) \
	MetaObjectUtils::AddStaticMetaObject<className> addMetaObject_##className( #stringName );

#endif // META_OBJECT_UTILS_H
