
#include <iopins.h>
#include <dispatcher.h>
#include <sys_tick.h>
#include <watchdog.h>

using namespace Mcucpp;
using namespace Mcucpp::IO;
using namespace Mcucpp::Clock;

#if defined(STM32F40_41xxx)

typedef Pd13 Led;

#elif defined(stm32l471)

typedef Pb7 Led;

#endif

void Blink()
{
    Led::Toggle();
    GetCurrentDispatcher().SetTimer(1000, Blink);
}

int main()
{
    Led::Port::Enable();
    Led::SetConfiguration(Led::Port::Out);
    Led::Set();
    Watchdog::Start(2000);
    SysTickTimer::Init(1);
    SysTickTimer::EnableInterrupt();
    GetCurrentDispatcher().SetTimerFunc(&GetTickCount);
    Blink();

    while (true)
    {
        GetCurrentDispatcher().Poll();
        Watchdog::Reset();
    }
    return 0;
}