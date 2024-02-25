
# Tektronix 4010 and 4014 Storage Tube Terminal Emulator

This is a [Tektronix 4010, 4013, 4014 and 4015](https://en.wikipedia.org/wiki/Tektronix_4010)
terminal emulator for the Raspberry Pi and other Linux systems.

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

All instructions can now be found in the [Manual](http://github.com/rricharz/Tek4010/manual.pdf)
