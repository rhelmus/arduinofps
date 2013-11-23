#include "fastspi.h"

#include <SPI.h>

CFastSPI fastSPI;

void CFastSPI::begin()
{
    pinMode(PIN_CS, OUTPUT);

    // First set up SPI via library
    SPI.begin();
//    SPI.setClockDivider(SPI_CLOCK_DIV8);
//    SPSR = (1 << SPI2X);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    digitalWrite(9, HIGH);

    // Now reconfigure some stuff

    // remember MCR
    origMCR = SPI0_MCR;

    // Use both CTARs, one for 8 bit, the other 16 bit
//    ctar0 = SPI_CTAR_FMSZ(7) | SPI_CTAR_PBR(0) | SPI_CTAR_BR(BAUD_DIV) | SPI_CTAR_CSSCK(BAUD_DIV) | SPI_CTAR_DBR;
//    ctar1 = SPI_CTAR_FMSZ(15) | SPI_CTAR_PBR(0) | SPI_CTAR_BR(BAUD_DIV) | SPI_CTAR_CSSCK(BAUD_DIV) | SPI_CTAR_DBR;

//    SPI0_CTAR0 = ctar0;
//    SPI0_CTAR1 = ctar1;
    setCTar();
}
