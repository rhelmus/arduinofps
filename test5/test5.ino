#include <SPI.h>
#include <GDT3.h>

#include "render.h"

namespace {

void initSprite(void)
{
    // init colours (black, red, green, blue)
    GD.wr16(PALETTE4A, RGB(0, 0, 0));
    GD.wr16(PALETTE4A + 2, RGB(255, 0, 0));
    GD.wr16(PALETTE4A + 4, RGB(0, 255, 0));
    GD.wr16(PALETTE4A + 6, RGB(0, 0, 255));
    
    GD.sprite(0, 30, 10, 0, 0b1000);
    
    for (uint8_t i=0; i<16; ++i)
        GD.fill(RAM_SPRIMG + (i*16), 1 + i / 5, 16);
    
    GD.sprite(1, 90, 10, 0, 0b1000);
}
    
}


void setup()
{
    Serial.begin(115200);
    GD.begin();
    initSprite();
    GD.microcode(render_code, sizeof(render_code));

    GD.wr(COMM+0, 2);
    GD.wr(RAM_SPR + 2048 + 0, 10);
    GD.wr16(RAM_SPR + 2048 + 1, 256 - (256 / 4));
    GD.wr(RAM_SPR + 2048 + 4, 10);
    GD.wr16(RAM_SPR + 2048 + 4 + 1, 256 - (256 / 2));

}


void loop()
{
    /*static uint16_t y = 10;
    
    delay(1000);
    y = (y + 10) & 255;
    
    GD.wr(RAM_SPR + 2, y);*/
    
}

