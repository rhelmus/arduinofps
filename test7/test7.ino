#include <stdint.h>

#include <Arduino.h>
#include <SPIFIFO.h>
#include <GDT3.h>

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
    SCREEN_WIDTH_SPR = SCREEN_WIDTH / 16, // must be multiple of 2

    // Sprites are vertically stretched. For raycasting we consider the unstretched height.
    SCREEN_HEIGHT = 192,
    SCREEN_HEIGHT_RAY = 96,
    SCREEN_HEIGHT_SPR = SCREEN_HEIGHT_RAY / 16,

    SCREEN_STARTX = (400 - SCREEN_WIDTH) / 2,
    SCREEN_STARTY = (300 - SCREEN_HEIGHT) / 2,

    TEX_WIDTH = 64,
    TEX_HEIGHT = 64,

    MAP_WIDTH = 24,
    MAP_HEIGHT = 24
};

// --------
// UNDONE: No globals

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

fix16_t posX = F16(22), posY = F16(12);  //x and y start position
fix16_t dirX = F16(-1), dirY = 0; //initial direction vector
fix16_t planeX = 0, planeY = F16(0.66); //the 2d raycaster version of camera plane


uint8_t texture[TEX_WIDTH * TEX_HEIGHT];

// --------

void GDcopyram(uint16_t addr, uint8_t *src, uint16_t count)
{
    GD.__wstart(addr);
    while (count > 1) {
        SPIFIFO.write(*src, SPI_CONTINUE);
        SPIFIFO.read();
        ++src;
        --count;
    }

    SPIFIFO.write(*src);
    SPIFIFO.read();

    GD.__end();
}

inline void SPIFIFOWrite(uint32_t b, uint32_t cont=0) __attribute__((always_inline));
inline void SPIFIFOWrite(uint32_t b, uint32_t cont)
{
    SPIFIFO.write(b, cont);
    SPIFIFO.read();
}

inline void SPIFIFOWrite16(uint32_t b, uint32_t cont=0) __attribute__((always_inline));
inline void SPIFIFOWrite16(uint32_t b, uint32_t cont)
{
    SPIFIFO.write16(b, cont);
    SPIFIFO.read();
}

void initSprites(void)
{
    // Layout sprites, from left to right, top to bottom

    const uint8_t yspacing = (((SCREEN_HEIGHT / 16) / SCREEN_HEIGHT_SPR) - 1);
    for (uint8_t i=0; i<(SCREEN_WIDTH_SPR * SCREEN_HEIGHT_SPR); i+=2)
    {
        const uint8_t spr = i / 2;
        const uint16_t y = SCREEN_STARTY + (i/SCREEN_WIDTH_SPR) * 16 +
                           (i/SCREEN_WIDTH_SPR) * (yspacing * 16);
        GD.sprite(i, SCREEN_STARTX + (i % SCREEN_WIDTH_SPR) * 16, y, spr, 0b0100); // index low byte
        GD.sprite(i+1, SCREEN_STARTX + ((i+1) % SCREEN_WIDTH_SPR) * 16, y, spr, 0b0110); // index high byte
    }

    // ref sprites for debug
    GD.sprite(200, SCREEN_STARTX - 16 - 1, SCREEN_STARTY + 0, 63, 0b0100);
    GD.sprite(201, SCREEN_STARTX - 16 - 1, SCREEN_STARTY + 32, 63, 0b0100);
    GD.sprite(202, SCREEN_STARTX - 16 - 1, SCREEN_STARTY + 64, 63, 0b0100);
    GD.sprite(203, SCREEN_STARTX - 16 - 1, SCREEN_STARTY + 96, 63, 0b0100);
    GD.sprite(204, SCREEN_STARTX - 16 - 1, SCREEN_STARTY + 128, 63, 0b0100);
    GD.sprite(205, SCREEN_STARTX - 16 - 1, SCREEN_STARTY + 160, 63, 0b0100);
    GD.fill(RAM_SPRIMG + (63 * 256), 0b0001, 256);
}

void initTextures(void)
{
    // Initialize palette
    // UNDONE
    GD.wr16(PALETTE16A, TRANSPARENT);
    GD.wr16(PALETTE16A + 2, RGB(255, 0, 0));
    GD.wr16(PALETTE16A + 4, RGB(0, 255, 0));
    GD.wr16(PALETTE16A + 6, RGB(0, 0, 255));

    //generate some textures
    for(uint8_t y=0; y<TEX_HEIGHT; ++y)
    {
        for(uint8_t x=0; x<TEX_WIDTH; ++x)
        {
            texture[TEX_WIDTH * x + y] = 1 + x / (TEX_WIDTH / 3);
        }
    }
}

void drawScreen(struct SRowData * __restrict__ rowdata) __attribute__((optimize("-O3")));
void drawScreen(struct SRowData * __restrict__ rowdata)
{
    uint8_t cursprblock[256];
    memset(cursprblock, 0, sizeof(cursprblock));

    SPIFIFOWrite16(RAM_SPRIMG | 0x8000, SPI_CONTINUE);

    for (uint_fast8_t sprrow=0; sprrow<SCREEN_HEIGHT_SPR; ++sprrow)
    {
        const uint_fast16_t sprrowy = sprrow * 16, nextsprowy = sprrowy + 16;

        for (uint_fast16_t x=0; x<SCREEN_WIDTH; ++x)
        {
            const uint_fast8_t sprx = x & 15; // sprx mod 16
            const uint_fast8_t highbyte = ((x & 31) >= 16);

            if (x && ((x & 31) == 0))
            {
                // NOTE: write16 is used to speed things up a little bit (hence we need casting)

                // First write?
                if (!sprrow && (x == 32))
                {
                    // Fill FIFO
                    SPIFIFO.write16(((uint16_t *)cursprblock)[0], SPI_CONTINUE);
                    SPIFIFO.write16(((uint16_t *)cursprblock)[1], SPI_CONTINUE);
                    SPIFIFO.write16(((uint16_t *)cursprblock)[2], SPI_CONTINUE);

                    for (uint_fast8_t i=3; i<128; ++i)
                        SPIFIFOWrite16(((uint16_t *)cursprblock)[i], SPI_CONTINUE);
                }
                else
                {
                    for (uint_fast8_t i=0; i<128; ++i)
                        SPIFIFOWrite16(((uint16_t *)cursprblock)[i], SPI_CONTINUE);
                }

                memset(cursprblock, 0, sizeof(cursprblock));
            }

            if ((nextsprowy <= rowdata[x].top) || (sprrowy > rowdata[x].bottom))
                continue;

            const uint_fast8_t starty = (rowdata[x].top > sprrowy) ? (rowdata[x].top - sprrowy) : 0;
            const uint_fast8_t endy = (rowdata[x].bottom < nextsprowy) ? (rowdata[x].bottom - sprrowy) : 15;

            const uint_fast16_t d = (sprrowy + starty) - (SCREEN_HEIGHT_RAY / 2) + (rowdata[x].lineHeight / 2);
            uint_fast16_t texY = d * rowdata[x].texZ;

            for (uint_fast8_t spry=starty; spry<=endy; ++spry)
            {
                const uint_fast8_t c = (texture[TEX_HEIGHT * rowdata[x].texX + (texY/256)] & 0b1111) << (4 * highbyte);
                cursprblock[spry * 16 + sprx] |= c;
                texY += rowdata[x].texZ;
            }
        }

        // Write out last row data
        for (uint_fast8_t i=0; i<128; ++i)
            SPIFIFOWrite16(((uint16_t *)cursprblock)[i], SPI_CONTINUE);
        memset(cursprblock, 0, sizeof(cursprblock));
    }

    // UNDONE
    SPIFIFOWrite(0);

    // Empty FIFO
    SPIFIFO.read();
    SPIFIFO.read();
    SPIFIFO.read();
}

void raycast(void) __attribute__((optimize("-O3")));
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
        const uint16_t lineHeight = fix16_to_int(fix16_div(F16(SCREEN_HEIGHT_RAY), perpWallDist));

        //calculate lowest and highest pixel to fill in current stripe
        int_fast16_t drawStart = -lineHeight / 2 + SCREEN_HEIGHT_RAY / 2;
        if (drawStart < 0)
            drawStart = 0;

        uint_fast16_t drawEnd = lineHeight / 2 + SCREEN_HEIGHT_RAY / 2;
        if (drawEnd >= SCREEN_HEIGHT_RAY)
            drawEnd = SCREEN_HEIGHT_RAY - 1;

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

    GD.waitvblank();

    const uint32_t dtime = millis();
    drawScreen(rowdata);

    Serial.print("dtime: "); Serial.println(millis() - dtime, DEC);
}


}


void setup()
{
    Serial.begin(115200);
    GD.begin();
    GD.wr16(BG_COLOR, RGB(0, 0, 0));

    initSprites();
    initTextures();

    GD.microcode(render_code, sizeof(render_code));
}


void loop()
{
    static uint32_t uptime = 0;
    const uint32_t curtime = millis();
    
    if (uptime < curtime)
    {
        const fix16_t rotSpeed = F16(0.3 * 3.0 * 0.1);

        const fix16_t oldDirX = dirX;
        dirX = fix16_sub(fix16_mul(dirX, fix16_cos(-rotSpeed)), fix16_mul(dirY, fix16_sin(-rotSpeed)));
        dirY = fix16_add(fix16_mul(oldDirX, fix16_sin(-rotSpeed)), fix16_mul(dirY, fix16_cos(-rotSpeed)));

        const fix16_t oldPlaneX = planeX;
        planeX = fix16_sub(fix16_mul(planeX, fix16_cos(-rotSpeed)), fix16_mul(planeY, fix16_sin(-rotSpeed)));
        planeY = fix16_add(fix16_mul(oldPlaneX, fix16_sin(-rotSpeed)), fix16_mul(planeY, fix16_cos(-rotSpeed)));

        // ---------
        
        raycast();
        Serial.print("frame: "); Serial.println(millis() - curtime);
        uptime = curtime + 150;
    }
}

