
#include <flash.h>
#include <usart.h>
#include <iopins.h>

#include <array>
#include "bootloader.h"
#include "boot_protocol.h"

using namespace Bootloader;

BootloaderApp::BootloaderApp()
	: _bootdata{
		  bootSignature : {'R', 'U', 'B', 'I', 'N', ' ', 'B', 'O', 'O', 'T'},
		  mcuType : {'S', 'T', 'M', '3', '2', 'L', '4', '7', '1'},
		  bootVersion : BootVersion
	  }
{
}

bool BootloaderApp::WriteFlash(const uint16_t *data, size_t size, uint32_t address)
{
	return true;
}

bool BootloaderApp::EraseFlash(uint32_t page)
{
	return true;
}

BootData &BootloaderApp::GetBootData()
{
	return _bootdata;
}

void BootloaderApp::InitBootData()
{
	_bootdata.pageCount = Flash::PageCount();
	uint32_t *uid = reinterpret_cast<uint32_t *>(UID_BASE);
	_bootdata.deviceId[0] = uid[0];
	_bootdata.deviceId[1] = uid[1];
	_bootdata.deviceId[2] = uid[2];
	_bootdata.deviceId[0] = 0;
	_bootdata.pageSizeMultiplier = 16;
}

uint32_t BootloaderApp::BootStartAddress()
{
	return (unsigned)&_stext;
}

uint32_t BootloaderApp::BootStartBootPage()
{
	return Flash::AddrToPage(&_stext);
}

bool BootloaderApp::PageFull(uint32_t page)
{
	unsigned *ptr = (unsigned *)Flash::PageAddress(page);
	unsigned size = Flash::PageSize(page) / 4;
	return (ptr[0] != 0xffffffff) && (ptr[size - 1] != 0xffffffff);
}

bool BootloaderApp::PageEmpty(uint32_t page)
{
	unsigned *ptr = (unsigned *)Flash::PageAddress(page);
	unsigned size = Flash::PageSize(page) / 4;
	return (ptr[0] == 0xffffffff) && (ptr[size - 1] == 0xffffffff);
}

bool BootloaderApp::IsRegionClear(uint32_t address, size_t size)
{
	uint8_t *end = (uint8_t *)(address + size);
	for (uint8_t *ptr = (uint8_t *)address; ptr < end; ptr++)
	{
		if (*ptr != 0xff)
			return 0;
	}
	return 1;
}

bool BootloaderApp::FindEntryPoint(uint32_t *sp, uint32_t *entry)
{
	for (unsigned page = 0; page < Flash::PageCount(); page++)
	{
		unsigned addr = Flash::PageAddress(page);
		unsigned size = Flash::PageSize(page) / 4; // size in dwords
		volatile unsigned *ptr = (unsigned *)addr;

		// если страница не полная и не пустая - она может содержать точку входа
		if (!PageFull(page) && !PageEmpty(page))
		{
			// линейный поиск в частично заполненой странице
			for (unsigned i = 0; i < size; i++)
			{
				if (ptr[i] == EntryPointMarker1)
				{
					if (ptr[i + 1] == EntryPointMarker2)
					{
						*sp = ptr[i + 2];
						*entry = ptr[i + 3];
						return true;
					}
				}
			}
		}
		// особый случай оба маркера в самом конце полной страницы
		else if (ptr[size - 4] == EntryPointMarker1 && ptr[size - 3] == EntryPointMarker2)
		{
			*sp = ptr[size - 2];
			*entry = ptr[size - 1];
			return true;
		}
	}
	return false;
}

void BootloaderApp::SetVectTable(const void *table)
{
	__disable_irq();
	SCB->VTOR = reinterpret_cast<uint32_t>(table);
	__DSB();
	__enable_irq();
}

typedef void (*AppEntryT)();
volatile AppEntryT AppEntry;

bool BootloaderApp::RunApplication()
{
	uint32_t sp = 0xffffffff, entry = 0xffffffff;
	if (FindEntryPoint(&sp, &entry))
	{
		__disable_irq();
		AppEntry = (AppEntryT)(entry);

		BootDeviceClock::Reset();
		Clock::SysClock::SelectClockSource(Clock::SysClockSource::Internal);
		__set_MSP(sp);
		SetVectTable((const void *)FLASH_BASE);
		__enable_irq();
		AppEntry();
	}
	return false;
}
