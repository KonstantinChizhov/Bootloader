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
    BootDevice::Init(115200);
    BootDevice::SelectTxRxPins<TxPin, RxPin>();
    BootDevice::SetRxTimeout(20);

    //BootDevice::SelectDePin<DePin>();

    modbus.SetAddress(BootModbusAddr);
    modbus.OnWriteHoldingRegs = [this] (uint16_t s, uint16_t c, DataBuffer &b){ return WriteHoldingRegisters(s, c, b);};
    modbus.OnReadHoldingRegs = [this] (uint16_t s, uint16_t c, DataBuffer &b){ return ReadHoldingRegisters(s, c, b);};

    if (!rtuTransport.StartListen())
    {
        return false;
    }
    return true;
}

ModbusError BootloaderProtocol::ReadHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer)
{
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
    uint16_t endReg = std::min<uint16_t>(start + count, PageBuffer.size());

    for (uint16_t reg = start; reg < endReg; reg++)
    {
        PageBuffer[reg] = buffer.ReadU16Be();
    }

    return ModbusError::NoError;
}