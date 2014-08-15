# Wordclock

![Wordclock Frontpanel][0]

This is a fork of the firmware in control of the Wordclock, which is a project
that emerged from a quite popular [thread][1] started over at the German
mikrocontroller.net forums. It is not *officially* linked with this project,
however. The reason for forking the project in the first place was to clean up
the sources in order to have a better foundation for future modifications. The
upstream project was not interested in this effort and appears to be quite dead,
anyway. Although not completely finished with the clean up yet, a couple of
issues have been fixed and a bunch of features were already added. This project
is based upon version 0.12 of the original firmware and is diverging from it
ever since. However, all of the changes up to version 0.13 have been included.

## HIGHLIGHTS

These are some of the highlights, which differentiate this version from the
original one and make it more attractive:

- **Sources:** Completely reorganized, fully documented and mostly cleaned up,
  making it more easy to understand and/or extend.

- **Restructuring:** Some rework of a few interrupt routines was performed,
  leading to a significant disburdening of the microcontroller. Quite a few
  datatypes that previously were reimplemented by different modules have been
  unified, too, which not only saves program space, but also integrates the
  modules in a more consequent way.

- **UART protocol:** There is support for controlling the Wordclock via a
  serial connection, making the IR remote control completely obsolete and
  providing a much more convenient way of interacting with the Wordclock.

- **IRMP:** As the version in the original firmware has not been touched for a
  couple of years now, it misses a lot of protocols that have been added to
  the mainline version in the meantime.

- **wordboot:** A new bootloader was developed, which is more feature-rich and
  better adopted to the Wordclock project, visually indicating its current
  status on the frontpanel display.

Refer to the [commit logs][4] for a full list of changes.

## HARDWARE REQUIREMENTS

The structure, hardware and electronics of this project are explained in detail
[here][2] and [there][3]. Keep in mind that this firmware will only work with
"Variante 1", which supports frontpanels for both English and German.

One of the goals of this fork is to stay as compatible as possible with the
original project and the corresponding hardware. However, as the program space
of the ATmega168 simply is not sufficient for all of the new features, it
became necessary to replace it with an ATmega328P, which is a drop in
replacement. The microcontroller was designed to be put into a socket, so this
should not be too much of a problem.

## SOFTWARE REQUIREMENTS

In order to build the source code, you'll need a current toolchain for Atmel
AVR microcontrollers. The code itself is tested and developed with avr-gcc
4.9.x, avr-binutils 2.24.x and avr-libc 1.8.x in mind. Other toolchains and/or
versions might work just fine, but are not fully tested.

## DOCUMENTATION

- There is a user manual, which was developed along with the source code
  itself. It is based upon LaTeX and managed in a separate repository. It can
  be found [here][5]. The user manual does not cover the technical aspects, but
  can be useful in case you want to know how a specific feature is expected to
  work. However, it was not updated for quite a while, so it is incomplete.

- The source code itself is documented quite heavily using [Doxygen][6]. An
  appropriate Doxyfile is provided along with the sources and can be found
  within the `doc/` directory. It can be used to generate a HTML and/or PDF
  reference.

## CONFIGURATION

There are various options, which influence the building process and hence the
resulting binary. These options can be found within `src/config.h`, along with
comments about their actual meaning and possible values.

## BUILDING

Until a Makefile will be provided along with this project, the source code
needs to be built manually and/or using an IDE. Make sure to exclude
`src/usermodes.c` from the build. It will be included within `src/user.c`
automatically and does not need to be compiled on its own.

## FLASHING

The Intel HEX file can be flashed using [avrdude][7], which can also be used to
configure the Fuse values. In its default configuration the following values
are expected:

- **lfuse**: `0xE2`
- **hfuse**: `0xD9`
- **efuse**: `0xFC`

Make sure your Fuse values match these or program them appropriately otherwise.

Usually you don't need to worry about the EEPROM at all. It will be initialized
and/or updated automatically by the software itself, whenever any change to the
software version or the internal structure of the data handled by the EEPROM is
detected. When no such change was made but you still want to get rid of any
settings within the EEPROM, you can simply erase it. It will then be
reinitialized with useful default values.

## BOOTLOADER

The original firmware was shipped with a bootloader, which in no way was FLOSS
and hence was not taken over into this project. The suggested alternatives were
platform dependent and did not implement any official standard. A new
bootloader named *wordboot* was developed. It can be found within the
`bootloader/wordboot` directory along with documentation about its usage.

wordboot makes use of the Wordclock hardware to visually indicate its current
status on the frontpanel display. Furthermore it implements the STK500 protocol
and is directly supported by [avrdude][7], so no proprietary tools are needed.

In order to be able to use *wordboot* the following change is necessary to the
fuse settings:

- **hfuse**: `0xDC`

Support for other bootloaders has been completely dropped in favour of
wordboot, so they are probably no longer working as expected.

## CONTRIBUTIONS

The source code itself is maintained using git. The project along with its
[repositories][8] lives over at github.com. Contributions of any kind are
highly welcome. The simplest and fastest way for these kind of things is to use
the means provided by github.com itself, which allows for reporting bugs and/or
requesting features. It is also the preferred way to submit patches in form of
pull requests. If you are new to git and/or are not familiar with this process,
refer to [this][9] for a detailed description on how to submit a pull request.

In case you are looking for something to work on, you probably want to take a
look at the `TODO` file within the projects root directory. More recently the
github.com [issue tracker][10] is used to keep track of issues and coordinate
the development of new features. There is enough work left to do, so feel free
to start hacking away ;).

There is also a [repository][11] over at gitorious.org, which currently is
being used only as a mirror.

## DONATIONS

[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png "Flattr This!")](https://flattr.com/submit/auto?user_id=johnpatcher&url=https://github.com/Wordclock/firmware)

[![PayPal donation](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif "PayPal")](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=karol%40babioch%2ede&lc=DE&item_name=Wordclock&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest)

Bitcoin: `1Dn6MEKgRAgdRS8Aeg88fug9XmdgTRpCDA`

## LICENSE

[![GNU GPLv3](http://www.gnu.org/graphics/gplv3-127x51.png "GNU GPLv3")](http://www.gnu.org/licenses/gpl.html)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

[0]: https://www.mikrocontroller.net/wikifiles/1/17/Wordclock-frontpanel.png "Wordclock Frontpanel"
[1]: https://www.mikrocontroller.net/topic/156661
[2]: https://www.mikrocontroller.net/articles/Word_Clock
[3]: https://www.mikrocontroller.net/articles/Word_Clock_Variante_1
[4]: https://github.com/Wordclock/firmware/commits/master
[5]: https://github.com/Wordclock/manual
[6]: https://www.stack.nl/~dimitri/doxygen/
[7]: http://www.nongnu.org/avrdude/
[8]: https://github.com/Wordclock
[9]: https://help.github.com/articles/using-pull-requests
[10]: https://github.com/Wordclock/firmware/issues
[11]: https://gitorious.org/Wordclock
