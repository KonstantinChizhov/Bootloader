# coding=utf-8
import os
import sys
from datetime import date

Import('*')
if not 'MCUCPP_PATH' in globals():
    MCUCPP_PATH = '#/../Mcucpp'

if 'device' in ARGUMENTS:
    deviceName = ARGUMENTS['device']

sys.path.insert(1, Dir(MCUCPP_PATH + '/scons').srcnode().abspath)

import devices

device = devices.SupportedDevices[deviceName]
linkerScripts = {'stm32f407': r'linker_scripts\stm32_40x.ld',
                 'stm32f429':  r'linker_scripts\stm32_40x.ld',
                 'stm32f100': r'linker_scripts\stm32_100xB.ld',
                 'stm32f103': r'linker_scripts\stm32_103xB.ld',
                 'stm32l471': r'linker_scripts\stm32_471.ld'}

device['linkerScript'] = linkerScripts[deviceName]

env = Environment(DEVICE=device,
                  toolpath=['%s/scons' % MCUCPP_PATH],
                  tools=['mcucpp'])

env['CUSTOM_HEX_PARAMS'] = '--only-section .isr_vectors_orig'

env.Append(CPPDEFINES={
    'BUILD_YEAR': date.today().year,
    'BUILD_MONTH': date.today().month,
    'BUILD_DAY': date.today().day})

#env.Append(LINKFLAGS = ["-nostdlib"])
env.Append(CCFLAGS=["-Os"])

bootTargets = []


def BuildBootloader(envBoot, suffix):

    bootloader = envBoot.Object('bootloader%s' % suffix, '#/./bootloader.cpp')
    protocol = envBoot.Object('boot_protocol%s' % suffix, '#/./boot_protocol.cpp')
    main = envBoot.Object('boot_main%s' % suffix, '#/./boot_main.cpp')

    bootloader_objects = [bootloader, protocol, main]
    elfBootloader = envBoot.Program(
        'Bootloader%s' % suffix, bootloader_objects)
    bootLss = envBoot.Disassembly(elfBootloader)
    bootHex = envBoot.Hex(elfBootloader)
    flash = envBoot.Flash(bootHex)
    #BootSize = envBoot.Size(elfBootloader, 'BootSize')
    bootTargets.extend([elfBootloader, bootLss, bootHex, flash])


BuildBootloader(env, deviceName)

Alias('Bootloader', bootTargets)
