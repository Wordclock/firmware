# wordboot

This directory contains a bootloader named *wordboot*. It was originally based
upon the [optiboot][1] bootloader from the [Arduino][2] project. By being
compatible with [avrdude][3] it offers platform independence. *wordboot* is
customized to the Wordclock project and uses the frontpanel to visually
indicate the current status.

## FEATURES

- [avrdude][3] compatibility: The bootloader implements the original STK500
  protocol as described in [AVR061: STK500 Communication Protocol][4] and hence
  is well supported by avrdude (and possibly other programming utilities).

- EEPROM support: In addition to simple read and write operations to the
  conventional flash memory, it is also possible to read from and write to the
  EEPROM memory.

- Smallish: The complete bootloader fits into only **512** words of program
  memory.

- Visual indication: The current status (during boot-up and flashing) is being
  signaled using the four available minute LEDs of the frontpanel.

- Configurable: Several options are available that influence the behaviour of
  the bootloader. They are described in more detail in the
  [appropriate](#CONFIGURATION) section.

- Written in C: Unlike many other bootloaders, *wordboot* is written completely
  in C, making it more easy to understand and maintain. It relies upon
  functionality provided by [avr-libc][5], upstreaming a great deal of
  complexity.

## REQUIREMENTS

The code itself is tested and developed with avr-gcc 4.9.x, avr-binutils 2.24.x
and avr-libc 1.8.x in mind. Other toolchains and/or versions might not work as
expected, especially since there are some restrains to the size the bootloader
is allowed to utilise.

This bootloader requires the `BOOTRST` fuse to be programmed. The boot flash
section size should be set to **512 words**, i.e. the boot start address is
`0x3e00` word address (`0x7c00` byte address). This results in the following
recommended fuse values:

- **lfuse**: `0xE2`
- **hfuse**: `0xDC`
- **efuse**: `0xFC`

## CONFIGURATION

There are several options directly influencing the behaviour of the bootloader:

- **BAUD_RATE**: The default value is *9600*. It was chosen as the actual
  Wordclock firmware uses the same baud rate, so no change is needed in case a
  Bluetooth module is used. Baud rates up to *115200* have been tested
  successfully. Effectively this is only limited by the hardware. Keep in mind,
  though, that the Wordclock project uses the internal RC oscillator.

- **LED_START_FLASHES**: The default value is *3*. This defines the number of
  LED flashes when the bootloader is entered.

- **LED_DATA_FLASH**: The default value is *1* (enabled). This defines whether
  the LEDs should flash when data is being received. Remove this define if
  you want to disable this feature.

- **ENABLE_RGB_SUPPORT**: The default value is *1* (enabled). This defines
  whether the bootloader should be built with RGB support, i.e. the minute
  LEDs will appear in white as all channels (red, green, blue) will be enabled
  simultaneously. Otherwise only the red channel will be used.

- **BOOTLOADER_TIMEOUT_MS**: The default value is *1000* (1 second). This
  defines the time after which the bootloader is exited and the actual firmware
  itself is started.

The options are expected to be changed within the Makefile (as additional
CFLAGS), although it is also possible to change them directly within the source
by defining appropriate macros.

## BUILDING

Building the bootloader requires the appropriate sections to be located
correctly. A suitable Makefile is provided taking care of this, while also
making sure that the result stays small by disabling unneeded options
enabled by default.

To build the firmware simply run `make` for the `wordclock` target:

    make wordclock

This will generate a `wordboot.hex` file, which can be flashed to the
microcontroller in the conventional way (i.e. ISP/HVPP). The `wordboot.lst`
file contains the resulting assembler listing and can be useful for debugging
purposes.

## USAGE

If the bootloader has been flashed successfully to the MCU and the fuses are
programmed correctly, the four minute LEDs should blink `LED_START_FLASHES`
times whenever the bootloader is entered (i.e. when powering up the Wordclock
or when restarting it using the appropriate UART command). The bootloader is
now accessible for as long as defined in `BOOTLOADER_TIMEOUT_MS`.

While the bootloader is accessible, an application can be flashed using the
following command:

    avrdude -p m328p -c arduino -b 9600 -P /dev/ttyUSB1 -U flash:w:Wordclock.hex

Make sure that the baud rate (-b) matches the configured setting. After a
successful flash the microcontroller performs a reset cycle and should start
the actual application briefly afterwards.

## UTILITIES

The provided `reset_wordboot.sh` script can be used to restart the Wordclock
programmatically. It sends the appropriate UART command to the given device.
If no device was specified `/dev/ttyUSB0` is chosen. The baud rate can be
specified as a second parameter and defaults to 9600.

    ./reset_wordclock.sh /dev/ttyUSB1 9600

[1]: https://code.google.com/p/optiboot/
[2]: http://www.arduino.cc/
[3]: http://www.nongnu.org/avrdude/
[4]: http://www.atmel.com/Images/doc2525.pdf
[5]: http://www.nongnu.org/avr-libc/

