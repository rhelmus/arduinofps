#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

#include <stdint.h>

struct Vec2D
{
    Real x; Real y;

    Vec2D(Real _x, Real _y) : x(_x), y(_y) { }
    Vec2D(void) = default;

    Vec2D operator+(const Vec2D &v) const { return Vec2D(x + v.x, y + v.y); }
    Vec2D operator-(const Vec2D &v) const { return Vec2D(x - v.x, y - v.y); }
    Vec2D operator*(const Vec2D &v) const { return Vec2D(x * v.x, y * v.y); }
};

uint8_t getEntityDrawRotation(Real plang, Real enang, int16_t ex);

#ifdef ARDUINO
#define debugf Serial.printf
#define getMillis millis
#else
#include <stdio.h>
#include <QDateTime>
#define debugf printf
#define getMillis QDateTime::currentMSecsSinceEpoch
#endif


#endif // UTILS_H
