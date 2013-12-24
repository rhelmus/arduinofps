#include <SPI.h>
#include <GDT3.h>

#include "render.h"

namespace {

enum
{
    SCREEN_WIDTH = 160,
    SCREEN_WIDTH_SPR = SCREEN_WIDTH / 16,
    RAM_SPRZOOM = RAM_SPRPAL
};

void setPixel4bpp(uint8_t spr, uint8_t spx, uint8_t spy, uint8_t c)
{
    const uint16_t addr = RAM_SPRIMG + ((spr / 4) * 256) + (spy * 16) + spx;
    uint8_t curpx = GD.rd(addr);
    GD.wr(addr, curpx | ((c & 0b11) << (2 * (spr & 3))));
}

void setPixel16bpp(uint8_t spr, uint8_t spx, uint8_t spy, uint8_t c)
{
    const bool low = (spr > 63);
    const uint16_t addr = RAM_SPRIMG + ((spr & 63) * 256) + (spy * 16) + spx;
    uint8_t curpx = GD.rd(addr);
    GD.wr(addr, curpx | ((c & 0b1111) << (4 * low)));
}

void initSprites(void)
{
    GD.wr16(PALETTE4A, RGB(255, 255, 255));
    GD.wr16(PALETTE4A + 2, RGB(255, 0, 0));
    GD.wr16(PALETTE4A + 4, RGB(0, 255, 0));
    GD.wr16(PALETTE4A + 6, RGB(0, 0, 255));

    GD.wr16(PALETTE4B + 0, RGB(0, 0, 255));
    GD.wr16(PALETTE4B + 2, RGB(0, 255, 0));
    GD.wr16(PALETTE4B + 4, RGB(255, 0, 0));
    GD.wr16(PALETTE4B + 6, RGB(255, 255, 255));

    for (uint8_t i=0; i<(2*SCREEN_WIDTH_SPR); ++i)
    {
        const uint8_t y = 10 + ((i / SCREEN_WIDTH_SPR) * 96);
        GD.sprite(i, (i*16) % SCREEN_WIDTH, y, i/4, 0b1000 | ((i & 3) << 1));

        for (uint8_t j=0; j<16; ++j)
            setPixel4bpp(i, j & 15, 7, 2);

        GD.wr(RAM_SPRZOOM + (i * 4), y);
        GD.wr16(RAM_SPRZOOM + (i * 4) + 1, 256 - (256 / 5));
    }

    // Duplicate sprite page
    for (uint16_t i=0; i<1024; ++i)
        GD.wr(RAM_SPR + 1024 + i, GD.rd(RAM_SPR + i));

    GD.wr(COMM+0, SCREEN_WIDTH_SPR);

#if 0
    GD.wr16(PALETTE16A, RGB(255, 255, 255));
    GD.wr16(PALETTE16A + 2, RGB(255, 0, 0));
    GD.wr16(PALETTE16A + 4, RGB(0, 255, 0));
    GD.wr16(PALETTE16A + 6, RGB(0, 0, 255));

    GD.wr16(PALETTE16B + 0, RGB(0, 0, 255));
    GD.wr16(PALETTE16B + 2, RGB(0, 255, 0));
    GD.wr16(PALETTE16B + 4, RGB(255, 0, 0));
    GD.wr16(PALETTE16B + 6, RGB(255, 255, 255));

    GD.sprite(0, 25, 100, 0, 0b0100);
    GD.sprite(1, 50, 100, 1, 0b0101);
    GD.sprite(2, 75, 100, 2, 0b1000);
    GD.sprite(3, 100, 100, 3, 0b1001);

    for (uint8_t i=0; i<16; ++i)
    {
        setPixel16bpp(0, i, 7, 2);
        setPixel16bpp(1, i, 7, 2);
        setPixel4bpp(2, i, 7, 2);
        setPixel4bpp(3, i, 7, 2);
    }
#endif

#if 0
    for (uint8_t i=0; i<16; ++i)
        GD.fill(RAM_SPRIMG + (i*16), (i / 4), 16);

    uint16_t spr = 0;
    for (uint16_t y=16; y<200; y+=32)
    {
        for (uint16_t x=0; x<350; x+=32)
        {
            GD.sprite(spr, x, 0, 0, 0b1000);
            GD.wr(RAM_SPR + 2048 + (spr * 4), y);
            GD.wr16(RAM_SPR + 2048 + (spr * 4) + 1, 256 - (256 / 2));

            ++spr;
        }
    }

    GD.wr(COMM+0, spr);
#endif

#if 0
    // ref sprites
    for (uint16_t y=16; y<200; y+=16)
        GD.sprite(spr++, 128, y, 0, 0b1000);
#endif
}

}


void setup()
{
    Serial.begin(115200);
    GD.begin();
    initSprites();
    GD.microcode(render_code, sizeof(render_code));

#if 0
    GD.wr(COMM+0, 2);
    GD.wr(RAM_SPR + 2048 + 0, 10);
    GD.wr16(RAM_SPR + 2048 + 1, 256 - (256 / 4));
    GD.wr(RAM_SPR + 2048 + 4, 10);
    GD.wr16(RAM_SPR + 2048 + 4 + 1, 256 - (256 / 2));
#endif

}


void loop()
{
    /*static uint16_t y = 10;
    
    delay(1000);
    y = (y + 10) & 255;
    
    GD.wr(RAM_SPR + 2, y);*/
    
}

