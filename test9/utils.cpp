#include "utils.h"

uint8_t getEntityDrawRotation(Real plang, Real enang, int16_t ex)
{
    // Based on Wolf3d's CalcRotate()

    float sangle = (((plang - enang) * 180 / M_PI) + (SCREEN_WIDTH/2 - ex)/8) - 180;
    sangle += (360 / 16);
    while (sangle > 360) sangle -= 360;
    while (sangle < 0) sangle += 360;

//    debugf("rot: %d\n", (int)(sangle / (360 / 8)));
    return sangle / (360 / 8);
}
