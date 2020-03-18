
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
extern unsigned long _nvmStart;
extern unsigned long _nvmEnd;

namespace Bootloader
{

struct AppEntryPoint
{
    bool operator==(const AppEntryPoint &rhs)
    {
        return appStackPointer == rhs.appStackPointer && appEntryAddr == rhs.appEntryAddr;
    }
    uint32_t version;
    uint32_t appStackPointer;
    uint32_t appEntryAddr;
    uint32_t reserved;
};

class BootloaderApp
{
    volatile bool _done = false;
    volatile bool _connected = false;
    BootData _bootdata;

public:
    BootloaderApp();
    void Reset();
    void Connected() { _connected = true; }
    bool ReplaceAndStoreAppEntryPoint(uint16_t *data);
    void InitBootData();
    bool WriteFlash(uint16_t *data, uint16_t page, uint16_t size, uint16_t offset);
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
    bool IsDone() { return _done && !_connected; }
    void Exit() { _done = true; }
};

} // namespace Bootloader
