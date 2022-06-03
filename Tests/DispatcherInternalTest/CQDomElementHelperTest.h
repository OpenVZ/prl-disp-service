/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2022 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///		CQDomElementHelperTest.h
///
/// @author
///		aleksandr.leskin
///
/// @brief
///		Tests fixture class for comparing two QT classes QDomElement
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CQDomElementHelperTest_H
#define CQDomElementHelperTest_H

#include <QDomElement>

class CQDomElementHelperTest
{
public:
	CQDomElementHelperTest(QString xmlFile) : mInfo(xmlFile) {};
	void testElement(const QDomElement &xDoc, const QDomElement &yDoc);
	static QString namesNE(const QString &x, const QString &y);
	static QString listOfAllAttr(const QDomElement &x);
private:
	void testAttr(const QDomElement &x, const QDomElement &y);
	void testChild(const QDomElement &x, const QDomElement &y);
	QDomElement findChild(const QDomElement &xElement, const QDomElement &yDoc);
	bool isAttrSame(const QDomElement &x, const QDomElement &y);
	QString mInfo;
};

#endif // CQDomElementHelperTest
