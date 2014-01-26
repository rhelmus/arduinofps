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

void dumpHeader(QImage &img, const QString &hname, const QString &varbase)
{
    QFile outfile(hname);
    if (!outfile.open(QFile::WriteOnly | QFile::Text))
        qFatal("Unable to open output file");

    QTextStream tstr(&outfile);
    tstr << "// Automatically generated header file\n";

    // palette

    const int oldtransind = fixColors(img);

    tstr << "static const uint16_t " << varbase << "Pal[] PROGMEM = {\n";

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

    tstr << "};\n\n";

    outfile.close();
}

void testRawFile(const QImage &img, const QString &fname, int oldtransind);

void dumpRawFile(QImage &img, const QString &fname)
{
    QFile outfile(fname);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        qFatal("Unable to open output file");

    // palette

    const int oldtransind = fixColors(img);

    for (int i=0; i<img.colorCount(); ++i)
    {
        uint16_t c;

        if (i == 0)
            c = TRANSPARENT;
        else
            c = getGDRGB(img.color(i));

        outfile.putChar(c & 0xFF); // low
        outfile.putChar(c >> 8); // high
    }

    // Texture data
    for (int y=0; y<64; ++y)
    {
        // Row data stored as nibbles, low byte == even px, high byte == odd px
        for (int x=0; x<64; x+=2)
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

    const QByteArray palba = infile.read(16 * 2); // palette (16 colors, 2 bytes for each color)

    for (int i=0; i<img.colorCount(); ++i)
    {
        const uint16_t c = (uint8_t)palba.at(i * 2) | ((uint16_t)palba.at(i * 2 + 1) << 8);
        const uint16_t imgc = (i == 0) ? TRANSPARENT : getGDRGB(img.color(i));
        if (c != imgc)
            qCritical() << "color index" << i << "differs:" << c << imgc;
    }

    const QByteArray texby = infile.read(64 * 32); // texture (64 * 64, nibble per pixel)

    for (int y=0; y<64; ++y)
    {
        for (int x=0; x<64; x+=2)
        {
            const uint8_t px = texby.at(y * (64/2) + (x/2));
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
    bool dumpheader = false;

    if (args.size() > 1)
    {
        if (args.at(1) == "--header")
            dumpheader = true;
        else if (args.at(1) == "--raw")
            dumpheader = false;
        else
            qFatal("Please specify --header or --raw");
    }

    if (args.size() < ((dumpheader) ? 5 : 4))
        qFatal("Not enough arguments!\nUsage: [--header/--raw] <infile> <outfile> (<basename>)");

    QImage img(args.at(2));

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

    if (dumpheader)
        dumpHeader(img, args.at(3), args.at(4));
    else
        dumpRawFile(img, args.at(3));

    app.quit();
}
