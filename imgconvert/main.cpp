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

uint16_t getGDRGB(const QRgb rgb)
{
    return RGB(qRed(rgb), qGreen(rgb), qBlue(rgb));
}

QImage getImage(const QString &fname)
{
    QImage img(fname);

    if (img.depth() != 8)
        qFatal("Only images with 8 bit depth are supported");

#if 0
    if (((img.width() % 64) != 0) || (img.height() != 64))
    {
        qDebug() << "img:" << fname << "w/h:" << img.width() << img.height();
        qFatal("Wrong image size");
    }
#endif

    QTransform tr;
    tr.translate(img.width()/2, img.height()/2);
    tr.rotate(270);
    tr.translate(-img.width()/2, -img.height()/2);
    img = img.transformed(tr).mirrored(false, true); // Save it rotated/mirrored, as this speeds up displaying

//    img.save("temp.bmp");

    return img;
}

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

int fixColors(QImage &img)
{
    // make sure that transparent color is set at index 0, return previous index

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

    return oldtransind;
}

void dumpHeader(const QString &iname, const QString &hname)
{
    QImage img(getImage(iname));
    QFile outfile(hname);
    if (!outfile.open(QFile::WriteOnly | QFile::Text))
        qFatal("Unable to open output file");

    QTextStream tstr(&outfile);
    tstr << "// Automatically generated header file\n";

    // palette

    const int oldtransind = fixColors(img);

    tstr << "static const uint16_t palette[] PROGMEM = {\n";

    for (int i=0; i<img.colorCount(); ++i)
    {
        tstr << "    ";
        if (i == 0)
            tstr << TRANSPARENT;
        else
            tstr << getGDRGB(img.color(i));

        if ((i+1) < img.colorCount())
            tstr << ", ";
        tstr << "\n";
    }

    tstr << "};\n\n";

    // Texture data
    tstr << "static const uint8_t wallTextures[] PROGMEM = {\n";

    for (int y=0; y<img.height(); ++y)
    {
        // Row data stored as nibbles, low byte == even px, high byte == odd px
        for (int x=0; x<img.width(); x+=2)
        {
            tstr << (getPixel(img, x, y, oldtransind) | (getPixel(img, x+1, y, oldtransind) << 4));
            if (((x+2) != img.width()) || ((y+1) != img.height()))
                tstr << ", ";
        }

        tstr << "\n";
    }

    tstr << "};\n\n";

    outfile.close();
}

void testRawFile(const QImage &img, const QString &fname, int oldtransind);

void dumpRawFile(const QString &iname, const QString &fname)
{
    QImage img(getImage(iname));

    QFile outfile(fname);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        qFatal("Unable to open output file");

    const int oldtransind = fixColors(img);

    // Texture data
    for (int y=0; y<img.height(); ++y)
    {
        // Row data stored as nibbles, low byte == even px, high byte == odd px
        for (int x=0; x<img.width(); x+=2)
            outfile.putChar(getPixel(img, x, y, oldtransind) |
                            (getPixel(img, x+1, y, oldtransind) << 4));
    }

    outfile.close();

    testRawFile(img, fname, oldtransind);
}

void testRawFile(const QImage &img, const QString &fname, int oldtransind)
{
    QFile infile(fname);
    if (!infile.open(QFile::ReadOnly))
        qFatal("Unable to open input file");

    qDebug() << fname << "size:" << infile.size();

    const QByteArray texby = infile.read(img.width() * img.height() / 2); // div 2: nibble per pixel

    for (int y=0; y<img.height(); ++y)
    {
        for (int x=0; x<img.width(); x+=2)
        {
            const uint8_t px = texby.at(y * (img.width()/2) + (x/2));
            const uint8_t imgpx = getPixel(img, x, y, oldtransind) |
                            (getPixel(img, x+1, y, oldtransind) << 4);
            if (px != imgpx)
                qCritical() << "pixel " << x << y << "differs:" << px << imgpx;
        }
    }

    infile.close();
}

}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList args = app.arguments();

    if (args.size() < 5)
        qFatal("Not enough arguments!\nUsage: <headerimg> <headerfile> <rawimg> <rawfile>");

    dumpHeader(args.at(1), args.at(2));
    dumpRawFile(args.at(3), args.at(4));

    app.quit();
}
