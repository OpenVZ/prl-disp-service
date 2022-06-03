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
///		CQDomElementHelperTest.cpp
///
/// @author
///		aleksandr.leskin
///
/// @brief
///		Tests fixture class for comparing two QT classes QDomElement
///
/////////////////////////////////////////////////////////////////////////////
#include "CQDomElementHelperTest.h"
#include <prlcommon/Interfaces/VirtuozzoQt.h>
#include <QtTest/QtTest>

QString CQDomElementHelperTest::namesNE(const QString &x, const QString &y)
{
	return " [ " + x + " != " + y + " ]";
}

QString CQDomElementHelperTest::listOfAllAttr(const QDomElement &x)
{
	QString result;
	QDomNamedNodeMap map = x.attributes();
	for( int i = 0 ; i < map.length() ; ++i )
	{
		if(!(map.item(i).isNull()))
		{
			QDomAttr attr = map.item(i).toAttr();
			result += attr.name() + "=" + attr.value() +"; ";
		}
	}
	return result;
}


bool CQDomElementHelperTest::isAttrSame(const QDomElement &xDoc, const QDomElement &yDoc)
{
	if (xDoc.hasAttributes() != yDoc.hasAttributes())
		return false;
	if (xDoc.attributes().size() != yDoc.attributes().size())
		return false;

	QDomNamedNodeMap xMap	= xDoc.attributes();
	QDomNamedNodeMap yMap	= yDoc.attributes();
	for( int i = 0 ; i < xMap.length() ; ++i )
	{
		if(!(xMap.item(i).isNull()))
		{
			QDomAttr xAttr = xMap.item(i).toAttr();
			if(!xAttr.isNull())
			{
				if (!yMap.contains(xAttr.name()))
					return false;
				QDomAttr yAttr = yMap.namedItem(xAttr.name()).toAttr();
				if (xAttr.value() != yAttr.value())
					return false;
			}
		}
	}
	return true;
}

void CQDomElementHelperTest::testAttr(const QDomElement &xDoc, const QDomElement &yDoc)
{
	QVERIFY2(xDoc.hasAttributes() == yDoc.hasAttributes(), QSTR2UTF8(mInfo));
	QVERIFY2(xDoc.attributes().size() == yDoc.attributes().size(), QSTR2UTF8(mInfo));

	QDomNamedNodeMap xMap 	= xDoc.attributes();
	QDomNamedNodeMap yMap	= yDoc.attributes();
	for( int i = 0 ; i < xMap.length() ; ++i )
	{
		if(!(xMap.item(i).isNull()))
		{
			QDomAttr xAttr = xMap.item(i).toAttr();
			if(!xAttr.isNull())
			{
				QVERIFY2(yMap.contains(xAttr.name()), QSTR2UTF8(mInfo + "  " + xAttr.name() + " Attribute is absent"));
				QDomAttr yAttr = yMap.namedItem(xAttr.name()).toAttr();
				QVERIFY2(xAttr.value() == yAttr.value(), QSTR2UTF8(mInfo + "  " + xAttr.name() + "->" +namesNE(xAttr.value(), yAttr.value())));
			}
		}
	}
}

void CQDomElementHelperTest::testChild(const QDomElement &xDoc, const QDomElement &yDoc)
{
	QVERIFY2(xDoc.childNodes().size() == yDoc.childNodes().size(), QSTR2UTF8(mInfo));

	QDomNode xNode = xDoc.firstChild();
	while(!xNode.isNull())
	{
		QDomElement xElement = xNode.toElement();
		if(!xElement.isNull())
		{
			QDomElement yElement = yDoc.firstChildElement(xElement.tagName());
			while (!yElement.isNull() && !isAttrSame(xElement, yElement))
				yElement = yElement.nextSiblingElement(xElement.tagName());

			QVERIFY2(!yElement.isNull(), QSTR2UTF8(mInfo + "->" + xElement.tagName() + " Child is absent with attributes: [" + listOfAllAttr(xElement) + "]"));
			CQDomElementHelperTest tester(mInfo);
			tester.testElement(xElement, yElement);
		}
		xNode = xNode.nextSibling();
	}
}

void CQDomElementHelperTest::testElement(const QDomElement &xDoc, const QDomElement &yDoc)
{
	QVERIFY2(xDoc.tagName() == yDoc.tagName(), QSTR2UTF8(mInfo + namesNE(xDoc.tagName(), yDoc.tagName())));
	mInfo += "->" + xDoc.tagName();
	testAttr(xDoc, yDoc);
	testChild(xDoc, yDoc);
}
