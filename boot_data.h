#pragma once
#include <stdint.h>

namespace Bootloader
{

struct BootData
{
    uint16_t bootSignature[16];
    uint16_t mcuType[16];
    uint16_t bootVersion;
    uint32_t deviceId[4];
    // flash memory map
    uint16_t pageCount;
    uint16_t pageSizeMultiplier;
};
} // namespace Bootloader