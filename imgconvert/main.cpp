#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRgb>
#include <QStringList>
#include <QTextStream>

#include <stdint.h>

// From GD.h
#define RGB(r,g,b) (uint16_t)((((r) >> 3) << 10) | (((g) >> 3) << 5) | ((b) >> 3))
#define TRANSPARENT (1 << 15) // transparent for chars and sprites

namespace {

int getPixel(const QImage &img, int x, int y, int oldtransind)
{
    int ret = img.pixelIndex(x, y);

    // fixup transparent pixels (in case the index was != 0)
    if (oldtransind > 0)
    {
        if (ret == oldtransind)
            ret = 0;
        else if (ret == 0)
            ret = oldtransind;
    }

    return ret;
}

void dumpSprite(const QImage &img, QTextStream &tstr, const QString &varbase, int oldtransind)
{
    qDebug() << "dumpSprite:" << varbase;

    tstr << "static const uint8_t " << varbase << "Tex[] PROGMEM = {\n";

    for (int y=0; y<64; ++y)
    {
        // Row data stored as nibbles, low byte == even px, high byte == odd px
        for (int x=0; x<64; x+=2)
        {
            tstr << (getPixel(img, x, y, oldtransind) | (getPixel(img, x+1, y, oldtransind) << 4));
            if (((x+2) != 64) || ((y+1) != 64))
                tstr << ", ";
        }

        tstr << "\n";
    }


#if 0
    for (int y=0; y<64; ++y)
    {
        // width == 64 pixels --> 2 blocks which are compressed to nibbles

        // First block (0-15 and 16-31)
        for (int x=0; x<16; ++x)
        {
            tstr << (img.pixelIndex(x, y) | (img.pixelIndex(x+16, y) << 4));
            if (((x+1) != 32) || ((y+1) != 64))
                tstr << ", ";
        }

        // Second block (32-47 and 48-63)
        for (int x=32; x<48; ++x)
        {
            tstr << (img.pixelIndex(x, y) | (img.pixelIndex(x+16, y) << 4));
            if (((x+1) != 48) || ((y+1) != 64))
                tstr << ", ";
        }

        tstr << "\n";
    }
#endif

#if 0
    for (int y=starty; y<(starty+16); ++y)
    {
        tstr << "    ";
        for (int x=startx; x<(startx+16); ++x)
        {
            qDebug() << "pI:" << img.pixelIndex(x, y) << img.pixelIndex(x+16, y);
            tstr << (img.pixelIndex(x, y) | (img.pixelIndex(x+16, y) << 4));
            if (((x+1) != (startx+16)) || ((y+1) != (starty+16)))
                tstr << ", ";
        }
        tstr << "\n";
    }
#endif
    tstr << "};\n\n";
}

}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList args = app.arguments();

    if (args.size() < 4)
        qFatal("Not enough arguments!\nUsage: <infile> <outfile> <basename>");

    QImage img(args.at(1));

    QTransform tr;
    tr.translate(32, 32);
    tr.rotate(270);
    tr.translate(-32, -32);
    img = img.transformed(tr).mirrored(false, true); // Save it rotated/mirrored, as this speeds up displaying

//    img.save("temp.bmp");

    qDebug() << "Image color format:" << img.format();

    if (img.depth() != 8)
        qFatal("Only images with 8 bit depth are supported");

    if ((img.width() != 64) || (img.height() != 64))
        qFatal("Only 64x64 images supported");

    qDebug() << "First 10 pixels:";

    for (int i=0; i<10; ++i)
    {
        QColor c = img.pixel(i, 0);
        qDebug() << "RGB:" << c.red() << c.green() << c.blue();
    }

    QFile outfile(args.at(2));
    if (!outfile.open(QFile::WriteOnly | QFile::Text))
        qFatal("Unable to open output file");

    QTextStream tstr(&outfile);
    tstr << "// Automatically generated header file for " << QFileInfo(args.at(1)).fileName() << "\n";

    const QString varbase = args.at(3);

    // palette

    // First make sure that transparent color is set at index 0
    const QRgb trRGB = qRgb(153, 0, 136);
    int oldtransind = -1;
    for (int i=0; i<img.colorCount(); ++i)
    {
        const QRgb c(img.color(i));
        if (c == trRGB)
        {
            oldtransind = i;
            if (i != 0)
            {
                const QRgb swapc(img.color(0));
                img.setColor(0, c);
                img.setColor(i, swapc);
            }
            break;
        }
    }


    tstr << "static const uint16_t " << varbase << "Pal[] PROGMEM = {\n";
    qDebug() << "Colors:";
    for (int i=0; i<img.colorCount(); ++i)
    {
        const QRgb c(img.color(i));
        tstr << "    ";
        if (c == trRGB)
            tstr << TRANSPARENT;
        else
            tstr << RGB(qRed(c), qGreen(c), qBlue(c));
        if ((i+1) < img.colorCount())
            tstr << ", ";
        tstr << "\n";

        qDebug() << "RGB: " << qRed(c) << qGreen(c) << qBlue(c);
    }

    tstr << "};\n\n";


    // Texture data
    dumpSprite(img, tstr, varbase, oldtransind);

#if 0
    for (int s=0; s<8; s+=2)
        dumpSprite(img, tstr, (s & 1) * 32, (s / 2) * 16,
                   QString("%1Sprite%2_%3").arg(varbase).arg(s).arg(s+1));
#endif

    outfile.close();


//    return a.exec();
    app.quit();
}
