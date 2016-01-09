#!/usr/bin/python

# From http://gameduino2.proboards.com/post/444/thread

import Image
import gameduino2 as gd2

files = [ "WAL00000.png",
        "WAL00004.png",
        "WAL00010.png",
        "WAL00012.png",
        "WAL00018.png",
        "WAL00026.png",
        "WAL00028.png",
        "WAL00034.png",
        "SPR00050.png"
        ]

class Wall(gd2.prep.AssetBin):
    header = "assets.h"
    def addall(self):
        i = 0
        for i in range(0, 8):
            print "File: ", files[i]
            self.defines.append(("WALL" + str(i) + "_OFFSET", gd2.prep.ul(len(self.alldata))))
            wall = gd2.prep.split(1, 64, Image.open(files[i]))
            self.load_handle("WALL" + str(i), wall, gd2.RGB565)

        self.defines.append(("SOLDIER_OFFSET", gd2.prep.ul(len(self.alldata))))
        self.load_handle("SOLDIER", [ Image.open(files[8]) ], gd2.ARGB1555)

if __name__ == '__main__':
    Wall().make()
