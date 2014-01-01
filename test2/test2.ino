#define USE_SPIFIFO

#include <stdint.h>

#ifdef USE_SPIFIFO
#include <Arduino.h>
#include <SPIFIFO.h>
#include <GDT3.h>
#else
#include <SPI.h>
#include <GD.h>
#include "fastspi.h"
#endif

#include "fixmath.h"
#include "render.h"

#define FRAMEBUFFER RAM_SPRIMG

namespace {

struct SRowData
{
    uint8_t color : 4;
    uint8_t top;
    uint8_t bottom;
    uint8_t texX : 6;
    uint8_t lineHeight;
    uint16_t texZ;
};

enum
{
    SCREEN_WIDTH = 320,
    SCREEN_WIDTH_CH = SCREEN_WIDTH / 8,
    SCREEN_HEIGHT = 200,
    SCREEN_HEIGHT_CH = SCREEN_HEIGHT / 8,

    TEX_WIDTH = 64,
    TEX_HEIGHT = 64,

    MAP_WIDTH = 24,
    MAP_HEIGHT = 24
};


uint8_t texture[TEX_WIDTH * TEX_HEIGHT];


uint16_t charOffset(uint8_t chx, uint8_t chy)
{
    return chy * SCREEN_WIDTH_CH + chx;
}

void GDcopyram(uint16_t addr, uint8_t *src, uint16_t count)
{
    GD.__wstart(addr);
    while (count > 1) {
#ifdef USE_SPIFIFO
        SPIFIFO.write(*src, SPI_CONTINUE);
        SPIFIFO.read();
#else
        SPI.transfer(*src);
#endif
        ++src;
        --count;
    }

#ifdef USE_SPIFIFO
        SPIFIFO.write(*src);
        SPIFIFO.read();
#else
        SPI.transfer(*src);
#endif

    GD.__end();
}

uint16_t getRGBColor(uint8_t c)
{
    if (c == 0)
        return RGB(255, 0, 0);
    else if (c == 1)
        return RGB(0, 255, 0);
    else
        return RGB(0, 0, 255);
}

void drawScreen(struct SRowData * __restrict__ rowdata) __attribute__((optimize("-O3")));

#ifdef USE_SPIFIFO

void drawScreen(struct SRowData * __restrict__ rowdata)
{
    uint16_t colorbshift[8];

    // cache this to make to avoid constant recalculations in the loops below
    for (uint8_t i=0; i<8; ++i)
        colorbshift[i] = 14 - 2 * i;// ((i < 4) ? (6 - (i*2)) : (14-((i*2)-8)));

//    uint32_t ptime = 0;

    uint16_t curch[8];
    memset(curch, 0, sizeof(curch));

#if 0
#else
    SPIFIFO.write16(FRAMEBUFFER | 0x8000, SPI_CONTINUE);
    SPIFIFO.read();
#endif

    for (uint_fast8_t chrow=0; chrow<SCREEN_HEIGHT_CH; ++chrow)
    {
        const uint_fast16_t chrowy = chrow * 8, nextchrowy = chrowy + 8;

        for (uint_fast16_t x=0; x<SCREEN_WIDTH; ++x)
        {
            const uint_fast8_t chx = x & 7; // chx mod 8

            if (x && !chx)
            {
#if 0
                GDcopyram(FRAMEBUFFER + (charOffset(x/8, chrow) * 16), (uint8_t *)curch, sizeof(curch));
                memset(curch, 0, sizeof(curch));
#else
                // First write?
                if (!chrow && (x == 8))
                {
                    // Fill FIFO
                    SPIFIFO.write16(curch[0], SPI_CONTINUE);
                    SPIFIFO.write16(curch[1], SPI_CONTINUE);
                    SPIFIFO.write16(curch[2], SPI_CONTINUE);

                    for (uint_fast8_t i=3; i<8; ++i)
                    {
                        SPIFIFO.write16(curch[i], SPI_CONTINUE);
                        SPIFIFO.read();
                    }
                }
                else
                {
                    for (uint_fast8_t i=0; i<8; ++i)
                    {
                        SPIFIFO.write16(curch[i], SPI_CONTINUE);
                        SPIFIFO.read();
                    }
                }
                memset(curch, 0, sizeof(curch));
#endif
            }

            if ((nextchrowy <= rowdata[x].top) || (chrowy > rowdata[x].bottom))
                continue;

            const uint_fast8_t starty = (rowdata[x].top > chrowy) ? (rowdata[x].top - chrowy) : 0;
            const uint_fast8_t endy = (rowdata[x].bottom < nextchrowy) ? (rowdata[x].bottom - chrowy) : 7;

            const uint_fast16_t d = (chrowy + starty) - (SCREEN_HEIGHT / 2) + (rowdata[x].lineHeight / 2);
            uint_fast16_t texY = ((uint32_t)d * (uint32_t)rowdata[x].texZ);

            for (uint_fast8_t chy=starty; chy<=endy; ++chy)
            {
                // UNDONE: rotate texture
                curch[chy] |= ((uint16_t)texture[TEX_HEIGHT * rowdata[x].texX + (texY/256)] << colorbshift[chx]);

                texY += rowdata[x].texZ;

//                ptime += (micros() - t);
            }
        }

#if 0
        GDcopyram(FRAMEBUFFER + (charOffset(SCREEN_WIDTH_CH-1, chrow) * 16), (uint8_t *)curch, sizeof(curch));
        memset(curch, 0, sizeof(curch));
#else
        for (uint_fast8_t i=0; i<8; ++i)
        {
            SPIFIFO.write16(curch[i], SPI_CONTINUE);
            SPIFIFO.read();
        }
        memset(curch, 0, sizeof(curch));
#endif
    }

#if 0
#else
    // UNDONE
    SPIFIFO.write(0);
    SPIFIFO.read();

    // Empty FIFO
    SPIFIFO.read();
    SPIFIFO.read();
    SPIFIFO.read();
#endif

//    Serial.print("ptime: "); Serial.println(ptime / 1000);
}

#else

void drawScreen(struct SRowData * __restrict__ rowdata)
{
    static uint16_t chpicline[SCREEN_WIDTH];
    uint16_t colorbshift[8];

    // cache this to make to avoid constant recalculations in the loops below
    for (uint8_t i=0; i<8; ++i)
        colorbshift[i] = ((i < 4) ? (6 - (i*2)) : (14-((i*2)-8)));

//    uint32_t ptime = 0;

    for (uint_fast8_t chrow=0; chrow<SCREEN_HEIGHT_CH; ++chrow)
    {
        const uint_fast16_t chrowy = chrow * 8, nextchrowy = chrowy + 8;

        memset(chpicline, 0, sizeof(chpicline));

        for (uint_fast16_t x=0; x<SCREEN_WIDTH; ++x)
        {
            if ((nextchrowy <= rowdata[x].top) || (chrowy > rowdata[x].bottom))
                continue;

            const uint_fast8_t starty = (rowdata[x].top > chrowy) ? (rowdata[x].top - chrowy) : 0;
            const uint_fast8_t endy = (rowdata[x].bottom < nextchrowy) ? (rowdata[x].bottom - chrowy) : 7;
            const uint_fast8_t chx = x & 7; // chx mod 8
            const uint_fast16_t picx = x - chx;

            const uint_fast16_t d = (chrowy + starty) - (SCREEN_HEIGHT / 2) + (rowdata[x].lineHeight / 2);
//            const uint_fast8_t startTexY = ((uint32_t)d * (uint32_t)rowdata[x].texZ) / 256;
            uint_fast16_t texY = ((uint32_t)d * (uint32_t)rowdata[x].texZ);

            for (uint_fast8_t chy=starty; chy<=endy; ++chy)
            {
#if 0
//                const uint32_t t = micros();

                const uint_fast8_t texY = startTexY + (((chy - starty) * rowdata[x].texZ) / 256);
//                const uint_fast16_t cv = ((uint16_t)texture[TEX_HEIGHT * texY + rowdata[x].texX] << colorbshift[chx]);
                // UNDONE: rotate texture
                const uint_fast16_t cv = ((uint16_t)texture[TEX_HEIGHT * rowdata[x].texX + texY] << colorbshift[chx]);


                //make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
//                if(side == 1) color = (color >> 1) & 8355711;
//                const uint16_t cv = ((uint16_t)rowdata[x].color << colorbshift[chx]);
#endif

                // UNDONE: rotate texture
                const uint_fast16_t cv = ((uint16_t)texture[TEX_HEIGHT * rowdata[x].texX + (texY/256)] << colorbshift[chx]);
                chpicline[picx + chy] |= cv;

                texY += rowdata[x].texZ;

//                ptime += (micros() - t);
            }
        }

#if 0
        GDcopyram(FRAMEBUFFER + (charOffset(0, chrow) * 16), (uint8_t *)chpicline, sizeof(chpicline));
#else
#ifdef USE_SPIFIFO
        SPIFIFO.write16((FRAMEBUFFER + (charOffset(0, chrow) * 16)) | 0x8000, SPI_CONTINUE);
        SPIFIFO.read();

        const uint_fast16_t endi = (sizeof(chpicline) / sizeof(chpicline[0])) - 1;

        for (uint_fast16_t i=0; i<endi; ++i)
        {
            //SPIFIFO.write16(chpicline[i], SPI_CONTINUE);
//            SPIFIFO.read();
            SPIFIFO.write(lowByte(chpicline[i]), SPI_CONTINUE);
            SPIFIFO.read();
            SPIFIFO.write(highByte(chpicline[i]), SPI_CONTINUE);
            SPIFIFO.read();
        }

        SPIFIFO.write16(chpicline[endi-1]);
        SPIFIFO.read();
#else
        fastSPI.startTransfer(FRAMEBUFFER + (charOffset(0, chrow) * 16));

        for (uint_fast16_t i=0; i<sizeof(chpicline); ++i)
            SPI_WRITE_8(*((uint8_t *)chpicline + i));

        fastSPI.endTransfer();
#endif
#endif
    }

//    Serial.print("ptime: "); Serial.println(ptime / 1000);
}
#endif

// --------

const uint8_t worldMap[MAP_HEIGHT][MAP_WIDTH]=
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
    {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
    {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

#if 0
float posX = 22, posY = 12;  //x and y start position
float dirX = -1, dirY = 0; //initial direction vector
float planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane
#else
fix16_t posX = F16(22), posY = F16(12);  //x and y start position
fix16_t dirX = F16(-1), dirY = 0; //initial direction vector
fix16_t planeX = 0, planeY = F16(0.66); //the 2d raycaster version of camera plane
#endif

// --------

void raycast(void) __attribute__((optimize("-O3")));

#if 0
void raycast()
{
    static SRowData rowdata[SCREEN_WIDTH];
    
    for (uint16_t x=0; x<SCREEN_WIDTH; ++x)
    {
        //calculate ray position and direction 
        const float cameraX = 2 * x / float(SCREEN_WIDTH) - 1; //x-coordinate in camera space
        const float rayPosX = posX;
        const float rayPosY = posY;
        const float rayDirX = dirX + planeX * cameraX;
        const float rayDirY = dirY + planeY * cameraX;
        //which box of the map we're in  
        uint8_t mapX = uint8_t(rayPosX);
        uint8_t mapY = uint8_t(rayPosY);
        
        //length of ray from current position to next x or y-side
        float sideDistX;
        float sideDistY;
        
        //length of ray from one x or y-side to next x or y-side
        float deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
        float deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));
        float perpWallDist;
        
        //what direction to step in x or y-direction (either +1 or -1)
        int8_t stepX;
        int8_t stepY;
        
        int hit = 0; //was there a wall hit?
        int side; //was a NS or a EW wall hit?
        //calculate step and initial sideDist
        if ((float)rayDirX < 0)
        {
            stepX = -1;
            sideDistX = ((float)rayPosX - mapX) * (float)deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1.0f - (float)rayPosX) * (float)deltaDistX;
        }
        if ((float)rayDirY < 0)
        {
            stepY = -1;
            sideDistY = ((float)rayPosY - mapY) * (float)deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1.0f - (float)rayPosY) * (float)deltaDistY;
        }
        
        //perform DDA
        while (hit == 0)
        {
            //jump to next map square, OR in x-direction, OR in y-direction
            if (sideDistX < sideDistY)
            {
                sideDistX += (float)deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += (float)deltaDistY;
                mapY += stepY;
                side = 1;
            }
            //Check if ray has hit a wall
            if (worldMap[mapX][mapY] > 0)
                hit = 1;
        }
        
        //Calculate distance projected on camera direction (oblique distance will give fisheye effect!)
        if (side == 0)
            perpWallDist = fabs((mapX - rayPosX + (1 - stepX) / 2) / (float)rayDirX);
        else
            perpWallDist = fabs((mapY - rayPosY + (1 - stepY) / 2) / (float)rayDirY);
        
        //Calculate height of line to draw on screen
        int16_t lineHeight = abs(int(SCREEN_HEIGHT / perpWallDist));
        
        //calculate lowest and highest pixel to fill in current stripe
        int16_t drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if(drawStart < 0)
            drawStart = 0;

        int16_t drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if(drawEnd >= SCREEN_HEIGHT)
            drawEnd = SCREEN_HEIGHT - 1;
        
        // -----------------
        

        //texturing calculations
        rowdata[x].color = worldMap[mapX][mapY] - 1; //1 subtracted from it so that texture 0 can be used!

        //calculate value of wallX
        float wallX; //where exactly the wall was hit
        if (side == 1)
            wallX = rayPosX + ((mapY - rayPosY + (1 - stepY) / 2) / rayDirY) * rayDirX;
        else
            wallX = rayPosY + ((mapX - rayPosX + (1 - stepX) / 2) / rayDirX) * rayDirY;
        wallX -= floor(wallX);

        //x coordinate on the texture
        uint8_t texX = uint8_t(wallX * float(TEX_WIDTH));
        if (((side == 0) && (rayDirX > 0)) || ((side == 1) && (rayDirY < 0)))
            texX = TEX_WIDTH - texX - 1;


        // -------------------

        // rowdata[x].color = constrain(worldMap[mapX][mapY], 1, 3); //1/*w % 3 + 1*/;
        rowdata[x].top = drawStart; //80 - (x / 5);
        rowdata[x].bottom = drawEnd; //127 + (x / 5);
        rowdata[x].lineHeight = lineHeight;
        rowdata[x].texX = texX;
    }
    
#if 0
    const uint32_t ctime = millis();
    GD.wr(COMM+0, 1); // Clear screen
    while (GD.rd(COMM+0))
        ; // Wait till done
    Serial.print("ctime: "); Serial.println(millis() - ctime, DEC);
#else
    GD.waitvblank();
#endif

    const uint32_t dtime = millis();
    drawScreen(rowdata);

    Serial.print("dtime: "); Serial.println(millis() - dtime, DEC);
}

#else

void raycast()
{
    static SRowData rowdata[SCREEN_WIDTH];
    const uint_fast8_t iposx = fix16_to_int(posX);
    const uint_fast8_t iposy = fix16_to_int(posY);

    fix16_t cameraX = F16(-1); //x-coordinate in camera space
    const fix16_t camxincr = fix16_div(F16(2), F16(SCREEN_WIDTH));

    for (uint_fast16_t x=0; x<SCREEN_WIDTH; ++x)
    {
        //calculate ray position and direction
        const fix16_t rayDirX = fix16_add(dirX, fix16_mul(planeX, cameraX));
        const fix16_t rayDirY = fix16_add(dirY, fix16_mul(planeY, cameraX));

        cameraX = fix16_add(cameraX, camxincr);

        //which box of the map we're in
        uint_fast8_t mapX = iposx;
        uint_fast8_t mapY = iposy;

        //length of ray from current position to next x or y-side
        fix16_t sideDistX;
        fix16_t sideDistY;

        //length of ray from one x or y-side to next x or y-side
        const fix16_t deltaDistX = fix16_sqrt(fix16_sadd(F16(1), fix16_sdiv(fix16_sq(rayDirY), fix16_sq(rayDirX))));
        const fix16_t deltaDistY = fix16_sqrt(fix16_sadd(F16(1), fix16_sdiv(fix16_sq(rayDirX), fix16_sq(rayDirY))));

        fix16_t perpWallDist;

        //what direction to step in x or y-direction (either +1 or -1)
        int_fast8_t stepX;
        int_fast8_t stepY;

        uint_fast8_t side; //was a NS or a EW wall hit?
        //calculate step and initial sideDist
        if (rayDirX < 0)
        {
            stepX = -1;
            sideDistX = fix16_mul(fix16_sub(posX, fix16_from_int(mapX)), deltaDistX);
        }
        else
        {
            stepX = 1;
            sideDistX = fix16_mul(fix16_sub(fix16_from_int(mapX + 1), posX), deltaDistX);
        }
        if (rayDirY < 0)
        {
            stepY = -1;
            sideDistY = fix16_mul(fix16_sub(posY, fix16_from_int(mapY)), deltaDistY);
        }
        else
        {
            stepY = 1;
            sideDistY = fix16_mul(fix16_sub(fix16_from_int(mapY + 1), posY), deltaDistY);
        }

        //perform DDA
        do
        {
            //jump to next map square, OR in x-direction, OR in y-direction
            if (sideDistX < sideDistY)
            {
                sideDistX = fix16_add(sideDistX, deltaDistX);
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY = fix16_add(sideDistY, deltaDistY);
                mapY += stepY;
                side = 1;
            }
        }
        while (worldMap[mapX][mapY] == 0);

        //Calculate distance projected on camera direction (oblique distance will give fisheye effect!)
        if (side == 0)
            perpWallDist = fix16_abs(fix16_div(fix16_add(fix16_sub(fix16_from_int(mapX), posX), fix16_div(fix16_from_int(1 - stepX), F16(2))), rayDirX));
        else
            perpWallDist = fix16_abs(fix16_div(fix16_add(fix16_sub(fix16_from_int(mapY), posY), fix16_div(fix16_from_int(1 - stepY), F16(2))), rayDirY));

        //Calculate height of line to draw on screen
        const uint16_t lineHeight = fix16_to_int(fix16_div(F16(SCREEN_HEIGHT), perpWallDist));

        //calculate lowest and highest pixel to fill in current stripe
        int_fast16_t drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0)
            drawStart = 0;

        uint_fast16_t drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT)
            drawEnd = SCREEN_HEIGHT - 1;

        // -----------------


        //texturing calculations
        rowdata[x].color = worldMap[mapX][mapY] - 1; //1 subtracted from it so that texture 0 can be used!

        //calculate value of wallX
        fix16_t wallX; //where exactly the wall was hit
        if (side == 1)
            wallX = fix16_add(posX, fix16_mul(fix16_div(fix16_add(fix16_sub(fix16_from_int(mapY), posY), fix16_div(fix16_from_int(1 - stepY), F16(2))), rayDirY), rayDirX));
        else
            wallX = fix16_add(posY, fix16_mul(fix16_div(fix16_add(fix16_sub(fix16_from_int(mapX), posX), fix16_div(fix16_from_int(1 - stepX), F16(2))), rayDirX), rayDirY));
        wallX = fix16_sub(wallX, fix16_floor(wallX));

        //x coordinate on the texture
        rowdata[x].texX = fix16_to_int(fix16_mul(wallX, F16(TEX_WIDTH)));
        if (((side == 0) && (rayDirX > 0)) || ((side == 1) && (rayDirY < 0)))
            rowdata[x].texX = TEX_WIDTH - rowdata[x].texX - 1;


        // -------------------

        // rowdata[x].color = constrain(worldMap[mapX][mapY], 1, 3); //1/*w % 3 + 1*/;
        rowdata[x].top = drawStart;
        rowdata[x].bottom = drawEnd;
        rowdata[x].lineHeight = lineHeight;
        rowdata[x].texZ = 256 * TEX_HEIGHT / rowdata[x].lineHeight;
    }

#if 0
    const uint32_t ctime = millis();
    GD.wr(COMM+0, 1); // Clear screen
    while (GD.rd(COMM+0))
        ; // Wait till done
    Serial.print("ctime: "); Serial.println(millis() - ctime, DEC);
#else
    GD.waitvblank();
#endif

    const uint32_t dtime = millis();
    drawScreen(rowdata);

    Serial.print("dtime: "); Serial.println(millis() - dtime, DEC);
}

#endif


#ifndef USE_SPIFIFO
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
#endif

void initTextures(void)
{
    //generate some textures
    for(uint8_t y=0; y<TEX_HEIGHT; ++y)
    {
        for(uint8_t x=0; x<TEX_WIDTH; ++x)
        {
            texture[TEX_WIDTH * y + x] = 1 + x / (TEX_WIDTH / 3);
#if 0
            int xorcolor = (x * 256 / texWidth) ^ (y * 256 / texHeight);
            //int xcolor = x * 256 / texWidth;
            int ycolor = y * 256 / texHeight;
            int xycolor = y * 128 / texHeight + x * 128 / texWidth;
            texture[0][texWidth * y + x] = 65536 * 254 * (x != y && x != texWidth - y); //flat red texture with black cross
            texture[1][texWidth * y + x] = xycolor + 256 * xycolor + 65536 * xycolor; //sloped greyscale
            texture[2][texWidth * y + x] = 256 * xycolor + 65536 * xycolor; //sloped yellow gradient
            texture[3][texWidth * y + x] = xorcolor + 256 * xorcolor + 65536 * xorcolor; //xor greyscale
            texture[4][texWidth * y + x] = 256 * xorcolor; //xor green
            texture[5][texWidth * y + x] = 65536 * 192 * (x % 16 && y % 16); //red bricks
            texture[6][texWidth * y + x] = 65536 * ycolor; //red gradient
            texture[7][texWidth * y + x] = 128 + 256 * 128 + 65536 * 128; //flat grey texture
#endif
        }
    }
}

}


void setup()
{
    Serial.begin(115200);

#ifdef USE_SPIFIFO

#if 0
    SPIFIFO.begin(9, SPI_CLOCK_8MHz);
    delay(4000);

    SPIFIFO.write(0x28, true);
    Serial.print("ret: "); Serial.println(SPIFIFO.read(), HEX);
    SPIFIFO.write(0x00, true);
    Serial.print("ret: "); Serial.println(SPIFIFO.read(), HEX);
    SPIFIFO.write(0x00, false);
    Serial.print("ret: "); Serial.println(SPIFIFO.read(), HEX);

    SPIFIFO.write16(0x8000, true);
    Serial.print("ret: "); Serial.println(SPIFIFO.read(), HEX);
    /*SPIFIFO.write(0x00, true);
    Serial.print("ret: "); Serial.println(SPIFIFO.read(), HEX);*/
    SPIFIFO.write(0x41, false);
    Serial.print("ret: "); Serial.println(SPIFIFO.read(), HEX);

    return;
#else
    GD.begin();
    /*SPIFIFO.begin(9, SPI_CLOCK_8MHz);
    delay(4000);
    Serial.print("ret: "); Serial.println(GD.rd(0x2800), HEX);*/
//    GD.wr(0x8000, 0x41);
//    return;
#endif

#else
    beginGD();
#endif

    GD.wr16(BG_COLOR, RGB(0, 0, 0));
    GD.fill(RAM_PIC, 255, 4 * 1024);
    
    // Setup framebuffer
    for (uint8_t row=0; row<SCREEN_HEIGHT_CH; ++row)
    {
        for (uint8_t col=0; col<SCREEN_WIDTH_CH; ++col)
            GD.wr(RAM_PIC + (row + 6) * 64 + col, col + ((row % 2) == 0) * SCREEN_WIDTH_CH);
    }

#if 1
    // All of the same palette for now
    for (uint16_t i=0; i<(SCREEN_WIDTH_CH * 2); ++i)
    {
        GD.wr16(RAM_PAL + (i * 8), TRANSPARENT);
        GD.wr16(RAM_PAL + (i * 8) + 2, getRGBColor(0));
        GD.wr16(RAM_PAL + (i * 8) + 4, getRGBColor(1));
        GD.wr16(RAM_PAL + (i * 8) + 6, RGB(0, 0, 255));
        /*GD.setpal(4 * i + 0, TRANSPARENT);
        GD.setpal(4 * i + 1, getRGBColor(0));
        GD.setpal(4 * i + 2, getRGBColor(1));
        GD.setpal(4 * i + 3, getRGBColor(2));*/
    }
#else
    GD.wr16(RAM_PAL + SCREEN_WIDTH + 0, TRANSPARENT);
    GD.wr16(RAM_PAL + SCREEN_WIDTH + 2, RGB(255, 0, 0));
    GD.wr16(RAM_PAL + SCREEN_WIDTH + 4, RGB(255, 255, 0));
    GD.wr16(RAM_PAL + SCREEN_WIDTH + 6, RGB(255, 0, 0));
//    GD.fill(RAM_PAL, 0x55, SCREEN_WIDTH * 2);
#endif

    initTextures();
    
    GD.microcode(render_code, sizeof(render_code));

#if 0
    for (uint8_t i=0; i<16; ++i)
        GD.wr(FRAMEBUFFER + i, 0xFF);

//    GD.fill(FRAMEBUFFER, 0xFF, 16 * SCREEN_WIDTH_CH * SCREEN_HEIGHT_CH);

    delay(2500);
    Serial.print("rd: ");

    for (uint16_t i=0; i<8; ++i)
    {
        Serial.println(GD.rd16(FRAMEBUFFER + (i*2)));
    }
#endif
}


void loop()
{
    static uint32_t uptime = 0;
    const uint32_t curtime = millis();
    
    if (uptime < curtime)
    {
#if 0
        // ---------
        //both camera direction and camera plane must be rotated
        float moveSpeed = 5.0; //the constant value is in squares/second
        float rotSpeed = 0.3 * 3.0 * 0.1;
        
        float oldDirX = dirX;
        dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
        dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
        float oldPlaneX = planeX;
        planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
        planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);

#else

        const fix16_t rotSpeed = F16(0.3 * 3.0 * 0.1);

        const fix16_t oldDirX = dirX;
        dirX = fix16_sub(fix16_mul(dirX, fix16_cos(-rotSpeed)), fix16_mul(dirY, fix16_sin(-rotSpeed)));
        dirY = fix16_add(fix16_mul(oldDirX, fix16_sin(-rotSpeed)), fix16_mul(dirY, fix16_cos(-rotSpeed)));

        const fix16_t oldPlaneX = planeX;
        planeX = fix16_sub(fix16_mul(planeX, fix16_cos(-rotSpeed)), fix16_mul(planeY, fix16_sin(-rotSpeed)));
        planeY = fix16_add(fix16_mul(oldPlaneX, fix16_sin(-rotSpeed)), fix16_mul(planeY, fix16_cos(-rotSpeed)));
#endif
        // ---------
        
        raycast();
        Serial.print("frame: "); Serial.println(millis() - curtime);
        uptime = curtime + 150;
    }
}

