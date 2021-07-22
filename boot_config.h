#pragma once

#include <usart.h>
#include <iopins.h>
using namespace Mcucpp;
using namespace Mcucpp::IO;
using namespace Mcucpp::Clock;

#if defined(STM32F40_41xxx)

typedef Usart1 BootDevice;
typedef Usart1Clock BootDeviceClock;

typedef Pb6 TxPin;
typedef Pb7 RxPin;
typedef NullPin DePin;
typedef Pd13 Led;
#define MCU_NAME {'S', 'T', 'M', '3', '2', 'F', '4', '0', '7'}

#elif defined(STM32L471xx)

typedef Usart3 BootDevice;
typedef Usart3Clock BootDeviceClock;

typedef Pc4 TxPin;
typedef Pc5 RxPin;
typedef Pb1 DePin;
//typedef Pb14 DePin;

typedef Pb9 Led;
#define MCU_NAME {'S', 'T', 'M', '3', '2', 'L', '4', '7', '1'}

typedef Usart2 DebugDevice;
typedef Usart2Sel DebugDeviceClock;
typedef Pa2 DebugTxPin;
typedef Pa3 DebugRxPin;

#endif
constexpr uint8_t BootModbusAddr = 201;

constexpr uint16_t PageBufferSize = 2048;
constexpr uint16_t PageBufferAddr = 1024;
constexpr uint16_t PageMapAddr = 1024;
constexpr uint16_t CommandAddress = 100;
constexpr uint16_t CommandParamsSize = 100;


constexpr uint16_t BootVersion = 1;
constexpr uint32_t EntryPointMarker1 = 0xA5A5A5A5;
constexpr uint32_t EntryPointMarker2 = 0x5A5A5A5A;

constexpr uint32_t BootStartTimeout = 5 * 1000;

#if defined(_DEBUG) && _DEBUG
#include <tiny_ostream.h>
#include <tiny_iomanip.h>
extern Mcucpp::basic_ostream<DebugDevice> cout;


#endif
