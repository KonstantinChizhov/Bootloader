# coding=utf-8
import os
import sys
from datetime import date
import copy
import binascii

Import('*')
if not 'MCUCPP_PATH' in globals():
    MCUCPP_PATH = '#/../Mcucpp'

if 'device' in ARGUMENTS:
    deviceName = ARGUMENTS['device']

if 'key' in ARGUMENTS:
    key = binascii.unhexlify(ARGUMENTS['key'])
    keyStr = ', '.join(['0x%x' % b for b in key ])
    print (keyStr)
else:
    raise Exception('No AES key was provided in command line')

sys.path.insert(1, Dir(MCUCPP_PATH + '/scons').srcnode().abspath)

import devices

device = copy.deepcopy(devices.SupportedDevices[deviceName])
deviceCopy = copy.deepcopy(device)

linkerScripts = {'stm32f407': r'linker_scripts\stm32_40x.ld',
                 'stm32f429':  r'linker_scripts\stm32_40x.ld',
                 'stm32f100': r'linker_scripts\stm32_100xB.ld',
                 'stm32f103': r'linker_scripts\stm32_103xB.ld',
                 'stm32l471': r'linker_scripts\stm32_471.ld'}

device['linkerScript'] = linkerScripts[deviceName]

env = Environment(DEVICE=device,
                  toolpath=['%s/scons' % MCUCPP_PATH],
                  tools=['mcucpp'])

testEnv = Environment(DEVICE=deviceCopy,
                  toolpath=['%s/scons' % MCUCPP_PATH],
                  tools=['mcucpp'])

env['CUSTOM_HEX_PARAMS'] = '--only-section .isr_vectors_orig'

env.Append(CPPDEFINES={
    'BUILD_YEAR': date.today().year,
    'BUILD_MONTH': date.today().month,
    'BUILD_DAY': date.today().day,
    'AES_KEY' : keyStr},
    )

env.Append(CPPDEFINES={'_DEBUG': 0})

env.Append(CPPPATH=['./tiny-AES-c'])

#env.Append(LINKFLAGS = ["-nostdlib"])
env.Append(CCFLAGS=["-Os"])
testEnv.Append(CCFLAGS=["-Os"])

bootTargets = []


def BuildBootloader(envBoot, testEnv, suffix):

    bootloader = envBoot.Object('bootloader%s' % suffix, './bootloader.cpp')
    protocol = envBoot.Object('boot_protocol%s' % suffix, './boot_protocol.cpp')
    main = envBoot.Object('boot_main%s' % suffix, './boot_main.cpp')
    crypt = envBoot.Object('boot_crypt%s' % suffix, './Encryption.cpp')
    aes = envBoot.Object('boot_aes%s' % suffix, './tiny-AES-c/aes.c')

    bootloader_objects = [bootloader, protocol, main, crypt, aes]
    elfBootloader = envBoot.Program(
        'Bootloader%s' % suffix, bootloader_objects)
    bootLss = envBoot.Disassembly(elfBootloader)
    bootHex = envBoot.Hex(elfBootloader)

    #BootSize = envBoot.Size(elfBootloader, 'BootSize')
    testAppObj = testEnv.Object('test_app_%s' % suffix, './test_app.cpp')
    elfTestApp = testEnv.Program('test_app_%s' % suffix, testAppObj)
    testAppLss = testEnv.Disassembly(elfTestApp)
    testAppHex = testEnv.Hex(elfTestApp)

    bootTargets.extend([elfBootloader, bootLss, bootHex, elfTestApp, testAppLss, testAppHex])

    # if hasattr(envBoot.__class__, 'Flash') and callable(getattr(envBoot.__class__, 'Flash')):
    flash = envBoot.Flash(bootHex)
    protect = envBoot.Protect(bootHex)
    # flash = testEnv.Flash(elfTestApp)
    bootTargets.extend([flash, protect])
       

BuildBootloader(env, testEnv, deviceName)

env.Alias('Bootloader', bootTargets)
