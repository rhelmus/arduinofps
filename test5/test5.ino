#include <SPI.h>
#include <GDT3.h>

#include "render.h"

namespace {

void initSprites(void)
{
    // init colours (black, red, green, blue)
    GD.wr16(PALETTE4A, RGB(255, 255, 255));
    GD.wr16(PALETTE4A + 2, RGB(255, 0, 0));
    GD.wr16(PALETTE4A + 4, RGB(0, 255, 0));
    GD.wr16(PALETTE4A + 6, RGB(0, 0, 255));
    
    for (uint8_t i=0; i<16; ++i)
        GD.fill(RAM_SPRIMG + (i*16), (i / 4), 16);

    uint16_t spr = 0;
    for (uint16_t y=16; y<200; y+=32)
    {
        for (uint16_t x=0; x<250; x+=32)
        {
            GD.sprite(spr, x, 0, 0, 0b1000);
            GD.wr(RAM_SPR + 2048 + (spr * 4), y);
            GD.wr16(RAM_SPR + 2048 + (spr * 4) + 1, 256 - (256 / 2));

            ++spr;
        }
    }

    GD.wr(COMM+0, spr);

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

