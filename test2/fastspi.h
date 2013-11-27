#ifndef FASTSPI_H
#define FASTSPI_H

#include <Arduino.h>

// Code based on ploveday's example: http://forum.pjrc.com/threads/1156-Teensy-3-SPI-Basic-Clock-Questions?p=29263&viewfull=1#post29263

class CFastSPI
{
    enum { BAUD_DIV = 2, PIN_CS = 9 };

    uint32_t origMCR;

    void setCTar(void)
    { SPI0_CTAR0 = SPI_CTAR_FMSZ(7) | SPI_CTAR_PBR(0) | SPI_CTAR_BR(BAUD_DIV) |
                   SPI_CTAR_CSSCK(BAUD_DIV) | SPI_CTAR_DBR;
      SPI0_CTAR1 = SPI_CTAR_FMSZ(15) | SPI_CTAR_PBR(0) | SPI_CTAR_BR(BAUD_DIV) |
                   SPI_CTAR_CSSCK(BAUD_DIV) | SPI_CTAR_DBR; }

public:
    void begin(void);

    void startTransfer(void) { digitalWrite(PIN_CS, LOW); setCTar(); SPI0_MCR = origMCR; }
    void startTransfer(uint16_t addr);
    void endTransfer(void) { digitalWrite(PIN_CS, HIGH); }
};

extern CFastSPI fastSPI;

#define SPI_SR_TXCTR 0x0000f000

#define SPI_WRITE_8(c) \
    do { \
        while ((SPI0_SR & SPI_SR_TXCTR) >= 0x00004000); \
        SPI0_PUSHR = ((c)&0xff) | SPI_PUSHR_CTAS(0) | SPI_PUSHR_CONT; \
    } while(0)

#define SPI_WRITE_16(w) \
    do { \
        while ((SPI0_SR & SPI_SR_TXCTR) >= 0x00004000); \
        SPI0_PUSHR = ((w)&0xffff) | SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT; \
    } while(0)


#define SPI_WAIT() \
    while ((SPI0_SR & SPI_SR_TXCTR) != 0); \
    while (!(SPI0_SR & SPI_SR_TCF)); \
    SPI0_SR |= SPI_SR_TCF;

#if 0
#define SPI_CS_ON(pin) digitalWrite(pin, 0); \
        SPI0_CTAR0 = ctar0; SPI0_CTAR1 = ctar1; SPI0_MCR = mcr;

#define SPI_CS_OFF(pin) SPI_WAIT(); \
        digitalWrite(pin, 1);

#endif


#endif // FASTSPI_H
