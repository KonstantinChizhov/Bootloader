import os
import sys
from intelhex import IntelHex
import time
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.exceptions import ModbusException

modbusAddr = 1
PageBufferSize = 2048
PageBufferAddr = 1024
PageMapAddr = 1024
CommandAddress = 100
CommandParamsSize = 100

def GetDeviceName(client):
	rr = client.read_holding_registers(0, 16, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	return ''.join(chr(i) for i in rr.registers)

def GetMcuName(client):
	rr = client.read_holding_registers(16, 16, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	return ''.join(chr(i) for i in rr.registers)

def AppendInt(a, b):
	return 

def GetMcuId(client):
	rr = client.read_holding_registers(34, 6, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	result = 0
	for i in rr.registers:
		result = (result << 16) | i
	return result

def GetBootVersionCount(client):
	rr = client.read_holding_registers(32, 1, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	return rr.registers[0]

def WritePage(client, data, page, offset):
	return 0


def ErasePage(client, page):
	return 0


def GetPageCount(client):
	rr = client.read_holding_registers(43, 2, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	result = rr.registers[0]
	if result < 4 or result > 8*1024:
		raise Exception('Unexpected page count: %d' % result)
	return rr.registers[0], rr.registers[1]

def GetFlashSize(client):
	rr = client.read_input_registers(45, 2, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	result = rr.registers[0] | (rr.registers[1] << 16)
	return result

def GetPageSize(client, page):
	rr = client.read_input_registers(page * 4 + 2, 2, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	result = rr.registers[0] | (rr.registers[1] << 16)
	if result < 1024 or result > 256*1024:
		raise Exception('Unexpected page %d size: %d' % (page, result))
	return result


def GetPageAddress(client, page):
	rr = client.read_input_registers(page * 4, 2, unit=1)
	if isinstance(rr, ModbusException):
		raise rr
	result = rr.registers[0] | (rr.registers[1] << 16)
	if result < 0x08000000 or result > 0x08000000 + 4*1024*1024:
		raise Exception('Unexpected page %d address: %x' % (page, result))
	return result


def BootInit():
	client = ModbusClient(method='rtu', port='COM7', stopbits=1, timeout=1, baudrate=115200)
	client.connect()
	return client


def BootPrettyWritePage(client, data, page, offset):
	print('Writing page #%u (%u bytes)\t' % (page, len(pageData)))

	error = ErasePage(client, page)
	if error != 0:
		raise Exception('Error erasing page. ErrCode = 0x%04x' % error)
	error = WritePage(client, data, page, offset)
	if error != 0:
		raise Exception('Error writing page. ErrCode = 0x%04x' % error)
	print('OK')



if len(sys.argv) <= 1:
	raise Exception('Parameter expected.')

firmwareFile = sys.argv[1]
print('Reading target file: "%s"' % firmwareFile)
if os.path.isfile(firmwareFile):
	ih = IntelHex(firmwareFile)
else:
	raise Exception('Target file is not exists "%s"' % firmwareFile)

hexItems = ih.todict()

print('Start address: 0x%08x' % ih.minaddr())
print('End address: 0x%08x' % ih.maxaddr())
print('Total code size: %u bytes' % (len(hexItems)-1))
client = BootInit()

print('Device name: %s' % GetDeviceName(client))
print('CPU name: %s' % GetMcuName(client))
print('MCU ID: %x' % GetMcuId(client))
print('Bootloader version: %d' % GetBootVersionCount(client))
pageCount = GetPageCount(client)
print ('Flash page count: %u (%u)' % pageCount)
print ('Total Flash size: %u' % (GetFlashSize(client) ))

page = 0
pageSize = GetPageSize(client, page)
pageEnd = GetPageAddress(client, page) + pageSize
pageData = []

for addr, value in hexItems.items():
	if not isinstance(addr, int):
		continue
	while addr >= pageEnd:
		if len(pageData) > 0:
			BootPrettyWritePage(client, pageData, page, 0)
		page += 1
		pageSize = GetPageSize(client, page)
		pageEnd = GetPageAddress(client, page) + pageSize
		pageData = []
	pageData.append(value)

align = (8 - (len(pageData) & 7)) & 7
for i in range(align):
	pageData.append(0)

# write last page
BootPrettyWritePage(client, pageData, page, 0)


