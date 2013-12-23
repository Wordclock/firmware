# Wordclock

![Wordclock Frontpanel][0]

This is a fork of the firmware in control of the Wordclock, which is a project
that emerged from a quite popular [thread][1] started over at the German
mikrcontroller.net forums. It is not *officially* linked with this project,
however.

The structure, hardware and electronic are explained in detail [here][2] and
[there][3]. Keep in mind that this firmware will only work with "Variante 1".

## DOCUMENTATION

- There is a user manual, which is developed along with the source code itself.
  It is based upon LaTeX and managed in a seperate repository. It can be found
  [here][4]. The user manual does not cover the technical aspects, but can be
  useful in case you want to know how a specific feature is expected to work.

- The source code itself is documented quite heavily using [Doxygen][5]. An
  appropriate Doxyfile is provided along with the sources and can be found
  within the "doc" directory. It can be used to generate a HTML and/or PDF
  reference.

## SOFTWARE REQUIREMENTS

In order to build the source code, you'll need a current toolchain for Atmel
AVR microcontrollers. The code itself is tested and developed with avr-gcc
4.8.x, avr-binutils 2.23.x and avr-libc 1.8.x in mind. Other toolchains and/or
versions might work just fine, but are not fully tested.

## CONFIGURATION

There are various options, which influence the building process and hence the
resulting binary. These options can be found within "main.h" in the "src"
directory, along with comments about their meaning and possible values.

## BUILDING

Until a Makefile will be provided along with this package, the source code
needs to be built manually and/or using your IDE. Make sure to exclude
"src/usermodes.c" from the build. It will be included within "user.c" and does
not need to be compiled on its own.

## FLASHING

The Intel HEX file can be flashed using [avrdude][7], which can also be used to
configure the Fuse values. In its default configuration the following values
are expected:

- **lfuse**: *0xe2*
- **hfuse**: *0xdc*
- **efuse**: *0xfd*

Make sure your Fuse values match these or program them appropriately otherwise.

Normally you don't need to worry about the EEPROM too much. It will be
initialized and/or updated automatically by the software itself, whenever any
change to the software version or the internal structure of the data handled by
the EEPROM is detected. When no such change was made but you still want to get
rid of any settings within the EEPROM, you can simply erase it. It will then be
reinitialized with senseful default values.

## BOOTLOADER

The original firmware was shipped with a bootloader called [AVRootloader][8]
from Hagen Re. Unfortunately this bootloader is in no way FLOSS and hence was
not taken over into this project itself. It is still supported, and can be
obtained on its own. There are also a few other bootloaders that are supported
directly, namely [chip45boot2][9] and [FastBoot][10] from Peter Danegger.

Depending upon the bootloader you choose, you might have to change the fuses
and/or the *BOOTLOADER_RESET_WDT *directive within "main.h":

- **AVRootloader, FastBoot**: Set **efuse** to *0xfc*

- **chip45boot2**: Set **efuse** to *0xf8* and **disable**
  *BOOTLOADER_RESET_WDT*

Instructions on how to actually use the bootloaders mentioned above can be
found on the appropriate project sites itself. You'll also find some German
assistance [here][3], which contains a short description of what you have to do
to get these bootloaders working correctly.

## CONTRIBUTIONS

The source code itself is maintained using git. The project along with its
repositories lives over at [gitorious.org][11]. Contributions of any kind
(bug reports, feature requests, patches) are always welcome. The TODO file
within the projects root directory contains a list of things that still
need to be done, so feel free to start hacking some of them ;).

[0]: https://www.mikrocontroller.net/wikifiles/4/42/Wordclock-frontplatte-v2.png "Wordclock Frontpanel"
[1]: https://www.mikrocontroller.net/topic/156661
[2]: https://www.mikrocontroller.net/articles/Word_Clock
[3]: https://www.mikrocontroller.net/articles/Word_Clock_Variante_1
[4]: https://gitorious.org/wordclock/manual
[5]: https://www.stack.nl/~dimitri/doxygen/
[6]: https://gitorious.org/wordclock/pages/Home
[7]: http://www.nongnu.org/avrdude/
[8]: https://www.mikrocontroller.net/articles/AVR-Bootloader_mit_Verschl%C3%BCsselung_von_Hagen_Re
[9]: http://www.chip45.com/avr_bootloader_atmega_xmega_chip45boot2.php
[10]: https://www.mikrocontroller.net/articles/AVR_Bootloader_FastBoot_von_Peter_Dannegger
[11]: https://gitorious.org/wordclock/