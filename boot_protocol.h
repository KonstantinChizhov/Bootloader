#pragma once

#include <stddef.h>
#include <stdint.h>
#include "bootloader.h"
#include <modbus.h>
#include "config.h"

namespace Bootloader
{

class BootloaderProtocol
{
    BootloaderApp &_bootloader;
    Mcucpp::Modbus::ModbusTransportRtu<BootDevice, Led> rtuTransport;
    Mcucpp::Modbus::ModbusSlave modbus{rtuTransport};

    uint8_t rxBuffer[256];
    uint8_t txBuffer[256];
    std::array<uint16_t, (PageBufferSize + 1) / 2> PageBuffer;

    DataChunk rxChunk{rxBuffer, 0, sizeof(rxBuffer)};
    DataChunk txChunk{txBuffer, 0, sizeof(txBuffer)};

public:
    BootloaderProtocol(const BootloaderProtocol&) = delete;
    BootloaderProtocol& operator=(const BootloaderProtocol&) = delete;

    Mcucpp::Modbus::ModbusError WriteHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer);
    Mcucpp::Modbus::ModbusError ReadHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer);

    BootloaderProtocol(BootloaderApp &bootloader);
    bool Init();
};

} // namespace Bootloader
