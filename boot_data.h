#pragma once
#include <stdint.h>

namespace Bootloader
{

enum class BootError : uint16_t
{
    Success,
    ArgumentError,
    NotFlashAddress,
    CrossPageBouns,
    LengthNotAligned,
    PageIsProtected,
    AddrNotAligned,
    RegionIsNotClear,
    WritingError,
    WrongPageNumber,
    EraseError,
    ErrorStoringEntryPoint,
    EntryPointNotFound,
    WrongCommand,
    FailedToDercrypt,
};

struct BootData
{
    uint16_t bootSignature[16];
    uint16_t mcuType[16];
    uint16_t bootVersion;
    uint32_t deviceId[4];

    BootError error;
    uint16_t pageCount;
    uint16_t applicationPageCount; 
    uint16_t totalFlashSizeLo;
    uint16_t totalFlashSizeHi;
};
} // namespace Bootloader