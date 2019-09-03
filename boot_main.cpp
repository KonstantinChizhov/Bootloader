
#include <dispatcher.h>
#include <sys_tick.h>
#include <watchdog.h>

#include "bootloader.h"
#include "boot_protocol.h"

using namespace Bootloader;

BootloaderApp bootloader;
BootloaderProtocol bootprotocol{bootloader};

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
    Portb::Enable();
    Porta::Enable();
    Portc::Enable();
    Portd::Enable();

    Led::Port::Enable();
    TxPin::Port::Enable();
    RxPin::Port::Enable();
    DePin::Port::Enable();

    Led::SetConfiguration(Led::Port::Out);
    //Led::Set();

    Watchdog::Start(2000);
    bootloader.SetVectTable(&g_pfnVectors[0]);
    bootloader.InitBootData();

    SysTickTimer::Init(1);
    SysTickTimer::EnableInterrupt();
    GetCurrentDispatcher().SetTimerFunc(&GetTickCount);
    Blink();
    

    if (!bootprotocol.Init())
    {
        bootloader.Exit();
    }

    //GetCurrentDispatcher().SetTimer(BootStartTimeout, []() { bootloader.Exit(); });

    while (!bootloader.IsDone())
    {
        GetCurrentDispatcher().Poll();
        Watchdog::Reset();
    }

    bootloader.RunApplication();
    NVIC_SystemReset();
    return 0;
}