#include <SPI.h>
#include <GD.h>

#include "fastspi.h"
#include "render.h"

namespace {

#define FRAMEBUFFER RAM_SPRIMG // Use all sprite memory for now

enum
{
    SCREEN_WIDTH = 320,
    SCREEN_WIDTH_CH = SCREEN_WIDTH / 8,
    SCREEN_HEIGHT = 200,
    SCREEN_HEIGHT_CH = SCREEN_HEIGHT / 8
};

uint16_t charFromPos(uint16_t chx, uint16_t chy)
{
    return FRAMEBUFFER + chy * (16 * SCREEN_WIDTH_CH) + (chx * 16);
}

// Modified version of GD::begin()
void beginGD()
{
    delay(250); // give Gameduino time to boot

    fastSPI.begin();

    GD.wr(J1_RESET, 1);           // HALT coprocessor
    GD.__wstart(RAM_SPR);            // Hide all sprites
    for (int i = 0; i < 512; i++)
        GD.xhide();
    GD.__end();
    GD.fill(RAM_PIC, 0, 1024 * 10);  // Zero all character RAM
    GD.fill(RAM_SPRPAL, 0, 2048);    // Sprite palletes black
    GD.fill(RAM_SPRIMG, 0, 64 * 256);   // Clear all sprite data
    GD.fill(VOICES, 0, 256);         // Silence
    GD.fill(PALETTE16A, 0, 128);     // Black 16-, 4-palletes and COMM

    GD.wr16(SCROLL_X, 0);
    GD.wr16(SCROLL_Y, 0);
    GD.wr(JK_MODE, 0);
    GD.wr(SPR_DISABLE, 0);
    GD.wr(SPR_PAGE, 0);
    GD.wr(IOMODE, 0);
    GD.wr16(BG_COLOR, 0);
    GD.wr16(SAMPLE_L, 0);
    GD.wr16(SAMPLE_R, 0);
    GD.wr16(SCREENSHOT_Y, 0);
    GD.wr(MODULATOR, 64);
}

void fastFill(uint16_t addr, uint8_t data, uint16_t size)
{
    addr |= 0x8000; // From GD lib

    fastSPI.startTransfer();

    SPI_WRITE_8(highByte(addr));
    SPI_WRITE_8(lowByte(addr));

    for (uint16_t i=0; i<size; ++i)
    {
        SPI_WRITE_8(data);
//        SPI_WAIT();
    }

    fastSPI.endTransfer();
}

}


void setup()
{
    Serial.begin(115200);
//    GD.begin();
    beginGD();
    GD.fill(RAM_PIC, 255, 4 * 1024);
    GD.microcode(render_code, sizeof(render_code));
    
    // Setup framebuffer
    for (uint8_t row=0; row<SCREEN_HEIGHT_CH; ++row)
    {
        for (uint8_t col=0; col<SCREEN_WIDTH_CH; ++col)
            GD.wr(RAM_PIC + (row + 6) * 64 + col, col + ((row % 2) == 0) * SCREEN_WIDTH_CH);
    }

    // All of the same palette for now
    for (uint16_t i=0; i<(SCREEN_WIDTH_CH * 2); ++i)
    {
        GD.setpal(4 * i + 0, TRANSPARENT);
        GD.setpal(4 * i + 1, RGB(255,0,0));
        GD.setpal(4 * i + 2, RGB(0,255,0));
        GD.setpal(4 * i + 3, RGB(0,0,255));
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
    static uint16_t filldelay;
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
        else
            v = 255;

        GD.waitvblank();
        delay(filldelay);
        const uint32_t ct2 = millis();
//        GD.fill(FRAMEBUFFER, v, 16 * SCREEN_WIDTH_CH * SCREEN_HEIGHT_CH);
        fastFill(FRAMEBUFFER, v, 16 * SCREEN_WIDTH_CH * SCREEN_HEIGHT_CH);
        Serial.print("fill time: "); Serial.println(millis() - ct2, DEC);

        ++ci;
        if (ci > 3)
            ci = 0;

        if (Serial.available())
        {
            const uint16_t v = Serial.parseInt();
            if (v == 0)
                Serial.read();
            else
            {
                Serial.print("Got: "); Serial.println(v);
                filldelay = v;
            }
        }
    }
}
