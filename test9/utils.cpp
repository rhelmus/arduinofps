#include <SPI.h>
#include <GD2.h>
#include <transports/wiring.h>

#include "utils.h"

// defined in GD2.cpp
extern GDTransport GDTR;

// adjusted from GDClass::load()
bool loadGDFile(const char *filename)
{
    const uint32_t bufsize = /*4 * 1024*/512; // make it a bit larger (was 512)

    GD.__end();
    Reader r;
    if (r.openfile(filename))
    {
//        byte buf[512];
        uint8_t buf[bufsize];
        while (r.offset < r.size)
        {
            uint16_t n = min(bufsize, r.size - r.offset);
            n = (n + 3) & ~3;   // force 32-bit alignment
            r.readsector(buf);

            GD.resume();
            GD.copyram(buf, n);
            GDTR.stop();
        }
        GD.resume();
        return 1;
    }
    GD.resume();
    return 0;
}

bool loadGDFileInVMem(const char *filename, VPtr<uint8_t, SPIRAMVAlloc> ptr)
{
    const uint32_t bufsize = 512; // fixed size by GD FAT lib

    GD.__end();
    Reader r;
    if (r.openfile(filename))
    {
        uint8_t buf[bufsize];
        while (r.offset < r.size)
        {
            uint16_t n = min(bufsize, r.size - r.offset);
            n = (n + 3) & ~3;   // force 32-bit alignment
            r.readsector(buf);

            memcpy(ptr, buf, n);
            ptr += n;
            //GDTR.stop(); // need this?
        }
        GD.resume();
        return 1;
    }
    GD.resume();
    return 0;
}
