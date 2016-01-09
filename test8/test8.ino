#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "wall_assets.h"

namespace {

enum
{
    SCREEN_WIDTH = 200,
    SCREEN_HEIGHT = 100,
    WORLD_SIZE = 8,
    WORLD_MAX_LENGTH = static_cast<int>(sqrt(WORLD_SIZE*WORLD_SIZE + WORLD_SIZE*WORLD_SIZE) + 0.5),
    TEXTURE_SIZE = 64,
    TEXTURE_COUNT = 8
};

typedef float Real;

struct Vec2D
{
    Real x; Real y;
    
    Vec2D(Real _x, Real _y) : x(_x), y(_y) { }
    Vec2D(void) = default;
    
    Vec2D operator+(const Vec2D &v) const { return Vec2D(x + v.x, y + v.y); }
    Vec2D operator-(const Vec2D &v) const { return Vec2D(x - v.x, y - v.y); }
    Vec2D operator*(const Vec2D &v) const { return Vec2D(x * v.x, y * v.y); }
};

struct
{
    Real angle = 0.5;
    Vec2D pos = Vec2D(1.5, 1.5);
    Vec2D spritePos = Vec2D(2.5, 2.5);
} cfg;

const int worldMap[WORLD_SIZE][WORLD_SIZE] = {
    { 1, 4, 1, 1, 6, 1, 1, 1 },
    { 2, 0, 0, 5, 0, 0, 0, 7 },
    { 1, 1, 0, 1, 0, 1, 1, 1 },
    { 6, 0, 0, 0, 0, 0, 0, 3 },
    { 1, 8, 8, 0, 8, 0, 8, 1 },
    { 2, 2, 0, 0, 8, 8, 7, 1 },
    { 3, 0, 0, 0, 0, 0, 0, 5 },
    { 2, 2, 2, 2, 7, 4, 4, 4 },
};

void setColorFromPos(int c)
{
    if (c == 1)
        GD.ColorRGB(0, 0, 255);
    else if (c == 2)
        GD.ColorRGB(255, 0, 255);
    else if (c == 3)
        GD.ColorRGB(255, 255, 0);
    else if (c == 4)
        GD.ColorRGB(255, 0, 0);
    else if (c == 5)
        GD.ColorRGB(0, 255, 0);
    else if (c == 6)
        GD.ColorRGB(0, 255, 255);
    else if (c == 7)
        GD.ColorRGB(150, 125, 70);
    else if (c == 8)
        GD.ColorRGB(50, 50, 150);
    else
        GD.ColorRGB(0, 0, 0);
}

inline int stencilFromDist(float dist)
{
    return min(static_cast<int>(dist * 255.0 / WORLD_MAX_LENGTH + 0.5), 255);
}

int lastWallTex = -1;
void drawStripe(int x, float dist, int texture, int col)
{
    const int h = static_cast<int>(SCREEN_HEIGHT / dist);
    
    if (lastWallTex != texture)
    {
        // need to set it for every handle (texture), cache to save some GD gfx list memory
        GD.BitmapHandle(texture);
        GD.BitmapSize(NEAREST, BORDER, BORDER, WALL0_WIDTH*2, SCREEN_HEIGHT*2);
        lastWallTex = texture;
    }
    
//     const int y = max(0, (SCREEN_HEIGHT / 2) - (h / 2));
//     h = min(h, SCREEN_HEIGHT - y);
    const int y = (SCREEN_HEIGHT / 2) - (h / 2);
    
    const Real sh = (Real)h / TEXTURE_SIZE;
    
    GD.cmd_loadidentity();
    GD.cmd_translate(F16(0), F16(y*2)); // need to translate: normally images drawn at y<0 are simply discarded completely
    GD.cmd_scale(F16(2.0), F16(sh*2));
    GD.cmd_setmatrix();
    
    const int stencil = stencilFromDist(dist);
    GD.StencilFunc(ALWAYS, stencil, 255);
    
    GD.Vertex2ii(x*2, /*y*/0, texture, col);
    Serial.printf("x/y/h/tex/col: %d/%d/%d/%d/%d\n", x, y, h, texture, col);
}

// #define NO_TEXTURES

void render(void)
{
    const uint32_t begintime = millis();

    GD.ClearStencil(255);
    GD.Clear();
    
    // ceiling
    GD.SaveContext();
    GD.ScissorXY(0, 0);
    GD.ScissorSize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2 / 2);
    GD.ClearColorRGB(RGB(56, 56, 56));
    GD.Clear();
    
    // floor
    GD.ScissorXY(0, SCREEN_HEIGHT*2 / 2 - 1);
//      GD.ScissorSize(SCREEN_WIDTH, SCREEN_HEIGHT / 2);
    GD.ClearColorRGB(RGB(112, 112, 112));
    GD.Clear();
    GD.RestoreContext();
    
#ifdef NO_TEXTURES
    GD.Begin(LINES);
#else
    GD.Begin(BITMAPS);
    GD.AlphaFunc(GREATER, 0); // for stencil: otherwise transparency doesn't work with overlapping gfx
    GD.StencilOp(REPLACE, REPLACE);
#endif
    
    // cast all rays here
    Real sina = sin(cfg.angle);
    Real cosa = cos(cfg.angle);
    Real u = cosa - sina;
    Real v = sina + cosa;
    Real du = 2 * sina / SCREEN_WIDTH;
    Real dv = -2 * cosa / SCREEN_WIDTH;
    
    const int sprxi = static_cast<int>(cfg.spritePos.x);
    const int spryi = static_cast<int>(cfg.spritePos.y);
    Real rayDists[SCREEN_WIDTH];
    bool spritevisible = false;
    
    for (int ray = 0; ray < SCREEN_WIDTH; ++ray, u += du, v += dv) {
        // every time this ray advances 'u' units in x direction,
        // it also advanced 'v' units in y direction
        Real uu = (u < 0) ? -u : u;
        Real vv = (v < 0) ? -v : v;
        Real duu = 1 / uu;
        Real dvv = 1 / vv;
        int stepx = (u < 0) ? -1 : 1;
        int stepy = (v < 0) ? -1 : 1;
        
        // the cell in the map that we need to check
        Real px = cfg.pos.x;
        Real py = cfg.pos.y;
        int mapx = static_cast<int>(px);
        int mapy = static_cast<int>(py);
        
        // the position and texture for the hit
        int texture = 0;
        Real hitdist = 0.1;
        Real texofs = 0;
        bool dark = false;
        
        // first hit at constant x and constant y lines
        Real distx = (u > 0) ? (mapx + 1 - px) * duu : (px - mapx) * duu;
        Real disty = (v > 0) ? (mapy + 1 - py) * dvv : (py - mapy) * dvv;
        
        // loop until we hit something
        while (texture <= 0)
        {
            if (distx > disty)
            {
                // shorter distance to a hit in constant y line
                hitdist = disty;
                disty += dvv;
                mapy += stepy;
                texture = worldMap[mapy][mapx];
                if (texture > 0) {
                    dark = true;
                    if (stepy > 0)
                    {
                        Real ofs = px + u * (mapy - py) / v;
                        texofs = ofs - floor(ofs);
                    }
                    else
                    {
                        Real ofs = px + u * (mapy + 1 - py) / v;
                        texofs = ofs - floor(ofs);
                    }
                }
            }
            else
            {
                // shorter distance to a hit in constant x line
                hitdist = distx;
                distx += duu;
                mapx += stepx;
                texture = worldMap[mapy][mapx];
                if (texture > 0)
                {
                    if (stepx > 0)
                    {
                        Real ofs = py + v * (mapx - px) / u;
                        texofs = ofs - floor(ofs);
                    }
                    else
                    {
                        Real ofs = py + v * (mapx + 1 - px) / u;
                        texofs = ceil(ofs) - ofs;
                    }
                }
            }
            
            if (texture == 0 && !spritevisible && mapx == sprxi && mapy == spryi)
                spritevisible = true;
        }
        
        // get the texture, note that the texture image
        // has two textures horizontally, "normal" vs "dark"
        int col = static_cast<int>((1.0 - texofs) * TEXTURE_SIZE);
        col = constrain(col, 0, TEXTURE_SIZE - 1);
        texture = (texture - 1) % TEXTURE_COUNT;
        /*
        const QRgb *tex = texsrc + TEXTURE_BLOCK * texture * 2 +
            (TEXTURE_SIZE * 2 * col);
        if (dark)
            tex += TEXTURE_SIZE;*/
       
       
#ifdef NO_TEXTURES
        // start from the texture center (horizontally)
        //         int h = static_cast<int>(SCREEN_WIDTH / hitdist / 2);
        const int h = static_cast<int>(SCREEN_HEIGHT / hitdist);

        // start from the screen center (vertically)
        // y1 will go up (decrease), y2 will go down (increase)
        int y1 = SCREEN_HEIGHT / 2;
        
        setColorFromPos(/*texture*/col % 8);
        
        GD.Vertex2ii(ray, max(0, y1-(h/2)));
        GD.Vertex2ii(ray, min(SCREEN_HEIGHT, y1+(h/2)));
#else
        drawStripe(ray, hitdist, texture, col);
#endif
        
        rayDists[ray] = hitdist;
    }
    
    if (spritevisible) // sprite cell is visible (--> sprite itself not necessarily)
    {
        const Real dx = cfg.spritePos.x - cfg.pos.x;
        const Real dy = cfg.spritePos.y - cfg.pos.y;
        const Real dist = sqrt(dx*dx + dy*dy); // sqrt needed?
        const Real sangle = atan2(dy, dx) - cfg.angle;
        const Real size = SCREEN_HEIGHT / (cos(sangle) * dist);
        const Real scalef = size / TEXTURE_SIZE;
        
        const int sx = tan(sangle) * SCREEN_HEIGHT;
        int sw = scalef * SOLDIER_WIDTH;
        const int sh = scalef * SOLDIER_HEIGHT;
        const int sxl = SCREEN_WIDTH/2 - sx - (sw/2);
        const int sxt = (SCREEN_HEIGHT - sh) / 2;

        // is the sprite visible at all? NOTE: spritevisible only took complete cell into account, not sprite size
        if (sxl < SCREEN_WIDTH && (sxl + sw) >= 0)
        {
            lastWallTex = -1; // reset because we're messing with settings
            
            if ((sxl + sw) > SCREEN_WIDTH)
                sw = SCREEN_WIDTH - sxl;
            
            GD.cmd_loadidentity();
            GD.cmd_scale(F16(scalef*2), F16(scalef*2));
            GD.cmd_setmatrix();
            
            GD.StencilOp(KEEP, REPLACE);
            GD.StencilFunc(GREATER, stencilFromDist(dist), 255);

            GD.BitmapHandle(SOLDIER_HANDLE);
            GD.BitmapSize(NEAREST, BORDER, BORDER, sw*2, (SCREEN_HEIGHT - sxt) * 2);
            
            GD.Vertex2f(sxl*2 * 16, sxt*2 * 16);
#if 0
            // sprite may be incompletely visible, we can assume only a left part and/or right part is covered
            
            int left = 0;
            while ((((sxl + left) < 0) || rayDists[(int)sxl + left] <= dist) && left < sw)
                ++left;
            
            //        Q_ASSERT(left < sw);
            
            int csw = sw - left; // corrected width
            while ((((sxl + left + csw) > SCREEN_WIDTH) || rayDists[(sxl + left + csw)-1] <= dist) && csw > 0)
                --csw;

            Serial.printf("sw/sh/sxl/sxt/left/csw/dist/rdist: %d/%d/%d/%d/%d/%d/%f/%f\n", sw, sh, sxl, sxt, left, csw, dist, rayDists[(sxl + left + csw)-1]);
            
            GD.BitmapHandle(SOLDIER_HANDLE);
//             GD.BitmapSource(SOLDIER_OFFSET + (left * 2));
//             GD.BitmapLayout(ARGB1555, SOLDIER_WIDTH * 2, SOLDIER_HEIGHT);
            // subtract y (sxt) from height: if y < 0 then height needs to be more to fully display visible part
            GD.BitmapSize(NEAREST, BORDER, BORDER, csw*2, (SCREEN_HEIGHT - sxt) * 2);
            
            GD.cmd_loadidentity();
            GD.cmd_translate(F16(-left*2), F16(0));
            GD.cmd_scale(F16(scalef*2), F16(scalef*2));
            GD.cmd_setmatrix();
            
            GD.Vertex2f((sxl + left)*2 * 16, sxt*2 * 16);
#endif
        }
    }
    
    GD.finish();
    Serial.print("draw buffer: ");
    Serial.println(GD.rd16(REG_CMD_DL));
    
    GD.swap();
    
    Serial.print("render time: "); Serial.println(millis() - begintime);
}

}

void setup()
{
    GD.begin();
    LOAD_ASSETS();
    Serial.begin(115200);
}

void loop()
{
    static int i = 0;
    if (i++ < 3)
        render();

    if (Serial.available())
    {
        const int ch = Serial.read();
        if (ch == 'a')
            cfg.angle += (0.025 * M_PI);
        else if (ch == 'd')
            cfg.angle -= (0.025 * M_PI);
        else if (ch == 'w' || ch == 's')
        {
            const Real step = (ch == 's') ? -0.25 : 0.25;
            Real dx = cos(cfg.angle) * step;
            Real dy = sin(cfg.angle) * step;
            Vec2D pos = cfg.pos + Vec2D(dx, dy);
            int xi = static_cast<int>(pos.x);
            int yi = static_cast<int>(pos.y);
            if (worldMap[yi][xi] == 0)
                cfg.pos = pos;
        }
        i = 0;
    }
    
    delay(5);
}
