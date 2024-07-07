
# Tektronix 4010 and 4014 Storage Tube Terminal Emulator

This is a [Tektronix 4010, 4013, 4014 and 4015](https://en.wikipedia.org/wiki/Tektronix_4010)
terminal emulator for the Raspberry Pi, Linux, macOS (Macintosh) and Windows.

All instructions can be found in [Manual.pdf](https://github.com/rricharz/Tek4010/blob/master/Manual.pdf)
in the main Tek4010 directory. The installation is described there. For macOS, start with the preparation instructions in
[macintosh.txt](https://github.com/rricharz/Tek4010/blob/master/macintosh.txt) in this repository. For Windows, start with
the preparation instructions in [windows.txt](https://github.com/rricharz/Tek4010/blob/master/windows.txt) in this repository.

![screen_shot](screendump.png?raw=true "tek4010 screendump")

Below is a screen shot of the spacelab from the ICEMDDN CAD package made on a CDC Cyber 175 mainframe
emulator.

![spacelab](spacelab.png?raw=true "spacelab screendump")

tek4010 can also display historical data for the
[MIT Project MAC](https://en.wikipedia.org/wiki/MIT_Computer_Science_and_Artificial_Intelligence_Laboratory#Project_MAC)
's ARDS (Advanced Remote Display Station):

![ARDS_screen_shot](trek.png?raw=true "tek4010 ARDS screendump")

tek4010 makes an effort to emulate the [storage tube display](https://en.wikipedia.org/wiki/Storage_tube)
of the Tektronix 4010, including the fading bright drawing spot. If the look and feel is not important, you can
use ["xterm"](https://en.wikipedia.org/wiki/Xterm) instead. "xterm" does not support all
graphics modes of the 4014.

It can be used to log into a historical Unix system such as
[2.11 BSD](https://en.wikipedia.org/wiki/Berkeley_Software_Distribution) on the
[PiDP-11](http://obsolescence.wixsite.com/obsolescence/pidp-11)
or a real historical system. It can also be used to display historical plot data.

The following picture shows a scale model of the Tektronix 4010 crafted by
Dave Ault using tek4010.

![scale model](scalemodel.jpg?raw=true "scale model of Tektronix 4010")

The storage tube emulator and the Tektronix 4010/4014 decoder were witten by Rene Richarz. The ARDS
decoder was written by Lars Brinkhoff. He also provided some interesting historical documents and
the ARDS plot files. Teunis van Beelen has written the helper program “rs232-console” to connect
to a host using a serial link. Dave Ault tested the serial link to his PDP-11/73. The historical
plot data for the Tektronix 4014 was obtained from Jos Dreesen. The special plot mode pictures
with variable brightness were obtained from Monty McGraw. He also helped to debug the special
plot point mode. The historical plot data of the spacelab from the ICEMDDN CAD package on a CDC
Cyber 175 mainframe emulator was obtained from Nick Glazzard. He also helped to improve the GIN
mode substantially. The CP/M GSXBASIC plot files are from Udo Munk. Thanks to Ian Schofield for
his critical comments and a code snippet for drawing dashed and dotted lines, and to Oscar Vermeulen
and Mark Matlock for their support. The manuals were obtained from bitsavers.org. Thanks also to all
others who contributed important ideas, helped with the debugging and preserved the historical data.
This program is the result of a community effort.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
