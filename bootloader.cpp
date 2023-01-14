
#include <flash.h>
#include <Storage/NvmStorage.h>
#include <usart.h>
#include <iopins.h>

#include <array>
#include "bootloader.h"
#include "boot_protocol.h"
#include <cstring>
#include "Encryption.h"

extern "C" void Reset_Handler();

using namespace Bootloader;

const uint32_t nvmPage = Flash::AddrToPage((void *)&_nvmStart);
const uint32_t nvmPageCount = ((uint8_t *)&_nvmEnd - (uint8_t *)&_nvmStart) / Flash::PageSize(nvmPage);
Storage::NvmStorage<AppEntryPoint> entryPointStorage(nvmPage, nvmPageCount);

BootloaderApp::BootloaderApp()
	: _bootdata{
		  bootSignature : {'R', 'U', 'B', 'I', 'N', ' ', 'B', 'O', 'O', 'T'},
		  mcuType : MCU_NAME,

		  bootVersion : BootVersion,
		  deviceId : {0, 0, 0, 0},
		  error : BootError::Success,
		  pageCount : 0,
		  applicationPageCount : 0,
		  totalFlashSizeLo : 0,
		  totalFlashSizeHi : 0
	  }
{
}

bool BootloaderApp::ReplaceAndStoreAppEntryPoint(uint16_t *data)
{
	uint32_t *ptr = reinterpret_cast<uint32_t *>(data);
	AppEntryPoint entryPointData{
		version : 0,
		appStackPointer : ptr[0],
		appEntryAddr : ptr[1],
		reserved : 0xffffffff
	};
	if (!entryPointStorage.Write(entryPointData))
	{
		_bootdata.error = BootError::ErrorStoringEntryPoint;
		return false;
	}

	ptr[0] = (uint32_t)&_estack;
	ptr[1] = (uint32_t)&Reset_Handler;
	return true;
}

bool BootloaderApp::WriteFlash(uint16_t *data, uint16_t page, uint16_t size, uint16_t offset)
{

#if defined(_DEBUG) && _DEBUG
	cout << "Write: " << hex << setw(8) << page << setw(8) << size << setw(8) << offset << "\r\n";
#endif
	if (size < 8)
	{
		_bootdata.error = BootError::ArgumentError;
		return false;
	}

	if (offset + size > Flash::PageSize(page))
	{
		_bootdata.error = BootError::WrongPageNumber;
		return false;
	}
	if (offset & 0x7) // dest should be aligned with a double word address
	{
		_bootdata.error = BootError::AddrNotAligned;
		return false;
	}
	if (size & 0x7) // length multiple of 8
	{
		_bootdata.error = BootError::LengthNotAligned;
		return false;
	}
	if (page >= BootStartBootPage())
	{
		_bootdata.error = BootError::PageIsProtected;
		return false;
	}

	size_t outputSize = 0;
	if (!DecryptPage((const uint8_t *)data, size, (uint8_t *)_pageData, &outputSize))
	{
		_bootdata.error = BootError::FailedToDercrypt;
		return false;
	}

	uint32_t address = Flash::PageAddress(page) + offset;
	if (!IsRegionClear(address, size))
	{
		if (std::memcmp((void *)address, _pageData, outputSize) == 0)
		{
			// re-trying to write same block
			_bootdata.error = BootError::Success;
			return true;
		}
		_bootdata.error = BootError::RegionIsNotClear;
		return false;
	}

	if (page == 0 && offset == 0)
	{
		if (!ReplaceAndStoreAppEntryPoint(_pageData))
		{
			return false;
		}
	}

	if (!Flash::WritePage(page, _pageData, outputSize, offset))
	{
		_bootdata.error = BootError::WritingError;
		return false;
	}
	return true;
}

bool BootloaderApp::EraseFlash(uint32_t page)
{
	if (page >= BootStartBootPage())
	{
		_bootdata.error = BootError::PageIsProtected;
		return false;
	}
	return Flash::ErasePage(page);
}

BootData &BootloaderApp::GetBootData()
{
	return _bootdata;
}

void BootloaderApp::InitBootData()
{
#if defined(UID_BASE)
	uint32_t *uid = reinterpret_cast<uint32_t *>(UID_BASE);
#else
	uint32_t *uid = reinterpret_cast<uint32_t *>(0x1FFF7A10);
#endif
	_bootdata.deviceId[0] = uid[0];
	_bootdata.deviceId[1] = uid[1];
	_bootdata.deviceId[2] = uid[2];
	_bootdata.deviceId[3] = 0;

	_bootdata.pageCount = Flash::PageCount();
	_bootdata.applicationPageCount = BootStartBootPage();
	_bootdata.totalFlashSizeLo = (uint16_t)(Flash::TotalSize() & 0xffff);
	_bootdata.totalFlashSizeHi = (uint16_t)((Flash::TotalSize() >> 16) & 0xffff);
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
	unsigned size = Flash::PageSize(page) / sizeof(unsigned);
	return (ptr[0] != 0xffffffff) && (ptr[size - 1] != 0xffffffff);
}

bool BootloaderApp::PageEmpty(uint32_t page)
{
	unsigned *ptr = (unsigned *)Flash::PageAddress(page);
	unsigned size = Flash::PageSize(page) / sizeof(unsigned);
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
	AppEntryPoint entryPoint;
	if (!entryPointStorage.Read(entryPoint))
	{
		_bootdata.error = BootError::EntryPointNotFound;
		return false;
	}
	if(entryPoint.appEntryAddr == 0 || entryPoint.appEntryAddr == 0xffffffff)
	{
		_bootdata.error = BootError::EntryPointNotFound;
		return false;
	}
#if defined(_DEBUG) && _DEBUG
	cout << "Entry point found: 0x" << hex << entryPoint.appEntryAddr << "\r\n";
#endif
	*sp = entryPoint.appStackPointer;
	*entry = entryPoint.appEntryAddr;
	return true;
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

void BootloaderApp::Reset()
{
	NVIC_SystemReset();
}
