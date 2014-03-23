#!/usr/bin/python

import os
import sys

from PIL import Image

wallImages = [
    "../wolf3d-res/WAL00000.png",
    "../wolf3d-res/WAL00001.png",
    "../wolf3d-res/WAL00002.png",
    "../wolf3d-res/WAL00003.png",
    "../wolf3d-res/WAL00004.png",
    "../wolf3d-res/WAL00005.png",
    "../wolf3d-res/WAL00006.png",
    "../wolf3d-res/WAL00007.png",
    "../wolf3d-res/WAL00008.png",
    "../wolf3d-res/WAL00009.png",
    "../wolf3d-res/WAL00010.png"
]

spriteImages = [
    "../wolf3d-res/SPR00050.png"
]


def mkdirp(directory):
    if not os.path.isdir(directory):
        os.makedirs(directory)

def onlyBasename(f):
    return os.path.basename(os.path.splitext(f)[0])

def createConcatImage(images, outfile):
    strlist = " ".join(images)
    os.system("convert +append %s %s" % (strlist, outfile))

def trimImage(infile, outfile):
    os.system("convert %s -bordercolor none -border 3x3 %s.tmp" % (infile, outfile))
    os.system("convert %s.tmp -trim +repage %s" % (outfile, outfile))
    os.remove("%s.tmp" % (outfile))

def flattenImage(infile, outfile):
    os.system("convert %s -background 'rgb(153, 0, 136)' -flatten %s" % (infile, outfile))

def ditherImage(infile, outfile):
    os.system("convert %s -dither FloydSteinberg -colors 16 -depth 4 %s" % (infile, outfile))

def remapPalette(infile, outfile, palfile):
    os.system("convert %s -dither FloydSteinberg -remap %s %s" % (infile, palfile, outfile))

mkdirp("tmp")

spriteImagesStripped = [ ]
for spr in spriteImages:
    splfname = "tmp/%s.png" % onlyBasename(spr)
    trimImage(spr, splfname)
    spriteImagesStripped.append(splfname)

createConcatImage(wallImages + spriteImagesStripped, "tmp/concatall.png")
#flattenImage("tmp/concatall.png", "tmp/concatall-flat.png")
ditherImage("tmp/concatall.png", "tmp/concatall-dithered.png")

createConcatImage(wallImages, "tmp/concatwalls.png")
remapPalette("tmp/concatwalls.png", "tmp/concatwalls-dithered.png", "tmp/concatall-dithered.png")

createConcatImage(spriteImagesStripped, "tmp/concatsprites.png")
remapPalette("tmp/concatsprites.png", "tmp/concatsprites-dithered.png", "tmp/concatall-dithered.png")
trimImage("tmp/concatsprites-dithered.png", "tmp/concatsprites-trimmed.png")
flattenImage("tmp/concatsprites-trimmed.png", "tmp/concatsprites-flat.png")

os.system("%s/imgconvert tmp/concatwalls-dithered.png gfx.h tmp/concatsprites-flat.png sprites.raw" % (os.path.dirname(sys.argv[0])))

hdr = open("gfx-info.h", "w")
hdr.write("// Automatically generated header file\n\n")

# dump enum
hdr.write("enum\n{\n")
for spr in spriteImagesStripped:
    hdr.write("    %s" % onlyBasename(spr))

    if spr == spriteImagesStripped[0]:
        hdr.write(" = 0")

    if spr != spriteImagesStripped[-1]:
        hdr.write(",\n")
    else:
        hdr.write("\n")
hdr.write("};\n")

hdr.write("""
struct SSpriteInfo
{
    uint16_t index, size;
};

// UNDONE: PROGMEM
static const SSpriteInfo spriteInfo[] = {
""")

cursprind = 0
for spr in spriteImagesStripped:
    res = Image.open(spr).size
    size = res[0] * res[1] * 0.5 # 1/2: pixels stored as nibbles
    hdr.write("    { %d, %d }" % (cursprind, size))
    if spr != spriteImagesStripped[-1]:
        hdr.write(",\n")
    else:
        hdr.write("\n")
    cursprind = cursprind + size

hdr.write("};\n")

hdr.close()

#os.remove("tmp/concatall.png")
#os.remove("tmp/concatall-flat.png")
#os.remove("tmp/concatall-dithered.bmp")
#os.remove("tmp/concatwalls.bmp")
#os.remove("tmp/concatwalls-dithered.bmp")
#os.remove("tmp/concatsprites.bmp")
#os.remove("tmp/concatsprites-trimmed.png")
#os.remove("tmp/concatsprites-flat.png")
#os.remove("tmp/concatsprites-dithered.bmp")
#os.rmdir("tmp")
