
#include <iopins.h>
#include <dispatcher.h>
#include <sys_tick.h>
#include <watchdog.h>
#include <usart.h>
#include <tiny_ostream.h>
#include <tiny_iomanip.h>

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

#elif defined(STM32L471xx)


typedef Usart3 BootDevice;
typedef Usart3Clock BootDeviceClock;

typedef Pc4 TxPin;
typedef Pc5 RxPin;
typedef Pb1 DePin;
typedef Pb9 Led;

#endif

basic_ostream<BootDevice> cout;

void Blink()
{
    Led::Toggle();
    GetCurrentDispatcher().SetTimer(1000, Blink);
}

struct Bar
{
public:
    unsigned c;
    unsigned d;
};

struct Foo
{
public:
    int a;
    int b;
    Bar bar;
    uint16_t buffer[100];
};

volatile Foo fooInstance{1, 2, {3, 4}, {0}};

int main()
{
    Led::Port::Enable();
    Led::SetConfiguration(Led::Port::Out);
    Led::Set();
    BootDevice::Init(115200);
    BootDevice::SelectTxRxPins<TxPin, RxPin>();
    BootDevice::SelectDePin<DePin>();
    BootDevice::SetRxTimeout(20);

    cout << "Hello!!\r\n";
   
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