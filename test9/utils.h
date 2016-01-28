#ifndef UTILS_H
#define UTILS_H

#include <virtmem.h>
#include <alloc/spiram_alloc.h>

#include "defs.h"

using namespace virtmem;

bool loadGDFile(const char *filename);
bool loadGDFileInVMem(const char *filename, VPtr<uint8_t, SPIRAMVAlloc> ptr);
uint8_t getEntityDrawRotation(Real plang, Real enang, int16_t ex);

#endif // UTILS_H
