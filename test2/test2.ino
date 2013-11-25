#include <SPI.h>
#include <GD.h>

#include "render.h"

#define FRAMEBUFFER RAM_SPRIMG

namespace {

struct SRowData
{
    uint8_t color : 4;
    uint16_t top : 9;
    uint16_t bottom : 9;
};

enum
{
    SCREEN_WIDTH = 320,
    SCREEN_WIDTH_CH = SCREEN_WIDTH / 8,
    SCREEN_HEIGHT = 200,
    SCREEN_HEIGHT_CH = SCREEN_HEIGHT / 8
};

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

void dumpCharCol(uint8_t chcol, struct SRowData *rowdata)
{
    static uint16_t chrawdata[8];
    uint8_t mintop = 255, maxbottom = 0;
    
    for (uint8_t i=0; i<8; ++i)
    {
        mintop = min(mintop, rowdata[i].top);
        maxbottom = max(maxbottom, rowdata[i].bottom);
    }
    
    const uint8_t minchtop = mintop / 8;
    const uint8_t maxchbottom = maxbottom / 8;
    
    for (uint8_t chrow=minchtop; chrow<=maxchbottom; ++chrow)
    {
#if 0
        bool unicolor = false;
        for (uint8_t chx=0; chx<8; ++chx)
        {
            const uint16_t y = chrow * 8;
            
            unicolor = ((rowdata[chx].top <= y) && (rowdata[chx].bottom >= (y+7)) &&
                        ((chx == 0) || (rowdata[chx-1].color == rowdata[chx].color)));
            
            if (!unicolor)
                break;
        }

        if (unicolor)
        {
            const uint16_t choffset = charOffset(chcol, chrow);
            GD.wr16(RAM_PAL + choffset * 8, getRGBColor(rowdata[0].color-1));
        }
        else
#endif
        {
            const uint16_t chrowy = chrow * 8;
            memset(chrawdata, 0, sizeof(chrawdata));
            
            for (uint8_t chx=0; chx<8; ++chx)
            {
                if (((chrowy+8) <= rowdata[chx].top) || (chrowy > rowdata[chx].bottom))
                    continue;
                
                uint8_t starty = 0, endy = 7;
                
                if ((rowdata[chx].top > chrowy) && (rowdata[chx].top < (chrowy + 8)))
                    starty = rowdata[chx].top - chrowy;
                else if ((rowdata[chx].bottom >= chrowy) && (rowdata[chx].bottom < (chrowy + 8)))
                    endy = rowdata[chx].bottom - chrowy;
                
                if (chx < 4)
                {
                    for (uint8_t chy=starty; chy<=endy; ++chy)
                        chrawdata[chy] |= ((uint16_t)rowdata[chx].color << (6 - (chx*2)));
                }
                else
                {
                    for (uint8_t chy=starty; chy<=endy; ++chy)
                        chrawdata[chy] |= ((uint16_t)rowdata[chx].color << (14-((chx*2)-8)));
                }
            }
            
            GDcopyram(FRAMEBUFFER + (charOffset(chcol, chrow) * 16), (uint8_t *)chrawdata,
                      sizeof(chrawdata));
        }
    }
}


#define mapWidth  24
#define mapHeight 24

// --------

uint8_t worldMap[mapHeight][mapWidth]=
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
        
        rowdata[x].color = constrain(worldMap[mapX][mapY], 1, 3); //1/*w % 3 + 1*/;
        rowdata[x].top = drawStart; //80 - (x / 5);
        rowdata[x].bottom = drawEnd; //127 + (x / 5);

    }
    
    const uint32_t ctime = millis();
    GD.wr(COMM+0, 1); // Clear screen
    while (GD.rd(COMM+0))
        ; // Wait till done
    Serial.print("ctime: "); Serial.println(millis() - ctime, DEC);

    const uint32_t dtime = millis();
    uint16_t chcol = 0;
    for (uint16_t x=7; x<SCREEN_WIDTH; x+=8)
    {
        dumpCharCol(chcol, &rowdata[x-7]);
        ++chcol;
    }

    Serial.print("dtime: "); Serial.println(millis() - dtime, DEC);
}


}


void setup()
{
    Serial.begin(115200);
    
    GD.begin();
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

