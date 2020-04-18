#include <flash.h>
#include <usart.h>
#include <iopins.h>
#include <Timers.h>
#include <binary_stream.h>
#include <dispatcher.h>
#include <sys_tick.h>
#include <watchdog.h>

#include <array>
#include "bootloader.h"
#include "boot_protocol.h"

using namespace Mcucpp::Modbus;
using namespace Bootloader;

BootloaderProtocol::BootloaderProtocol(BootloaderApp &bootloader)
    : _bootloader{bootloader}
{
}
using namespace std::placeholders;

bool BootloaderProtocol::Init()
{
    Clock::HsiClock::Enable();
    BootDeviceClock::SelectClockSource(Clock::UsartClockSrc::Hsi);

    rtuTransport.SetStaticBuffers(&rxChunk, &txChunk);
    BootDevice::Init(115200, BootDevice::Default | BootDevice::TwoStopBits);
    BootDevice::SelectTxRxPins<TxPin, RxPin>();
    BootDevice::SetRxTimeout(20);

    BootDevice::SelectDePin<DePin>();

    modbus.SetAddress(BootModbusAddr);
    modbus.OnWriteHoldingRegs = [this](uint16_t s, uint16_t c, DataBuffer &b) { return WriteHoldingRegisters(s, c, b); };
    modbus.OnReadHoldingRegs = [this](uint16_t s, uint16_t c, DataBuffer &b) { return ReadHoldingRegisters(s, c, b); };
    modbus.OnReadInputRegisters = [this](uint16_t s, uint16_t c, DataBuffer &b) { return ReadInputRegisters(s, c, b); };

    if (!rtuTransport.StartListen())
    {
        return false;
    }
    return true;
}

uint16_t BootloaderProtocol::GetPageMapItem(uint16_t index)
{
    PageProps prop = (PageProps)(index % PageMapEntrySize);
    uint16_t page = index / PageMapEntrySize;

    if (page >= Flash::PageCount())
    {
        return 0;
    }
    if (prop == PageProps::AddressLo)
    {
        return (uint16_t)(Flash::PageAddress(page) & 0xffff);
    }
    if (prop == PageProps::AddressHi)
    {
        return (uint16_t)((Flash::PageAddress(page) >> 16) & 0xffff);
    }
    if (prop == PageProps::SizeLo)
    {
        return (uint16_t)(Flash::PageSize(page) & 0xffff);
    }
    if (prop == PageProps::SizeHi)
    {
        return (uint16_t)((Flash::PageSize(page) >> 16) & 0xffff);
    }

    return 0xffff;
}

ModbusError BootloaderProtocol::ReadInputRegisters(uint16_t start, uint16_t count, DataBuffer &buffer)
{
    _bootloader.Connected();
    uint16_t end = std::min<uint16_t>(start + count, Flash::PageCount() * PageMapEntrySize);
    for (uint16_t reg = start; reg < end; reg++)
    {
        buffer.WriteU16Be(GetPageMapItem(reg));
    }
    return ModbusError::NoError;
}

ModbusError BootloaderProtocol::ReadHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer)
{
    _bootloader.Connected();
    uint16_t bootDataEnd = std::min<uint16_t>(start + count, sizeof(BootData) / 2);
    uint16_t *bootDataPtr = reinterpret_cast<uint16_t *>(&_bootloader.GetBootData());
    for (uint16_t reg = start; reg < bootDataEnd; reg++)
    {
        buffer.WriteU16Be(bootDataPtr[reg]);
    }

    return ModbusError::NoError;
}

ModbusError BootloaderProtocol::WriteHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer)
{
    _bootloader.Connected();
    uint16_t endReg = start + count;
#if defined(_DEBUG) && _DEBUG
    cout << setw(5) << start << setw(5) << count;
    if(count < 20)
    {
        cout << "{";
        for(auto d : buffer)
            cout << setw(5) << d;
        cout << "}";
    }
    cout << "\r\n";
#endif
    if ((start >= CommandAddress && start < CommandAddress + CommandParamsSize) || (endReg >= CommandAddress && endReg < CommandAddress + CommandParamsSize))
    {
        uint16_t startInRange = (uint16_t)std::max<int>(start - CommandAddress, 0);
        uint16_t countInRange = std::min<uint16_t>(count, CommandParamsSize - startInRange);
        return WriteCommand(startInRange, countInRange, buffer);
    }

    if ((start >= PageBufferAddr && start < PageBufferAddr + PageBuffer.size()) || (endReg >= PageBufferAddr && endReg < PageBufferAddr + PageBuffer.size()))
    {
        uint16_t startInRange = (uint16_t)std::max<int>(start - PageBufferAddr, 0);
        uint16_t countInRange = std::min<uint16_t>(count, PageBuffer.size() - startInRange);
        return WriteBuffer(startInRange, countInRange, buffer);
    }

    return ModbusError::IllegalAddress;
}

ModbusError BootloaderProtocol::WriteBuffer(uint16_t start, uint16_t count, DataBuffer &buffer)
{
    uint16_t endReg = std::min<uint16_t>(start + count, PageBuffer.size());
    for (uint16_t reg = start; reg < endReg; reg++)
    {
        PageBuffer[reg] = buffer.ReadU16Be();
    }
    return ModbusError::NoError;
}

ModbusError BootloaderProtocol::WriteCommand(uint16_t start, uint16_t count, DataBuffer &buffer)
{
    uint16_t endReg = std::min<uint16_t>(start + count, CommandParamsSize);
    for (uint16_t reg = start; reg < endReg; reg++)
    {
        uint16_t value = buffer.ReadU16Be();
        CommandLayout field = (CommandLayout)reg;
        if (field == CommandLayout::Page)
        {
            if (value >= _bootloader.BootStartBootPage())
            {
                return ModbusError::IllegalValue;
            }
            _commandData.page = value;
        }
        if (field == CommandLayout::Offset)
        {
            if (value >= Flash::PageSize(_commandData.page))
            {
                return ModbusError::IllegalValue;
            }
            _commandData.offset = value;
        }
        if (field == CommandLayout::Length)
        {
            if (value > Flash::PageSize(_commandData.page) - _commandData.offset)
            {
                return ModbusError::IllegalValue;
            }
            _commandData.length = value;
        }
        if (field == CommandLayout::Command)
        {
            if (!ExecuteCommand(static_cast<BootCommand>(value)))
            {
                return ModbusError::NotAcknowledge;
            }
        }
    }

    return ModbusError::NoError;
}

bool BootloaderProtocol::ExecuteCommand(BootCommand command)
{
    _bootloader.GetBootData().error = BootError::Success;
    switch (command)
    {
    case BootCommand::PageErase:
        return _bootloader.EraseFlash(_commandData.page);
    case BootCommand::PageWrite:
        return _bootloader.WriteFlash(PageBuffer.data(), _commandData.page, _commandData.length, _commandData.offset);
    case BootCommand::RunApplication:
        return _bootloader.RunApplication();
    case BootCommand::Reset:
        _bootloader.Reset();
    case BootCommand::PageRead:
    case BootCommand::None:
        break;
    default:
        _bootloader.GetBootData().error = BootError::WrongCommand;
        return false;
    }
    return true;
}