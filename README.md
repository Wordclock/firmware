# Wordclock

![Wordclock Frontpanel][0]

This is a fork of the firmware in control of the Wordclock, which is a project
that emerged from a quite popular [thread][1] started over at the German
mikrocontroller.net forums. It is not *officially* linked with this project,
however. The reason for forking the project in the first place was to clean up
the sources in order to have a better foundation for future modifications. The
upstream project wasn't interested in this effort and appears to be quite dead,
anyway. Although not completely finished with the clean up yet, a couple of
issues have been fixed and a bunch of features were already added. This project
is based upon version 0.12 of the original firmware and is diverging from it
ever since. However, all of the changes up to version 0.13 have been included.

## HIGHLIGHTS

These are some of the highlights, which differentiate this version from the
original one and make it more attractive:

- **Sources:** Completely reorganized, fully documented and mostly cleaned up,
  making it more easy to understand and/or extend them.

- **Restructuring:** Some rework of a few interrupt routines was performed,
  leading to a significant disburdening of the microcontroller..

- **UART protocol:** There is support for controlling the Wordclock via a
  serial connection, making the IR remote control completely obsolete and
  providing a much more convenient way of interacting with the Wordclock.

- **IRMP:** As the version in the original firmware hasn't been touched for a
  couple of years now and actually misses a lot of features and/or bug fixes
  newer versions of IRMP offer.

Refer to the [commit logs][4] for a full list of changes.

## HARDWARE REQUIREMENTS

The structure, hardware and electronics of this project are explained in detail
[here][2] and [there][3]. Keep in mind that this firmware will only work with
"Variante 1".

One of the goals of this fork is definitely to stay as compatible as possible
with the original project and the corresponding hardware. However, as the
program space of the ATmega168 is simply not sufficient for all of the new
features, is highly recommended to replace the microcontroller with the
ATmega328, which is drop in replacement. As the microcontroller was designed to
be put into a socket anyway, this shouldn't be too much of a problem.
Alternatively you can always give up some functionality by disabling the
appropriate switches within `src/config.h`.

## SOFTWARE REQUIREMENTS

In order to build the source code, you'll need a current toolchain for Atmel
AVR microcontrollers. The code itself is tested and developed with avr-gcc
4.8.x, avr-binutils 2.23.x and avr-libc 1.8.x in mind. Other toolchains and/or
versions might work just fine, but are not fully tested.

## DOCUMENTATION

- There is a user manual, which was developed along with the source code
  itself. It is based upon LaTeX and managed in a separate repository. It can
  be found [here][5]. The user manual does not cover the technical aspects, but
  can be useful in case you want to know how a specific feature is expected to
  work.

- The source code itself is documented quite heavily using [Doxygen][6]. An
  appropriate Doxyfile is provided along with the sources and can be found
  within the `doc/` directory. It can be used to generate a HTML and/or PDF
  reference.

## CONFIGURATION

There are various options, which influence the building process and hence the
resulting binary. These options can be found within `src/config.h`, along with
comments about their meaning and possible values.

## BUILDING

Until a Makefile will be provided along with this package, the source code
needs to be built manually and/or using an IDE. Make sure to exclude
`src/usermodes.c` from the build. It will be included within `src/user.c`
automatically and does not need to be compiled on its own.

## FLASHING

The Intel HEX file can be flashed using [avrdude][7], which can also be used to
configure the Fuse values. In its default configuration the following values
are expected:

- **lfuse**: `0xe2`
- **hfuse**: `0xdc`
- **efuse**: `0xfd`

Make sure your Fuse values match these or program them appropriately otherwise.

Usually you don't need to worry about the EEPROM at all. It will be initialized
and/or updated automatically by the software itself, whenever any change to the
software version or the internal structure of the data handled by the EEPROM is
detected. When no such change was made but you still want to get rid of any
settings within the EEPROM, you can simply erase it. It will then be
reinitialized with useful default values.

## BOOTLOADER

The original firmware was shipped with a bootloader called [AVRootloader][8]
from Hagen Re. However, this bootloader is in no way FLOSS and hence was not
taken over into this project itself. It is still supported, and can be obtained
on its own. There are also a few other bootloaders that are supported directly,
namely [chip45boot2][9] and [FastBoot][10] from Peter Danegger.

Depending upon the bootloader you choose, you might have to change the fuses
and/or the `BOOTLOADER_RESET_WDT` directive within `src/config.h`:

- **AVRootloader, FastBoot**: Set **efuse** to `0xfc`

- **chip45boot2**: Set **efuse** to `0xf8` and **disable**
  `BOOTLOADER_RESET_WDT`

Instructions on how to actually use the bootloaders mentioned above can be
found on the appropriate project sites itself. You'll also find some German
assistance [here][3], which contains a short description of what you have to do
in order to get these bootloaders working correctly.

## CONTRIBUTIONS

The source code itself is maintained using git. The project along with its
repositories lives over at [gitorious.org][11]. Contributions of any kind
(bug reports, feature requests, patches) are welcome. The `TODO` file within
the projects root directory contains a list of things that still need to be
done, so feel free to start hacking away ;).

[0]: https://www.mikrocontroller.net/wikifiles/1/17/Wordclock-frontpanel.png "Wordclock Frontpanel"
[1]: https://www.mikrocontroller.net/topic/156661
[2]: https://www.mikrocontroller.net/articles/Word_Clock
[3]: https://www.mikrocontroller.net/articles/Word_Clock_Variante_1
[4]: https://gitorious.org/wordclock/firmware/commits/
[5]: https://gitorious.org/wordclock/manual
[6]: https://www.stack.nl/~dimitri/doxygen/
[7]: http://www.nongnu.org/avrdude/
[8]: https://www.mikrocontroller.net/articles/AVR-Bootloader_mit_Verschl%C3%BCsselung_von_Hagen_Re
[9]: http://www.chip45.com/avr_bootloader_atmega_xmega_chip45boot2.php
[10]: https://www.mikrocontroller.net/articles/AVR_Bootloader_FastBoot_von_Peter_Dannegger
[11]: https://gitorious.org/wordclock/
