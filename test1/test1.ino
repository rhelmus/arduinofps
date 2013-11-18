#include <SPI.h>
#include <GD.h>

#include "render.h"
#include "font8x8.h"

namespace {

#define FRAMEBUFFER RAM_SPRIMG // Use all sprite memory for now

#if 0
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

#endif

enum
{
    SCREEN_WIDTH = 256,
    SCREEN_WIDTH_CH = SCREEN_WIDTH / 8,
    SCREEN_HEIGHT = 64,
    SCREEN_HEIGHT_CH = SCREEN_HEIGHT / 8
};

uint16_t charFromPos(uint16_t chx, uint16_t chy)
{
    return FRAMEBUFFER + chy * (16 * SCREEN_WIDTH_CH) + (chx * 16);
}

}


void setup()
{
    Serial.begin(115200);
    GD.begin();
    GD.fill(FRAMEBUFFER, 0, 16 * 1024);
    GD.fill(RAM_PAL, 0, 2 * 1024);
    GD.fill(RAM_PIC, 255, 4 * 1024);
    GD.fill(RAM_CHR, 0, 4 * 1024);
    GD.microcode(render_code, sizeof(render_code));
    
    // Setup framebuffer
    for (uint8_t row=0; row<SCREEN_HEIGHT_CH; ++row)
    {
        for (uint8_t col=0; col<SCREEN_WIDTH_CH; ++col)
            GD.wr(RAM_PIC + row * 64 + col, col + ((row % 2) == 0) * SCREEN_WIDTH_CH);
    }

    // All of the same palette for now
    for (uint16_t i=0; i<(SCREEN_WIDTH_CH * 2); ++i)
    {
        GD.setpal(4 * i + 0, TRANSPARENT);
        GD.setpal(4 * i + 1, RGB(255,0,0));
        GD.setpal(4 * i + 2, RGB(0,255,0));
        GD.setpal(4 * i + 3, RGB(0,0,255));

//        GD.wr16(RAM_PAL + 8 * i + 0, TRANSPARENT);
//        GD.wr16(RAM_PAL + 8 * i + 2, RGB(255,0,0));
        /*GD.wr16(RAM_PAL + 8 * i + 4, RGB(0,255,0));
        GD.wr16(RAM_PAL + 8 * i + 6, RGB(0,0,255));*/
    }

    // Test

//    GD.setpal(0, RGB(255, 255, 255));
//    for (uint16_t i=0; i<(4096); ++i)
//        GD.wr(FRAMEBUFFER + i, 255);
//    GD.fill(charFromPos(0, 0), 255, 16);
}

void loop()
{
#if 0
    static uint8_t dump = 1;
    
    if (dump)
    {
        --dump;
        delay(2000);
        for (uint16_t ch=0; ch<4096; ++ch)
        {
            Serial.print(ch, DEC); Serial.print(": "); Serial.println(GD.rd(FRAMEBUFFER + ch), DEC);
        }
        /*for (uint16_t ch=0; ch<(25 * 64); ++ch)
        {
            Serial.print(ch, DEC); Serial.print(": "); Serial.println(GD.rd(RAM_PIC + ch), DEC);
        }*/
    }
#endif

    static uint32_t uptime;
    static uint8_t ci;
    const uint32_t curtime = millis();

    if (uptime < curtime)
    {
        uptime = curtime + 750;
        uint8_t v;
        if (ci == 0)
            v = 0;
        else if (ci == 1)
            v = 0x55;
        else if (ci == 2)
            v = 0xAA;
        else v = 255;

        GD.waitvblank();
        GD.fill(FRAMEBUFFER, v, 16 * SCREEN_WIDTH_CH * SCREEN_HEIGHT_CH);

        ++ci;
        if (ci > 3)
            ci = 0;
    }
}
