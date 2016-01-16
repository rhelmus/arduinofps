#!/usr/bin/python

# From http://gameduino2.proboards.com/post/444/thread

import Image
import gameduino2 as gd2
import os
import zlib

files_old = [ "WAL00000.png",
        "WAL00004.png",
        "WAL00010.png",
        "WAL00012.png",
        "WAL00018.png",
        "WAL00026.png",
        "WAL00028.png",
        "WAL00034.png",
        "SPR00050.png"
        ]

# From GD2's prep.py
def ul(x):
    return str(x) + "UL"

def wall(f, n):
    return dict(filename = f, name = n, fmt = gd2.RGB565)

def entity(f, n):
    return dict(filename = f, name = n, fmt = gd2.ARGB1555)

gfxEntries = [
        wall("WAL00000.png", "wall0"),
        wall("WAL00004.png", "wall1"),
        wall("WAL00010.png", "wall2"),
        wall("WAL00012.png", "wall3"),
        wall("WAL00018.png", "wall4"),
        wall("WAL00026.png", "wall5"),
        wall("WAL00028.png", "wall6"),
        wall("WAL00034.png", "wall7"),

        entity("SPR00050.png", "soldier0"),
    ]

class BaseAsset(gd2.prep.AssetBin):
    def __init__(self, filename, name, fmt, header, output):
        gd2.prep.AssetBin.__init__(self)
        self.assetName = name.upper() + "_IMG"
        self.fmt = fmt
        self.filename = filename
        self.header = header
        self.prefix = name.upper() + "_"
        self.asset_file = output

    def addall(self):
        # use addim: no need for handles etc.
        self.align(2) # this is something load_handle normally does, but addim doesn't
        self.addim(self.assetName, Image.open(self.filename), self.fmt)

    def extras(self, hh):
        #print >>hh, "#undef LOAD_ASSETS"
        pass

    # modified version from GD2's prep.y. Mainly stripped things we don't want
    def make(self):
        self.addall()
        if len(self.alldata) > 0x40000:
            print "Error: The data (%d bytes) is larger the the GD2 RAM" % len(self.alldata)
            sys.exit(1)
        self.defines.append((self.prefix + "ASSETS_END", ul(len(self.alldata))))
        # disable this: we want to dynamically set the ptr for cmd_inflate
#        self.cmd_inflate(0)
        calldata = zlib.compress(self.alldata)
        print 'Assets report'
        print '-------------'
        print 'Header file:    %s' % self.header
        print 'GD2 RAM used:   %d' % len(self.alldata)
        if not self.asset_file:
            print 'Flash used:     %d' % len(calldata)
        else:
            print 'Output file:    %s' % self.asset_file
            print 'File size:      %d' % len(calldata)

        commandblock = self.commands + calldata

        hh = open(self.header, "w")
        open(self.asset_file, "wb").write(commandblock)
        print >>hh, '#define %sLOAD_ASSETS()  GD.safeload("%s");' % (self.prefix, self.asset_file)
        for (nm,v) in self.defines:
            print >>hh, "#define %s %s" % (nm, v)
        for i in self.inits:
            print >>hh, i
        self.extras(hh)


class Wall(gd2.prep.AssetBin):
    header = "assets.h"
    def addall(self):
        i = 0
        for i in range(0, 8):
            print "File: ", files_old[i]
            self.defines.append(("WALL" + str(i) + "_OFFSET", gd2.prep.ul(len(self.alldata))))
            wall = gd2.prep.split(1, 64, Image.open(files_old[i]))
            self.load_handle("WALL" + str(i), wall, gd2.RGB565)

        self.defines.append(("SOLDIER_OFFSET", gd2.prep.ul(len(self.alldata))))
        self.load_handle("SOLDIER", [ Image.open(files_old[8]) ], gd2.ARGB1555)

if __name__ == '__main__':
    Wall().make()


    hdir = "gfx_header"
    gdir = "gfx"

    if not os.path.exists(hdir):
        os.mkdir(hdir)
    if not os.path.exists(gdir):
        os.mkdir(gdir)

    with open("gfx.h", "w") as gfxheader:
        for e in gfxEntries:
            h = ("%s/%s.h") % (hdir, e["name"])
            BaseAsset(e["filename"], e["name"], e["fmt"], h, ("%s/%s.gd2") % (gdir, e["name"])).make()
            print >>gfxheader, ("#include \"%s\"") % h

        print >>gfxheader, "\nenum Sprite\n{"
        for e in gfxEntries:
            print >>gfxheader, ("    SPRITE_%s,") % e["name"].upper()
        print >>gfxheader, "    SPRITE_END,\n    SPRITE_NONE\n};"

        print >>gfxheader, """
struct SpriteInfo
{
    const char *file;
    int width, height;
    int fmt;
    uint32_t size;
};"""

        print >>gfxheader, ("\nconst SpriteInfo sprites[SPRITE_END]\n{")
        for e in gfxEntries:
            print >>gfxheader, ("    {{ \"{0}.gd2\", {1}_IMG_WIDTH, {1}_IMG_HEIGHT, {2}, {1}_ASSETS_END }},").format(e["name"], e["name"].upper(), e["fmt"])
        print >>gfxheader, "};"

