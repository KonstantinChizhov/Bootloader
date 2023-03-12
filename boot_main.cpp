
#include <dispatcher.h>
#include <sys_tick.h>
#include <watchdog.h>

#include "bootloader.h"
#include "boot_protocol.h"

using namespace Bootloader;

BootloaderApp bootloader;
BootloaderProtocol bootprotocol{bootloader};

#if defined(_DEBUG) && _DEBUG
Mcucpp::basic_ostream<DebugDevice> cout;
#endif
extern "C" void Reset_Handler();

__attribute__((section(".isr_vectors_orig"), used)) void (*const g_pfnVectors_orig[])(void) =
    {
        (void (*)(void))((unsigned long)&_estack),
        Reset_Handler};

extern void (*const g_pfnVectors[])(void);

void Blink()
{
    Led::Toggle();
    GetCurrentDispatcher().SetTimer(100, Blink);
}

int main()
{
    bootloader.SetVectTable(&g_pfnVectors[0]);
    Portb::Enable();
    Porta::Enable();
    Portc::Enable();
    Portd::Enable();

    Porta::SetConfiguration(0xffff, Porta::In);
    Portb::SetConfiguration(0xffff, Portb::In);
    Portc::SetConfiguration(0xffff, Portc::In);
    Portd::SetConfiguration(0xffff, Portd::In);

    Porta::SetPullUp(0xffff, Porta::PullDown);
    Portb::SetPullUp(0xffff, Portb::PullDown);
    Portc::SetPullUp(0xffff, Portc::PullDown);
    Portd::SetPullUp(0xffff, Portd::PullDown);

    Led::Port::Enable();
    TxPin::Port::Enable();
    RxPin::Port::Enable();
    DePin::Port::Enable();

#if defined(_DEBUG) && _DEBUG

    Clock::HsiClock::Enable();
    DebugDeviceClock::Set(UsartClockSrc::Hsi);
    DebugDevice::Init(115200);
    DebugDevice::SelectTxRxPins<DebugTxPin, DebugRxPin>();
    cout << "Bootloader started\r\n";
#endif
    Led::SetConfiguration(Led::Port::Out);
    Led::Set();

    Watchdog::Start(2000);
    bootloader.InitBootData();

    SysTickTimer::Init(1);
    SysTickTimer::EnableInterrupt();
    GetCurrentDispatcher().SetTimerFunc(&GetTickCount);

    if (!bootprotocol.Init())
    {
#if defined(_DEBUG) && _DEBUG
        cout << "Init Failed\r\n";
#endif
        bootloader.Exit();
    }

    Blink();

    GetCurrentDispatcher().SetTimer(BootStartTimeout, []()
                                    { bootloader.Exit(); });
    do
    {
        while (!bootloader.IsDone())
        {
            GetCurrentDispatcher().Poll();
            Watchdog::Reset();
        }
#if defined(_DEBUG) && _DEBUG
        cout << "Try running app\r\n";
#endif
        if(!bootloader.RunApplication())
        {
        #if defined(_DEBUG) && _DEBUG
            cout << "No entry point found\r\n";
        #endif
        }
        bootloader.Connected();
    } while (true);
    
    bootloader.Reset();
    return 0;
}