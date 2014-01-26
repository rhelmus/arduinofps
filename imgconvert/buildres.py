#!/usr/bin/python

import os

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
    "../wolf3d-res/SPR_GRD_S_1.bmp"
]


def mkdirp(directory):
    if not os.path.isdir(directory):
        os.makedirs(directory)

def createConcatImage(images, outfile):
    strlist = " ".join(images)
    os.system("convert +append %s %s" % (strlist, outfile))

def ditherImage(infile, outfile):
    os.system("convert %s -dither FloydSteinberg -colors 16 -depth 4 %s" % (infile, outfile))

def remapPalette(infile, outfile, palfile):
    os.system("convert %s -dither FloydSteinberg -remap %s %s" % (infile, palfile, outfile))

mkdirp("tmp")

createConcatImage(wallImages + spriteImages, "tmp/concatall.bmp")
ditherImage("tmp/concatall.bmp", "tmp/concatall-dithered.bmp")

createConcatImage(wallImages, "tmp/concatwalls.bmp")
remapPalette("tmp/concatwalls.bmp", "tmp/concatwalls-dithered.bmp", "tmp/concatall-dithered.bmp")

createConcatImage(spriteImages, "tmp/concatsprites.bmp")
remapPalette("tmp/concatsprites.bmp", "tmp/concatsprites-dithered.bmp", "tmp/concatall-dithered.bmp")


os.remove("tmp/concatall.bmp")
os.remove("tmp/concatall-dithered.bmp")
os.remove("tmp/concatwalls.bmp")
os.remove("tmp/concatwalls-dithered.bmp")
os.remove("tmp/concatsprites.bmp")
os.remove("tmp/concatsprites-dithered.bmp")
os.rmdir("tmp")
