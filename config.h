#pragma once

#include <usart.h>
#include <iopins.h>

using namespace Mcucpp;
using namespace Mcucpp::IO;

typedef Mcucpp::LpUsart1 BootDevice;
typedef Clock::LpUart1Clock BootDeviceClock;


typedef Pb11 TxPin;
typedef Pb10 RxPin;
typedef NullPin DePin;

typedef Pb7 Led;

constexpr uint8_t BootModbusAddr = 1;

constexpr uint16_t PageBufferSize = 2048;
constexpr uint16_t PageBufferAddr = 1024;
constexpr uint16_t PageMapAddr = 1024;
constexpr uint16_t CommandAddress = 100;
constexpr uint16_t CommandParamsSize = 100;


constexpr uint16_t BootVersion = 1;
constexpr uint32_t EntryPointMarker1 = 0xA5A5A5A5;
constexpr uint32_t EntryPointMarker2 = 0x5A5A5A5A;

constexpr uint32_t BootStartTimeout = 1000;

