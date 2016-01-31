#!/usr/bin/python

# From http://gameduino2.proboards.com/post/444/thread

import copy
import Image
import gameduino2 as gd2
import glob
import os
import zlib

from collections import OrderedDict

# From GD2's prep.py
def ul(x):
    return str(x) + "UL"


gfxWalls = OrderedDict([
    ('wall0', 'walls/WAL00000.png'),
    ('wall1', 'walls/WAL00004.png'),
    ('wall2', 'walls/WAL00010.png'),
    ('wall3', 'walls/WAL00012.png'),
    ('wall4', 'walls/WAL00018.png'),
    ('wall5', 'walls/WAL00026.png'),
    ('wall6', 'walls/WAL00028.png'),
    ('wall7', 'walls/WAL00034.png')])

gfxEntities = OrderedDict([
    ('soldier00', 'entities/SPR00050.png'),
    ('soldier01', 'entities/SPR00051.png'),
    ('soldier02', 'entities/SPR00052.png'),
    ('soldier03', 'entities/SPR00053.png'),
    ('soldier04', 'entities/SPR00054.png'),
    ('soldier05', 'entities/SPR00055.png'),
    ('soldier06', 'entities/SPR00056.png'),
    ('soldier07', 'entities/SPR00057.png')])

gfxEntries = copy.deepcopy(gfxWalls)
gfxEntries.update(gfxEntities) # merge dicts

class BaseAsset(gd2.prep.AssetBin):
    def __init__(self, filename, fmt, output):
        gd2.prep.AssetBin.__init__(self)
        self.fmt = fmt
        self.filename = filename
        self.asset_file = output

    def addall(self):
        # use addim: no need for handles etc.
        self.align(2) # this is something load_handle normally does, but addim doesn't
        img = Image.open(self.filename)
        self.addim(None, img, self.fmt)
        (self.width, self.height) = img.size

    # modified version from GD2's prep.y. Mainly stripped things we don't want
    def make(self):
        self.addall()
        if len(self.alldata) > 0x40000:
            print "Error: The data (%d bytes) is larger the the GD2 RAM" % len(self.alldata)
            sys.exit(1)
        # disable this: we want to dynamically set the ptr for cmd_inflate
#        self.cmd_inflate(0)
        calldata = zlib.compress(self.alldata)
        self.size = ul(len(self.alldata))
        self.csize = ul(len(calldata))

        print 'Assets report'
        print '-------------'
        print 'GD2 RAM used:   %d' % len(self.alldata)
        if not self.asset_file:
            print 'Flash used:     %d' % len(calldata)
        else:
            print 'Output file:    %s' % self.asset_file
            print 'File size:      %d' % len(calldata)

        commandblock = self.commands + calldata

        open(self.asset_file, "wb").write(commandblock)


if __name__ == '__main__':
    gdir = "gfx"

    if not os.path.exists(gdir):
        os.mkdir(gdir)

    with open("gfx.h", "w") as gfxheader:
        print >>gfxheader, "\nenum Sprite\n{"
        for e in gfxEntries:
            print >>gfxheader, ("    SPRITE_%s,") % os.path.splitext(os.path.basename(e))[0].upper()
        print >>gfxheader, "    SPRITE_END,\n    SPRITE_NONE\n};"

        print >>gfxheader, """
struct SpriteInfo
{
    const char *file;
    int width, height;
    int fmt;
    uint32_t size, comprsize;
};"""

        print >>gfxheader, ("\nconst SpriteInfo sprites[SPRITE_END]\n{")
        for e, f in gfxEntries.iteritems():
            fmt = gd2.RGB565 if e in gfxWalls else gd2.ARGB1555
            outf = os.path.splitext(os.path.basename(f))[0] + ".gd2"
            a = BaseAsset(f, fmt, ("%s/%s") % (gdir, outf))
            a.make()
            print >>gfxheader, ("    {{ \"{0}\", {1}, {2}, {3}, {4}, {5} }},").format(outf, a.width, a.height, fmt, a.size, a.csize)
        print >>gfxheader, "};"

