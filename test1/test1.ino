#include <SPI.h>
#include <GD.h>

#include "render.h"
#include "font8x8.h"

namespace {

#define FRAMEBUFFER 0x6fff

static byte stretch[16] = {
  0x00, 0x03, 0x0c, 0x0f,
  0x30, 0x33, 0x3c, 0x3f,
  0xc0, 0xc3, 0xcc, 0xcf,
  0xf0, 0xf3, 0xfc, 0xff
};

void ascii()
{
    long i;
    for (i = 0; i < 768; i++) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        byte b = pgm_read_byte_far(GET_FAR_ADDRESS(font8x8) + i);
#else
        byte b = pgm_read_byte(font8x8 + i);
#endif
        byte h = stretch[b >> 4];
        byte l = stretch[b & 0xf];
        GD.wr(FRAMEBUFFER + /*(16 * ' ')*/ + (2 * i), h);
        GD.wr(FRAMEBUFFER + /*(16 * ' ')*/ + (2 * i) + 1, l);
    }
    /*for (i = 0x20; i < 0x80; i++)*/
    for (i = 0x0; i < 0x60; i++)
    {
        GD.setpal(4 * i + 0, TRANSPARENT);
        GD.setpal(4 * i + 3, RGB(255,255,255));
    }
//     GD.fill(RAM_PIC, ' ', 4096);
    GD.fill(RAM_PIC, 255, 4096);
}

void putstr(int x, int y, const char *s)
{
    GD.__wstart(FRAMEBUFFER + (y << 6) + x);
    while (*s)
        SPI.transfer(*s++);
    GD.__end();
}

}


void setup()
{
    Serial.begin(115200);
    GD.begin();
    GD.fill(RAM_PAL, 0, 8 * 256);
    GD.fill(FRAMEBUFFER, 0, 16 * 256);
    ascii();
    
    /*for (uint16_t i=0; i<4096; i+=2)
        GD.wr16(FRAMEBUFFER + i, i);*/

    GD.microcode(render_code, sizeof(render_code));
    
    delay(1000);
    
    const uint8_t rlen = 8, clen = 32;
    for (uint8_t row=0; row<rlen; ++row)
    {
        for (uint8_t col=0; col<clen; ++col)
            GD.wr(RAM_PIC + row * 64 + col, col + ((row % 2) == 0 ? 1 : 0) * clen);
    }
//     GD.putstr(10, 10, "Heuh");
}

void loop()
{
    static uint8_t dump = 1;
    
    if (dump)
    {
        --dump;
        delay(2000);
        /*for (uint16_t ch=0; ch<4096; ch+=2)
        {
            Serial.print(ch, DEC); Serial.print(": "); Serial.println(GD.rd16(RAM_CHR + ch), DEC);
        }*/
        for (uint16_t ch=0; ch<256; ++ch)
        {
            Serial.print(ch, DEC); Serial.print(": "); Serial.println(GD.rd(RAM_PIC + ch), DEC);
        }
    }
}
