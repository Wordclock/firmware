#!/bin/sh

if [ -z "$1" ]
then
    DEV=/dev/ttyUSB0
else
    DEV=$1
fi

if [ -z "$2" ]
then
    BAUD=9600
else
    BAUD=$2
fi

/usr/bin/stty -F $DEV $BAUD cs8 -cstopb -ixon
/usr/bin/echo -e '\rr\r' > $DEV
