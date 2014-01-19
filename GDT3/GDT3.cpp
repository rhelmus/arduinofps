/*
 * Copyright (c) 2011 by James Bowman <jamesb@excamera.com>
 * Gameduino library for arduino.
 *
 */

#ifdef BOARD_maple

#include "wirish.h"
HardwareSPI SPI(1);
#include "GDT3.h"

#else
#include "Arduino.h"
#include <avr/pgmspace.h>
#include <SPIFIFO.h>
#include <GDT3.h>
#endif

GDClass GD;

// UNDONE
#define bswap16(x) ((x)>>8 | ((x)&255)<<8)

void GDClass::begin()
{
  delay(250); // give Gameduino time to boot
  setSPI();

  GD.wr(J1_RESET, 1);           // HALT coprocessor
  __wstart(RAM_SPR);            // Hide all sprites
  for (int i = 0; i < 511; i++)
    GD.xhide();
  GD.xhide(true);
  __end();
  fill(RAM_PIC, 0, 1024 * 10);  // Zero all character RAM
  fill(RAM_SPRPAL, 0, 2048);    // Sprite palletes black
  fill(RAM_SPRIMG, 0, 64 * 256);   // Clear all sprite data
  fill(VOICES, 0, 256);         // Silence
  fill(PALETTE16A, 0, 128);     // Black 16-, 4-palletes and COMM

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

void GDClass::end() {
}

void GDClass::setSPI()
{
    SPIFIFO.begin(9, SPI_CLOCK_8MHz);
}

void GDClass::__start(unsigned int addr) // start an SPI transaction to addr
{
  SPIFIFO.write16(addr, SPI_CONTINUE);
  SPIFIFO.read();
}

void GDClass::__wstart(unsigned int addr) // start an SPI write transaction to addr
{
  __start(0x8000|addr);
}

void GDClass::__wstartspr(unsigned int sprnum)
{
  __start((0x8000 | RAM_SPR) + (sprnum << 2));
  spr = 0;
}

void GDClass::__end() // end the SPI transaction
{
}

byte GDClass::rd(unsigned int addr)
{
  SPIFIFO.write16(addr, SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(0);
  return SPIFIFO.read();
}

void GDClass::wr(unsigned int addr, byte v)
{
  __wstart(addr);
  SPIFIFO.write(v);
  SPIFIFO.read();
  __end();
}

unsigned int GDClass::rd16(unsigned int addr)
{
  SPIFIFO.write16(addr, SPI_CONTINUE);
  SPIFIFO.read();

#if 1 
  SPIFIFO.write16(0);
  return SPIFIFO.read();
#else
  unsigned int r;
  SPIFIFO.write(0, SPI_CONTINUE);
  r = SPIFIFO.read();
  SPIFIFO.write(0);
  r |= (SPIFIFO.read() << 8);
  return r;
#endif
}

void GDClass::wr16(unsigned int addr, unsigned int v)
{
  __wstart(addr);
  
#if 1
  SPIFIFO.write16(bswap16(v));
  SPIFIFO.read();
#else
  SPIFIFO.write(lowByte(v), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(highByte(v));
  SPIFIFO.read();
#endif
  __end();
}

void GDClass::fill(int addr, byte v, unsigned int count)
{
  __wstart(addr);
  while (count > 1)
  {
      SPIFIFO.write(v, SPI_CONTINUE);
      SPIFIFO.read();
      --count;
  }
  SPIFIFO.write(v);
  SPIFIFO.read();
  __end();
}

void GDClass::copy(unsigned int addr, const uint8_t *src, int count)
{
  __wstart(addr);
  while (count > 1) {
    SPIFIFO.write(pgm_read_byte_near(src), SPI_CONTINUE);
    SPIFIFO.read();
    src++;
    --count;
  }
  
  SPIFIFO.write(pgm_read_byte_near(src));
  SPIFIFO.read();
  
  __end();
}

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
void GDClass::copy(unsigned int addr, uint_farptr_t src, int count)
{
  __wstart(addr);
  while (count--) {
    SPI.transfer(pgm_read_byte_far(src));
    src++;
  }
  __end();
}
#endif

void GDClass::microcode(const uint8_t *src, int count)
{
  wr(J1_RESET, 1);
  copy(J1_CODE, src, count);
  wr(J1_RESET, 0);
}

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
void GDClass::microcode(uint_farptr_t src, int count)
{
  wr(J1_RESET, 1);
  copy(J1_CODE, src, count);
  wr(J1_RESET, 0);
}
#endif

void GDClass::setpal(int pal, unsigned int rgb)
{
  wr16(RAM_PAL + (pal << 1), rgb);
}

void GDClass::sprite(int spr, int x, int y, byte image, byte palette, byte rot, byte jk)
{
  __wstart(RAM_SPR + (spr << 2));
  SPIFIFO.write(lowByte(x), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write((palette << 4) | (rot << 1) | (highByte(x) & 1), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(lowByte(y), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write((jk << 7) | (image << 1) | (highByte(y) & 1));
  SPIFIFO.read();
  __end();
}

void GDClass::xsprite(int ox, int oy, char x, char y, byte image, byte palette, byte rot, byte jk)
{
  if (rot & 2)
    x = -16-x;
  if (rot & 4)
    y = -16-y;
  if (rot & 1) {
      int s;
      s = x; x = y; y = s;
  }
  ox += x;
  oy += y;
  SPIFIFO.write(lowByte(ox), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write((palette << 4) | (rot << 1) | (highByte(ox) & 1), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(lowByte(oy), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write((jk << 7) | (image << 1) | (highByte(oy) & 1), SPI_CONTINUE);
  SPIFIFO.read();
  spr++;
}

void GDClass::xhide(bool last)
{
  SPIFIFO.write16(bswap16(400), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write16(bswap16(400), !last);
  SPIFIFO.read();
  spr++;
}

void GDClass::plots(int ox, int oy, PROGMEM sprplot *psp, byte count, byte rot, byte jk)
{
  while (count--) {
    struct sprplot sp;
    memcpy_P((void*)&sp, psp++, sizeof(sp));
    GD.xsprite(ox, oy, sp.x, sp.y, sp.image, sp.palette, rot, jk);
  }
}

// UNDONE
#if 0
void GDClass::sprite2x2(int spr, int x, int y, byte image, byte palette, byte rot, byte jk)
{
  __wstart(0x3000 + (spr << 2));
  GD.xsprite(x, y, -16, -16, image + 0, palette, rot, jk);
  GD.xsprite(x, y,   0, -16, image + 1, palette, rot, jk);
  GD.xsprite(x, y, -16,   0, image + 2, palette, rot, jk);
  GD.xsprite(x, y,   0,   0, image + 3, palette, rot, jk);
  __end();
}
#endif

void GDClass::waitvblank()
{
  // Wait for the VLANK to go from 0 to 1: this is the start
  // of the vertical blanking interval.

  while (rd(VBLANK) == 1)
    ;
  while (rd(VBLANK) == 0)
    ;
}

/* Fixed ascii font, useful for debug */

#include "font8x8.h"
static byte stretch[16] = {
  0x00, 0x03, 0x0c, 0x0f,
  0x30, 0x33, 0x3c, 0x3f,
  0xc0, 0xc3, 0xcc, 0xcf,
  0xf0, 0xf3, 0xfc, 0xff
};


void GDClass::ascii()
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
    GD.wr(0x1000 + (16 * ' ') + (2 * i), h);
    GD.wr(0x1000 + (16 * ' ') + (2 * i) + 1, l);
  }
  for (i = 0x20; i < 0x80; i++) {
    GD.setpal(4 * i + 0, TRANSPARENT);
    GD.setpal(4 * i + 3, RGB(255,255,255));
  }
  GD.fill(RAM_PIC, ' ', 4096);
}

void GDClass::putstr(int x, int y, const char *s)
{
  GD.__wstart((y << 6) + x);
  while (*s && *(s + 1))
  {
      SPIFIFO.write(*s, SPI_CONTINUE);
      SPIFIFO.read();
      ++s;
  }
  
  if (*s)
  {
      SPIFIFO.write(*s);
      SPIFIFO.read();
  }
  GD.__end();
}

void GDClass::voice(int v, byte wave, unsigned int freq, byte lamp, byte ramp)
{
  __wstart(VOICES + (v << 2));
  SPIFIFO.write(lowByte(freq), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(highByte(freq) | (wave << 7), SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(lamp, SPI_CONTINUE);
  SPIFIFO.read();
  SPIFIFO.write(ramp);
  SPIFIFO.read();
  __end();
}

void GDClass::screenshot(unsigned int frame)
{
  int yy, xx;
  byte undone[38];  // 300-long bitmap of lines pending

  // initialize to 300 ones
  memset(undone, 0xff, 37);
  undone[37] = 0xf;
  int nundone = 300;

  Serial.write(0xa5);   // sync byte
  Serial.write(lowByte(frame));
  Serial.write(highByte(frame));

  while (nundone) {
    // find a pending line a short distance ahead of the raster
    int hwline = GD.rd16(SCREENSHOT_Y) & 0x1ff;
    for (yy = (hwline + 7) % 300; ((undone[yy>>3] >> (yy&7)) & 1) == 0; yy = (yy + 1) % 300)
      ;
    GD.wr16(SCREENSHOT_Y, 0x8000 | yy);   // ask for it

    // housekeeping while waiting: mark line done and send yy
    undone[yy>>3] ^= (1 << (yy&7));
    nundone--;
    Serial.write(lowByte(yy));
    Serial.write(highByte(yy));
    while ((GD.rd(SCREENSHOT_Y + 1) & 0x80) == 0)
      ;

    // Now send the line, compressing zero pixels
    uint16_t zeroes = 0;
    for (xx = 0; xx < 800; xx += 2) {
      uint16_t v = GD.rd16(SCREENSHOT + xx);
      if (v == 0) {
        zeroes++;
      } else {
        if (zeroes) {
          Serial.write(lowByte(zeroes));
          Serial.write(0x80 | highByte(zeroes));
          zeroes = 0;
        }
        Serial.write(lowByte(v));
        Serial.write(highByte(v));
      }
    }
    if (zeroes) {
      Serial.write(lowByte(zeroes));
      Serial.write(0x80 | highByte(zeroes));
    }
  }
  GD.wr16(SCREENSHOT_Y, 0);   // restore screen to normal
}

// near ptr version
class GDflashbits {
public:
  void begin(const uint8_t *s) {
    src = s;
    mask = 0x01;
  }
  byte get1(void) {
    byte r = (pgm_read_byte_near(src) & mask) != 0;
    mask <<= 1;
    if (!mask) {
      mask = 1;
      src++;
    }
    return r;
  }
  unsigned short getn(byte n) {
    unsigned short r = 0;
    while (n--) {
      r <<= 1;
      r |= get1();
    }
    return r;
  }
private:
  const uint8_t *src;
  byte mask;
};

static GDflashbits GDFB;

void GDClass::uncompress(unsigned int addr, const uint8_t *src)
{
  GDFB.begin(src);
  byte b_off = GDFB.getn(4);
  byte b_len = GDFB.getn(4);
  byte minlen = GDFB.getn(2);
  unsigned short items = GDFB.getn(16);
  while (items--) {
    if (GDFB.get1() == 0) {
      GD.wr(addr++, GDFB.getn(8));
    } else {
      int offset = -GDFB.getn(b_off) - 1;
      int l = GDFB.getn(b_len) + minlen;
      while (l--) {
        GD.wr(addr, GD.rd(addr + offset));
        addr++;
      }
    }
  }
}

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
// far ptr version
class GDflashbitsF {
public:
  void begin(uint_farptr_t s) {
    src = s;
    mask = 0x01;
  }
  byte get1(void) {
    byte r = (pgm_read_byte_far(src) & mask) != 0;
    mask <<= 1;
    if (!mask) {
      mask = 1;
      src++;
    }
    return r;
  }
  unsigned short getn(byte n) {
    unsigned short r = 0;
    while (n--) {
      r <<= 1;
      r |= get1();
    }
    return r;
  }
private:
  uint_farptr_t src;
  byte mask;
};
static GDflashbitsF GDFBF;

void GDClass::uncompress(unsigned int addr, uint_farptr_t src)
{
  GDFBF.begin(src);
  byte b_off = GDFBF.getn(4);
  byte b_len = GDFBF.getn(4);
  byte minlen = GDFBF.getn(2);
  unsigned short items = GDFBF.getn(16);
  while (items--) {
    if (GDFBF.get1() == 0) {
      GD.wr(addr++, GDFBF.getn(8));
    } else {
      int offset = -GDFBF.getn(b_off) - 1;
      int l = GDFBF.getn(b_len) + minlen;
      while (l--) {
        GD.wr(addr, GD.rd(addr + offset));
        addr++;
      }
    }
  }
}
#endif
