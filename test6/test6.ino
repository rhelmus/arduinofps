#include <SPI.h>
#include <GDT3.h>

#include "guard.h"

namespace {

void initSprites(void)
{
    GD.copy(PALETTE16A, (const uint8_t *)guardPal, sizeof(guardPal));
    /*for (int i=0; i<16; ++i)
        GD.wr16(PALETTE16A + (i*2), RGB(255, 0, 0));*/

    // Copy 32x16 pixel (ie 4bpp sprites) blocks to sprite ram
#if 1
    uint8_t spriteblock = 0;
    for (uint8_t starty=0; starty<64; starty+=16)
    {
        for (uint8_t startx=0; startx<32; startx+=16)
        {
            for (uint8_t y=starty; y<(starty+16); ++y)
                GD.copy(RAM_SPRIMG + (spriteblock * 256) + ((y & 15) * 16), guardTex + (y * 32) + startx, 16);
            ++spriteblock;
        }
    }
#endif
//    GD.fill(RAM_SPRIMG, 0b01100110, 16 * 1024);

    // Layout sprites
    for (uint8_t i=0; i<16; i+=2)
    {
        GD.sprite(i, (i & 3) * 16, (i/4) * 16, i/2, 0b0100); // low byte
        GD.sprite(i+1, ((i+1) & 3) * 16, (i/4) * 16, i/2, 0b0110); // high byte
    }
}


}

void setup()
{
    Serial.begin(115200);
    GD.begin();
    initSprites();

//    GD.microcode(render_code, sizeof(render_code));
}

void loop()
{

}
