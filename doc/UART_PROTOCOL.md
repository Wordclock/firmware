# Wordclock - UART PROTOCOL

This document describes the protocol that allows the Wordclock to be controlled
with specific commands sent to the the microcontroller via the UART interface.
The revision of the protocol this document describes is **1.0**.

## MOTIVATION

The main application for this protocol is to be able to control the Wordclock
via a Bluetooth serial connection using an appropriate adapter. The protocol
provides much more functionality than the classic IR remote control, mainly
because there is also a backward channel for responses of the Wordclock.

Furthermore it is also possible to set some properties (e.g. color, time) in a
much more direct way, which increases the ease of use by a great deal.

## REQUIREMENTS

The protocol handling has to be built into the firmware itself. This is
controlled by the `ENABLE_UART_PROTOCOL` switch within the `src/config.h` file
and is enabled by default. The baud rate of the connection is defined by the
UART module. The appropriate setting is called `UART_BAUD` and can be found
within `src/uart.h`. The other parameters for this connections are:

- 8 bits per character
- No parity bit
- 1 Stop bit

This is also known as *8N1*. These parameters need to be the same on both ends
of the transmission, so make sure to set up everything correctly.

## DEBUGGING

The `LOG_LEVEL_UART_PROTOCOL_DEFAULT` switch within `src/config.h` can be used
to enable debugging messages of the uart_protocol module and is disabled
by default. You can also change the log level during runtime using the commands
listed below. Note however, that you might miss some events in this case.

## COMMAND SYNTAX

This section describes the command syntax in generic terms and applies to all
of the commands listed below.

Any command that is directed at the Wordclock needs to terminated with the
character as defined in `UART_PROTOCOL_COMMAND_INPUT_EOL` (defaults to `\r`).
Only after having received this character will the Wordclock try to execute
the command.

A command consists of the command itself and optional arguments to the command.
The command itself can be as long as defined in 
`UART_PROTOCOL_COMMAND_MAX_LENGTH` (defaults to `3`). If the command expects
any parameters there must be at least one space character between the command
and the first argument. The arguments itself need to be seperated by at least
one or more space characters. Every argument is expected to be passed in its
hexadecimal representation ([0-9a-f]) and needs to consist of exactly two
digits.

The format of any sort of response is described in the section `RESPONSES`.

## COMMANDS

This section lists all the valid commands along with the responses they will
generate.

### IR user command

**Command**: i UC  
UC: **See list below**  
**Description:** Issues the appropriate user command  
**Response:** OK  

These commands are equivalent to the appropriate IR user commands. They allow
the control the Wordclock in the same way it would be possible via an IR remote
control. The user command you want to issue is provided as the first and only
argument to this command.

Valid arguments along with the user command they will trigger are listed below:

- `o`: `UC_ONOFF`
- `l`: `UC_BRIGHTNESS_UP`
- `m`: `UC_BRIGHTNESS_DOWN`
- `+`: `UC_UP`
- `-`: `UC_DOWN`
- `t`: `UC_SET_TIME`
- `a`: `UC_SET_ONOFF_TIMES`
- `d`: `UC_DCF_GET_TIME`
- `N`: `UC_NORMAL_MODE`
- `P`: `UC_PULSE_MODE`
- `D`: `UC_DEMO_MODE`
- `H`: `UC_HUE_MODE`
- `r`: `UC_CHANGE_R`
- `g`: `UC_CHANGE_G`
- `b`: `UC_CHANGE_B`
- `h`: `UC_CHANGE_HUE`
- `c`: `UC_CALIB_BRIGHTNESS`
- `A`: `UC_AMBILIGHT`
- `B`: `UC_BLUETOOTH`
- `X`: `UC_AUXPOWER`
- `s`: `DISPLAY_SPECIAL_USER_COMMANDS`

For details about the user commands mentioned above, please refer to
`user_command_t` within `src/user_command.h`. Depending upon the value of
`ENABLE_INDIVIDUAL_CONFIG` within `src/config.h` and the appropriate switches
within `src/config.h` not all of these commands might be available.

All of these commands will response with a plain simple `OK` once they were
executed. The response for anything other than mentioned above and/or for
commands not implemented by this particular build will always be `ERROR`.

### Version

**Command**: v  
**Description:** Returns the version of the firmware  
**Response:** MAJOR MINOR  
MAJOR: [0-9a-f]{2}  
MINOR: [0-9a-f]{2}

### Keepalive

**Command**: k  
**Description:** Keeps the connection alive without any side-effects  
**Response:** OK


### Reset

**Command**: r  
**Description:** Resets the microcontroller  
**Response:** OK


### Factory reset

**Command**: f  
**Description:** Resets the Wordclock to its factory state  
**Response:** OK


### Get brightness from LDR

**Command**: bg  
**Description:** Returns the brightness as measured by the LDR  
**Response:** B  
B: [0-9a-f]{2} **Measured brightness**


### Read current color

**Command**: cr  
**Description:** Returns the RGB values of the currently used color  
**Response:** R G B  
R: [0-9a-f]{2} **Value for red**  
G: [0-9a-f]{2} **Value for green**  
B: [0-9a-f]{2} **Value for blue**  


### Write current color

**Command**: cw R G B  
R: [0-9a-f]{2} **Value for red**  
G: [0-9a-f]{2} **Value for green**  
B: [0-9a-f]{2} **Value for blue**  
**Description:** Applies the given values immediately  
**Response (values valid):** OK  
**Response (values invalid):** ERROR


### Get number of color presets

**Command**: pn  
**Description:** Returns number of color presets compiled into the firmware  
**Response:** N  
N: [0-9a-f]{2} **hex representation of `UI_COLOR_PRESET_COUNT`**


### Get currently active color preset

**Command**: pa  
**Description:** Returns the currently active color preset  
**Response:** N  
N: [0-9a-f]{2} **Value between 0 and `UI_COLOR_PRESET_COUNT` - 1**


### Set currently active color preset

**Command**: ps N  
**Description:** Sets the currently active color preset  
**Response (N was valid):** OK  
**Response (N was invalid):** ERROR


### Read color preset

**Command**: pr N  
N: [0-9a-f]{2} **Color preset to read**  
**Description:** Returns the RGB values of the specified color preset  
**Response (N was valid):** R G B  
R: [0-9a-f]{2} **Value for red**  
G: [0-9a-f]{2} **Value for green**  
B: [0-9a-f]{2} **Value for blue**  
**Response (N was invalid):** ERROR


### Write color preset

**Command**: pw N R G B  
N: [0-9a-f]{2} **Color preset to write**  
R: [0-9a-f]{2} **Value for red**  
G: [0-9a-f]{2} **Value for green**  
B: [0-9a-f]{2} **Value for blue**  
**Description:** Writes the given RGB values to the given color preset  
**Response (values valid):** OK  
**Response (values invalid):** ERROR


### Get current time

**Command**: tg  
**Description:** Returns the current time (hour, minutes, seconds)  
**Response (normal):** H M S  
H: [0-9a-f]{2} **Hour**  
M: [0-9a-f]{2} **Minutes**  
S: [0-9a-f]{2} **Seconds**  
**Response (error reading time from RTC):** ERROR


### Set current time

**Command**: ts H M S  
H: [0-9a-f]{2} **Hour**  
M: [0-9a-f]{2} **Minutes**  
S: [0-9a-f]{2} **Seconds**  
**Description:** Sets the current time (hour, minutes, seconds)  
**Response (values valid):** OK  
**Response (values invalid):** ERROR  
**Response (error writing time to RTC):** ERROR

### Get current date

**Command**: dg  
**Description:** Returns the current date (day, month, year, weekday)  
**Response (normal):** D M Y W  
D: [0-9a-f]{2} **Day**  
M: [0-9a-f]{2} **Month**  
Y: [0-9a-f]{2} **Year**  
W: [0-9a-f]{2} **Weekday**  
**Response (error reading date from RTC):** ERROR


### Set current date

**Command**: ds D M Y W  
D: [0-9a-f]{2} **Day**  
M: [0-9a-f]{2} **Month**  
Y: [0-9a-f]{2} **Year**  
W: [0-9a-f]{2} **Weekday**  
**Description:** Sets the current date (day, month, year, weekday)  
**Response (values valid):** OK  
**Response (values invalid):** ERROR  
**Response (error writing date to RTC):** ERROR


### Enable logging globally

**Command**: le  
**Description:** Enables the logging globally  
**Response:** OK


### Disable logging globally

**Command**: ld  
**Description:** Disables the logging globally  
**Response:** OK


### Check global logging status

**Command**: li  
**Description:** Checks whether logging is enabled globally  
**Response (enabled globally):** 01  
**Response (disabled globally):** 00  


### Get list of modules

**Command**: lm  
**Description:** Outputs a list of available modules  
**Response (for each module):** N  
N: [0-9a-zA-Z]* **Name of module**


### Get list of log levels

**Command**: ll  
**Description:** Outputs a list of available log levels  
**Response (for each log level):** N  
N: [0-9a-zA-Z]* **Name of log level**


### Set log level for particular module

**Command**: ls M L  
M: [0-9a-f]{2} **Module**  
L: [0-9a-f]{2} **Log level**  
**Description:** Sets the log level for a particular module  
**Response (values valid):** OK  
**Response (values invalid):** ERROR  


### Get log level for particular module

**Command**: lg M  
M: [0-9a-f]{2} **Module**  
**Description:** Returns the log level for a particular module  
**Response (values valid):** L  
L: [0-9a-f]{2} **Log level**  
**Response (values invalid):** ERROR  


## RESPONSES

Whenever an EOL as described by `UART_PROTOCOL_COMMAND_INPUT_EOL` within
`src/uart_protocol.h` (which by default is `\r\n`) is received the Wordclock
will try to execute the given command and response with a message that is
prefixed by the string defined by `UART_PROTOCOL_OUTPUT_PREFIX` within
`src/uart_protocol.h` (defaults to `>`).

So, by default, all of the responses will look like this:

    >RESPONSE\r\n

The content of the response itself depends upon the command that was detected
and is described in the section `COMMANDS`. Whenever the command could not be
detected successfully the content of the response will simply be `ERROR`.

Applications that interpret the response of a command should ignore
**anything** that is not prefixed with `UART_PROTOCOL_OUTPUT_PREFIX`, because
this output is not meant to be a response ot the command itself, but is most
likely some form of debugging information.

## EXAMPLES

This section contains various examples, which should help to get a better
understanding of the protocol. Special characters like `\r` and `\n` are shown
explicitely, but obviously wouldn't be visible on an actual terminal.

Issue an UC_ONOFF user command:

    i o\r
    >OK\r\n


Issue an UC_NORMAL_MODE user command:

    i N\r
    >OK\r\n


**Not** valid, results in an error repsonse:

    i Z\r
    >ERROR\r\n


Return the version of the firmware:

    v\r
    >00 0d\r\n


Performs a factory reset:

    f\r
    >OK\r\n


Get currently active color, which is red:

    cr\r
    >ff 00 00\r\n


Set currently active color to blue:

    cw 00 00 ff\r
    >OK\r\n


Set currently active color with invalid values:

    cw 00 00 zz\r
    >ERROR\r\n
