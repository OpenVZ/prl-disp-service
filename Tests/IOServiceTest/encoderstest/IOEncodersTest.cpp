/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */


#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QBuffer>
#include <QImage>
#include <QtTest>

#include "EncoderRegistrator.h"
#include "IOServiceEncodersTestData.xpm"

using namespace IOService;

/*****************************************************************************/

IOPackage::EncodingType encoders [] = {
    IOPackage::RREEncoding,
    IOPackage::HextileEncoding,
    IOPackage::ZRLEEncoding,
};

const size_t encodersSz = sizeof(encoders) / sizeof(encoders[0]);

/*****************************************************************************/

class IOEncodersTest : public QObject
{
    Q_OBJECT

public:
	IOEncodersTest(Encoder *pEncoder,
                       const uchar* imgPtr,
                       quint32 bytesPerLine,
                       quint32 depth,
                       quint32 width,
                       quint32 height )
	: m_pEncoder(pEncoder),
          m_imgPtr(imgPtr),
          m_bytesPerLine(bytesPerLine),
          m_depth(depth),
          m_width(width),
          m_height(height)
    {}

private slots:
    void testEncoder();

private:
    Encoder *m_pEncoder;
    const uchar* m_imgPtr;
    quint32 m_bytesPerLine;
    quint32 m_depth;
    quint32 m_width;
    quint32 m_height;
};

void IOEncodersTest::testEncoder()
{
    Stream localOut( 8 );

    QRect imgRect(0, 0, m_width, m_height);

    // We should use copy buffer only for RRE encoding,
    // because RRE encoding changes source image.
    const uchar* copyImgPtr = m_imgPtr;

    if ( m_pEncoder->encodingType() == IOPackage::RREEncoding ) {
        copyImgPtr = new uchar[ m_bytesPerLine * m_height ];
        ::memcpy( const_cast<uchar*>(copyImgPtr),
                  m_imgPtr, m_bytesPerLine * m_height );
    }

    bool res = m_pEncoder->writeRect( imgRect, copyImgPtr,
                                      m_bytesPerLine, m_depth, localOut);

    if ( m_pEncoder->encodingType() == IOPackage::RREEncoding )
        delete copyImgPtr;

    QCOMPARE( res, true );

    // Seek to the beginning
    localOut.seek(0);

    uchar* imgPtr = new uchar[ m_bytesPerLine * m_height ];
    ::memset(imgPtr, 0, m_bytesPerLine * m_height);

    res = m_pEncoder->readRect( imgRect, localOut,
                                imgPtr, m_bytesPerLine,
                                m_depth );

    QCOMPARE( res, true );
    QCOMPARE( ::memcmp(m_imgPtr, imgPtr, m_bytesPerLine * m_height), 0 );

    delete imgPtr;
}

/*****************************************************************************/

int main ( int argc, char* argv[] )
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    int nRet = 0;

    QImage img(IOServiceEncodersTestData_xpm);

    const quint32 depths[] = { 8, 15, 16, 24, 32 };
    const quint32 depthsSize = sizeof(depths) / sizeof(depths[0]);

    // take big bytes per line
    const quint32 bytesPerLine = img.width() * 4 * 16;
    const quint32 width = img.width();
    const quint32 height = img.height();

    for ( quint32 i = 0; i < depthsSize; ++i ) {
        const quint32 depth = depths[i];

        qWarning("Depth %d", depth);

        uchar* imgPtr = new uchar[ bytesPerLine * height ];
        ::memset(imgPtr, 0, bytesPerLine * height);
        for ( quint32 y = 0; y < height; ++y ) {
            ::memcpy( imgPtr + y * bytesPerLine,
                      img.bits() + y * width * ((depth + 7)/8),
                      width * ((depth + 7)/8) );
        }

        for ( uint i = 0; i < encodersSz; ++i ) {
            Encoder* enc = EncoderRegistrator::instance().display_findEncoder(encoders[i]);
            IOEncodersTest encoder_test( enc,
                                         imgPtr,
                                         bytesPerLine,
                                         depth,
                                         width,
                                         height );
            if (QTest::qExec(&encoder_test))
                nRet = -1;
        }

        delete imgPtr;
    }
    return nRet;
}

/*****************************************************************************/
#include "IOEncodersTest.moc"
