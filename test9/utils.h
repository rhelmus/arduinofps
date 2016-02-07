#ifndef UTILS_H
#define UTILS_H

#include <virtmem.h>
#include <alloc/spiram_alloc.h>

#include "defs.h"

using namespace virtmem;

struct Vec2D
{
    Real x; Real y;

    Vec2D(Real _x, Real _y) : x(_x), y(_y) { }
    Vec2D(void) = default;

    Vec2D operator+(const Vec2D &v) const { return Vec2D(x + v.x, y + v.y); }
    Vec2D operator-(const Vec2D &v) const { return Vec2D(x - v.x, y - v.y); }
    Vec2D operator*(const Vec2D &v) const { return Vec2D(x * v.x, y * v.y); }
};

bool loadGDFileInVMem(const char *filename, VPtr<uint8_t, SPIRAMVAlloc> ptr);
uint8_t getEntityDrawRotation(Real plang, Real enang, int16_t ex);

#endif // UTILS_H
