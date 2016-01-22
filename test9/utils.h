#ifndef UTILS_H
#define UTILS_H

#include <virtmem.h>
#include <alloc/spiram_alloc.h>

using namespace virtmem;

bool loadGDFile(const char *filename);
bool loadGDFileInVMem(const char *filename, VPtr<uint8_t, SPIRAMVAlloc> ptr);

#endif // UTILS_H
