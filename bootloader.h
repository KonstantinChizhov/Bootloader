
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "boot_data.h"

extern unsigned long _stext;  // start text == program
extern unsigned long _etext;  // end program
extern unsigned long _sdata;  // begin initiated data
extern unsigned long _edata;  // end initiated data
extern unsigned long _sbss;   // begin uninitiated data
extern unsigned long _ebss;   // end uninitiated data
extern unsigned long _estack; // top of the stack

namespace Bootloader
{

class BootloaderApp
{
    volatile bool _done = false;
    BootData _bootdata;

public:
    BootloaderApp();

    void InitBootData();
    bool WriteFlash(const uint16_t *data, size_t size, uint32_t address);
    bool EraseFlash(uint32_t page);
    bool RunApplication();
    BootData &GetBootData();
    uint32_t BootStartAddress();
    uint32_t BootStartBootPage();
    bool PageFull(uint32_t page);
    bool PageEmpty(uint32_t page);
    bool IsRegionClear(uint32_t address, size_t size);
    bool FindEntryPoint(uint32_t *sp, uint32_t *entry);
    void SetVectTable(const void *table);
    bool IsDone() { return _done; }
    void Exit() { _done = true; }
};

} // namespace Bootloader
