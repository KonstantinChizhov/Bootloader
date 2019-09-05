#pragma once

#include <stddef.h>
#include <stdint.h>
#include "bootloader.h"
#include <modbus.h>
#include "config.h"

namespace Bootloader
{

enum class PageProps
{
    AddressLo,
    AddressHi,
    SizeLo,
    SizeHi,
};

enum class CommandLayout :uint16_t
{
    Page = 0,
    Offset = 1,
    Length = 2,
    Command = 3,
};

enum class BootCommand : uint16_t
{
    None,
    PageErase,
    PageWrite,
    PageRead,
    RunApplication,
};

using Mcucpp::Modbus::ModbusError;

struct CommandData
{
    uint16_t page = 0;
    uint16_t offset = 0;
    uint16_t length = 0;
};

class BootloaderProtocol
{
    BootloaderApp &_bootloader;
    CommandData _commandData;

    Mcucpp::Modbus::ModbusTransportRtu<BootDevice, Led> rtuTransport;
    Mcucpp::Modbus::ModbusSlave modbus{rtuTransport};

    uint8_t rxBuffer[256];
    uint8_t txBuffer[256];
    std::array<uint16_t, (PageBufferSize + 1) / 2> PageBuffer;

    DataChunk rxChunk{rxBuffer, 0, sizeof(rxBuffer)};
    DataChunk txChunk{txBuffer, 0, sizeof(txBuffer)};

public:
    static constexpr uint16_t PageMapEntrySize = 4; // of 16 bit words

    BootloaderProtocol(const BootloaderProtocol &) = delete;
    BootloaderProtocol &operator=(const BootloaderProtocol &) = delete;

    ModbusError WriteHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer);
    ModbusError ReadHoldingRegisters(uint16_t start, uint16_t count, DataBuffer &buffer);
    ModbusError ReadInputRegisters(uint16_t start, uint16_t count, DataBuffer &buffer);

    BootloaderProtocol(BootloaderApp &bootloader);
    bool Init();
    uint16_t GetPageMapItem(uint16_t index);
    ModbusError WriteCommand(uint16_t start, uint16_t count, DataBuffer &buffer);
    ModbusError WriteBuffer(uint16_t start, uint16_t count, DataBuffer &buffer);
    bool ExecuteCommand(BootCommand command);
};

} // namespace Bootloader
