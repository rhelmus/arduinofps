#include <SPI.h>
#include <GD.h>

#include "fastspi.h"
#include "render.h"

#include <stdint.h>

#define FRAMEBUFFER RAM_SPRIMG

namespace {

struct SRowData
{
    uint8_t color : 4;
    uint8_t top;
    uint8_t bottom;
    uint8_t texX : 6;
    uint8_t lineHeight;
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
    while (count--) {
        SPI.transfer(*src);
        ++src;
    }
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
void drawScreen(struct SRowData * __restrict__ rowdata)
{
    static uint16_t chpicline[SCREEN_WIDTH];
    uint16_t colorbshift[8];

    // cache this to make to avoid constant recalculations in the loops below
    for (uint8_t i=0; i<8; ++i)
        colorbshift[i] = ((i < 4) ? (6 - (i*2)) : (14-((i*2)-8)));

    for (uint_fast8_t chrow=0; chrow<SCREEN_HEIGHT_CH; ++chrow)
    {
        const uint_fast16_t chrowy = chrow * 8, nextchrowy = chrowy + 8;

        memset(chpicline, 0, sizeof(chpicline));

        for (uint_fast16_t x=0; x<SCREEN_WIDTH; ++x)
        {
            if ((nextchrowy <= rowdata[x].top) || (chrowy > rowdata[x].bottom))
                continue;

            const uint_fast8_t starty = ((rowdata[x].top > chrowy) && (rowdata[x].top < nextchrowy)) ? (rowdata[x].top - chrowy) : 0;
            const uint_fast8_t endy = ((rowdata[x].bottom >= chrowy) && (rowdata[x].bottom < nextchrowy)) ? (rowdata[x].bottom - chrowy) : 7;

#if 0
            if ((rowdata[x].top > chrowy) && (rowdata[x].top < nextchrowy))
                starty = rowdata[x].top - chrowy;
            else if ((rowdata[x].bottom >= chrowy) && (rowdata[x].bottom < nextchrowy))
                endy = rowdata[x].bottom - chrowy;
#endif

            const uint_fast8_t chx = x & 7; // chx mod 8
            const uint_fast16_t picx = x - chx;

#if 0
//            const uint16_t cv = ((chx < 4) ? ((uint16_t)rowdata[x].color << (6 - (chx*2))) :
//                                             ((uint16_t)rowdata[x].color << (14-((chx*2)-8))));

            const uint16_t cv = ((uint16_t)rowdata[x].color << colorbshift[chx]);
            for (uint8_t chy=starty; chy<=endy; ++chy)
                chpicline[picx + chy] |= cv;
#endif

            for (uint_fast8_t chy=starty; chy<=endy; ++chy)
            {
                // 256 and 128 factors to avoid floats

                const uint_fast16_t d = (chrowy + chy) * 256 - SCREEN_HEIGHT * 128 + rowdata[x].lineHeight * 128;
                const uint_fast8_t texY = ((d * TEX_HEIGHT) / rowdata[x].lineHeight) / 256;
                const uint_fast16_t cv = ((uint16_t)texture[TEX_HEIGHT * texY + rowdata[x].texX] << colorbshift[chx]);
//                //make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
//                if(side == 1) color = (color >> 1) & 8355711;
//                const uint16_t cv = ((uint16_t)rowdata[x].color << colorbshift[chx]);

                chpicline[picx + chy] |= cv;
            }
        }

        fastSPI.startTransfer(FRAMEBUFFER + (charOffset(0, chrow) * 16));

        for (uint_fast16_t i=0; i<sizeof(chpicline); ++i)
            SPI_WRITE_8(*((uint8_t *)chpicline + i));

        fastSPI.endTransfer();
    }
}


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

float posX = 22, posY = 12;  //x and y start position
float dirX = -1, dirY = 0; //initial direction vector
float planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane

// --------

void raycast(void) __attribute__((optimize("-O3")));
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
    
//    GD.begin();
    beginGD();
    GD.wr16(BG_COLOR, RGB(0, 0, 0));
    GD.fill(RAM_PIC, 255, 4 * 1024);
    
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
        GD.setpal(4 * i + 1, getRGBColor(0));
        GD.setpal(4 * i + 2, getRGBColor(1));
        GD.setpal(4 * i + 3, getRGBColor(2));
    }

    initTextures();
    
    GD.microcode(render_code, sizeof(render_code));
}


void loop()
{
    static uint32_t uptime = 0;
    const uint32_t curtime = millis();
    
    if (uptime < curtime)
    {
#if 1
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
#endif
        // ---------
        
        raycast();
        Serial.print("frame: "); Serial.println(millis() - curtime);
        uptime = curtime + 150;
    }
}

