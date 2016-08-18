import os

libraries_path = os.getenv("CODE_LIBRARY_PATH")

cppdefines = ['F_CPU=16000000', "F_USB=16000000", "USE_LUFA_CONFIG_HEADER"]
cppflags = ['-Wall', '-Wextra', '-std=c99', '-mmcu=at90usb162', '-g', '-Os', '-ffunction-sections', '-fdata-sections']
cpppath = [".", libraries_path + "/AVR", libraries_path + "/LUFA", libraries_path + "/Minimus", libraries_path + "/Generics"]
linker_flags = ['-Wl,--gc-sections', '-Os', '-mmcu=at90usb162']

env = Environment(ENV=os.environ, CC="avr-gcc", CPPFLAGS=cppflags, CPPPATH=cpppath, CPPDEFINES=cppdefines)

sources = [
    'usb-keypress-trigger.c',
    'Descriptors.c',
    libraries_path + "/AVR/lib_clk.c",
    libraries_path + "/AVR/lib_fuses.c",
    libraries_path + "/Minimus/minimus.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/USBTask.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/DeviceStandardReq.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c",
    libraries_path + "/LUFA/LUFA/Drivers/USB/Class/Device/HIDClassDevice.c",
]

objects = [ env.Object(src) for src in sources ]

elf_builder = env.Program('usb-keypress-trigger.elf', objects, LINKFLAGS=linker_flags)
hex_copier = env.Command("usb-keypress-trigger.hex", "usb-keypress-trigger.elf", "avr-objcopy -R .eeprom -O ihex $SOURCE $TARGET")

env.AlwaysBuild(elf_builder)
env.AlwaysBuild(hex_copier)

if 'upload' in COMMAND_LINE_TARGETS:
    mmcu = "atmega32u2"
    commands = [
        "dfu-programmer {} erase".format(mmcu),
        "dfu-programmer {} flash $SOURCE".format(mmcu),
        "dfu-programmer {} start".format(mmcu)
    ]

    upload_command = env.Command(None, "usb-keypress-trigger.hex", commands)
    env.Alias("upload", upload_command)

