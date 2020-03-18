import os
import sys
from intelhex import IntelHex
import time
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.exceptions import ModbusException

modbusAddr = 1
PageBufferSize = 64
PageBufferAddr = 1024
PageMapAddr = 1024
CommandAddress = 100
CommandParamsSize = 100

ParamPage = 0
ParamOffset = 1
ParamLength = 2
ParamCommand = 3

CommandNone = 0
CommandPageErase = 1
CommandPageWrite = 2
CommandPageRead = 3
CommandRunApplication = 4
CommandReset = 5

modAddr = 1
retries = 3

def WriteRegs(clinet, addr, values):
	rr = None
	for attempt in range(0, retries):
		rr = client.write_registers(addr, values, unit=modAddr)
		if not isinstance(rr, ModbusException):
			return rr
	raise rr

def ReadHoldingRegs(clinet, addr, count):
	rr = None
	for attempt in range(0, retries):
		rr = client.read_holding_registers(addr, count, unit=modAddr)
		if not isinstance(rr, ModbusException):
			return rr
	raise rr

def ReadInputRegs(clinet, addr, count):
	rr = None
	for attempt in range(0, retries):
		rr = client.read_input_registers(addr, count, unit=modAddr)
		if not isinstance(rr, ModbusException):
			return rr
	raise rr

def GetDeviceName(client):
	rr = ReadHoldingRegs(client, 0, 16)
	return ''.join(chr(i) for i in rr.registers)

def GetMcuName(client):
	rr = ReadHoldingRegs(client, 16, 16)
	return ''.join(chr(i) for i in rr.registers)

def GetMcuId(client):
	rr = ReadHoldingRegs(client, 34, 6)
	result = 0
	for i in rr.registers:
		result = (result << 16) | i
	return result

def GetBootVersionCount(client):
	rr = ReadHoldingRegs(client, 32, 1)
	return rr.registers[0]

def WritePage(client, data, page, offset):
	modbusData = []
	for i in range(0, len(data), 2):
		modbusData.append(data[i] + (data[i + 1] << 8))
	
	rr = WriteRegs(client, PageBufferAddr, modbusData)
	data = [page, offset, len(data), CommandPageWrite]
	rr = WriteRegs(client, CommandAddress, data)
	return 0


def ErasePage(client, page):
	data = [page, 0, 0, CommandPageErase]
	rr = WriteRegs(client, CommandAddress, data)
	return 0

def Reset(client):
	data = [0, 0, 0, CommandReset]
	rr = client.write_registers(CommandAddress, data, unit=modAddr)
	rr = client.write_registers(CommandAddress, data, unit=modAddr)
	# ignore errors here, bootlodaer will not responce anyway
	return 0

def GetPageCount(client):
	rr = ReadHoldingRegs(client,43, 2)
	result = rr.registers[0]
	if result < 4 or result > 8*1024:
		raise Exception('Unexpected page count: %d' % result)
	return rr.registers[0], rr.registers[1]

def GetFlashSize(client):
	rr = ReadHoldingRegs(client, 45, count=2)
	result = rr.registers[0] | (rr.registers[1] << 16)
	return result

def GetPageSize(client, page):
	rr = ReadInputRegs(client, page * 4 + 2, 2)
	result = rr.registers[0] | (rr.registers[1] << 16)
	if result < 1024 or result > 256*1024:
		raise Exception('Unexpected page %d size: %d' % (page, result))
	return result


def GetPageAddress(client, page):
	rr = ReadInputRegs(client, page * 4, 2)
	result = rr.registers[0] | (rr.registers[1] << 16)
	if result < 0x08000000 or result > 0x08000000 + 4*1024*1024:
		raise Exception('Unexpected page %d address: %x' % (page, result))
	return result


def BootInit():
	client = ModbusClient(method='rtu', port='COM4', stopbits=1, timeout=1, baudrate=115200)
	client.connect()
	return client


def BootPrettyWritePage(client, data, page, offset):
	print('Writing page #%u (%u bytes)\t' % (page, len(data)))

	error = ErasePage(client, page)
	if error != 0:
		raise Exception('Error erasing page. ErrCode = %s' % error)
	chunkSize = PageBufferSize
	wordsWritten = 0
	for chunk in range(offset, offset + len(data), chunkSize):
		chunkData = data[chunk:chunk+chunkSize]
		print('Writing chunk #%u (%u bytes)\t' % (chunk, len(chunkData)))
		error = WritePage(client, chunkData, page, chunk)
		if error != 0:
			raise Exception('Error writing page. ErrCode = %s' % error)

		wordsWritten += chunkSize
	
	# writing rest of page
	chunkData = data[wordsWritten:-1]
	print('Writing last chunk #%u (%u bytes)\t' % (chunk, len(chunkData)))
	error = WritePage(client, chunkData, page, wordsWritten)
	if error != 0:
		raise Exception('Error writing page. ErrCode = %s' % error)
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

# for i in range(0, pageCount[0]):
# 	pageSize = GetPageSize(client, i)
# 	print ("page %i size = %i" % (i, pageSize))

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

# align data to be written on 8 byte boundary
align = (8 - (len(pageData) & 7)) & 7
for i in range(align):
	pageData.append(0)

# write last page
BootPrettyWritePage(client, pageData, page, 0)
print("Success")
print("Reseting system")
Reset(client)
