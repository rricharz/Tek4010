# Tektronix 4010 Terminal Emulator

This is a [Tektronix 4010](https://en.wikipedia.org/wiki/Tektronix_4010) terminal emulator
for the Raspberry Pi.

It attempts to emulate the storage tube display of the Tektronix 4010, including the bright
drawing spot. At the moment, it only supports persistent drawing, but there are plans to
emulate the Tektronix 4014 "write through" mode to support a small number of non-persistent
vectors for animations.

To the best of my knowledge this is the only Tektronix 4010 emulator, which makes an effort
to emulate the storage tube behavior of the 4010. If the look and feel is not important, you can
use ["xterm"](https://en.wikipedia.org/wiki/Xterm) instead. "xterm" does not support the
"write through" mode.

This tek4010 emulator is currently in beta-testing and updated frequently.

It can be used to log into a historical Unix system such as
[2.11 BSD](https://en.wikipedia.org/wiki/Berkeley_Software_Distribution) on the
[pidp11](http://obsolescence.wixsite.com/obsolescence/pidp-11)
or a real historical system.

This [video](https://youtu.be/tmy7dx_8fAM) was generated entirely on a Raspberry Pi. The Raspberry Pi was
running the following software at the same time. The graphics drawings were recorded at about 50% of
the actual speed due to the screen recorder using up cpu cycles.

- PiDP-11 software with simh, emulating 2.11 BSD Unix, executing graphics programs
- tek4010 software using telnet to access the historical OS with a terminal multiplexer
- [simplescreenrecorder](https://www.maartenbaert.be/simplescreenrecorder) to record the video
- vnc to control everything from a Mac

This tek4010 emulator does currently not support the crosshair cursor of some 4010 terminals
with its graphics input (GIN) mode. The tab character is implemented as a tab8 function
instead of the single blank character of the original 4010 to make text better
readable. Also, there is no hardcopy mode, but you can make screen snapshots using "scrot",
or screen videos using [simplescreenrecorder](https://www.maartenbaert.be/simplescreenrecorder).

Install the tek4010 emulator from this repo on a Raspberry Pi. I propose using

	git clone git://github.com/rricharz/Tek4010
	cd Tek4010

This allows you to get updates later easily as follows:

	cd Tek4010
	git pull

There is a file "captured_data" in the repo, which you can use to test the tek4010 emulator.
"captured_data" was produced in 2.11 BSD using my program "dodekagon". Type

	./tek4010 cat captured_data -noexit

If you want to test text output, type for example

	./tek4010 head -n 32 tek4010.c -noexit

Don't forget the LAST argument "-noexit", which tells tek4010 to stay alive after cat or
head has finished so that you have a chance to look at the output.

If you want to see a demo of historical Tektronix 4014 plot files, type

	./tek4010 ./demo.sh

The emulator does use "rsh" or "telnet", because historical Unix systems do not support
the secure ssh protocol, and because ssh does not allow using a virtual emulator such as tek4010
for security reasons. You need therefore to install rsh or telnet on the Raspberry Pi running
the tek4010 emulator:

	sudo apt-get install rsh-client
or

	sudo apt-get install telnet

If you want to use this emulator together with 2.11 BSD Unix, look also at
[Using the historical Unix 2.11 BSD operating system on the PiDP-11](https://github.com/rricharz/pidp11-2.11bsd.git)

**Login directly into a remote historical Unix operating system**

This can either be a real historical computer, or a virtual system using simh such
as the PiDP-11.

First, you need to test the remote login from your client machine into your historical
system, using

	rsh -l user_name system
or

	telnet system

where "user_name" is the name of the user on the historical operating system, and "system"
is the hostname of this system. If the historical operating system is running using an
emulator, this is NOT the hostname of the system, on which the emulator is running. See
the chaptor below if you prefer to login into the system, on which the emulator is
running. For example, type

	rsh -l rene pdp11
or

	telnet pdp11

If this works properly, you can use the tek4010 emulator as follows:

	./tek4010 rsh -l user_name system
or

	./tek4010 telnet system

If the terminal window is closed right away, there is a problem with your rsh or
telnet call. Test it first without tek4010.

The following keys are not transmitted to the Unix system, but are executed locally
in the terminal emulator and clear the persistent screen:

	home
	page up
	page down
	control arrow up
	control arrow left

These keys emulate the "page" key of the Tektronix 4010. You need to use one of these
keys frequently to avoid to get a mess on the screen, as on a real Tektronix 4010.

**Login into the system running simh (same or different Raspberry Pi)**

This makes sense, if you have set up a virtual DZ11 for multiple user login, opening a
telnet port for multiplexed terminals. On the PiDP-11 using 2.11 BSD, the distribution software has
already set up port 4000 for 8 multiuser terminals. First, you need to install and test
telnet (2.11 BSD needs to be up and running in multiuser mode):

	sudo apt-get install telnet
	telnet raspi_hostname 4000

or

	telnet localhost 4000

where "raspi_hostname" is the hostname of your system running simh. If you are using telnet
and tek4010 on the same system as the system running simh, use "localhost" instead of
"raspi_hostname". You can even use VNC viewer on your laptop in this case, instead of an
attached keyboard and mouse.

Once this works, you can start tek4010 as follows:

	tek4010 raspi_hostname 4000

or

	tek4010 localhost 4000	

**Login into PiDP-11 running on the same Raspberry Pi, using the console**

This is the least preferred setup, only to be used if you cannot use one of the setups
above. You cannot use the tek4010 emulator running screens, as it is done in the standard setup
of the PiDP using the console, because screens filters the output stream of simh and is
therefore unsuitable for graphics terminals such as the tek4010 emulator. If you don't
want to change the standard setup, use control-e to stop simh, and then "exit" to quit simh.

Because tek4010 needs rsh, you need to install rsh-server and rsh-client on
the Raspberry Pi. You cannot use telnet here.

	sudo apt-get install rsh-server
	sudo apt-get install rsh-client

Now start tek4010 as follows:

	./tek4010 rsh -l pi localhost

This should give you a login prompt into your Raspberry Pi. If not, test the rsh call first.

Once your password has been accepted, be prepared to use the "home" key or any of the other
keys described above frequently to avoid to get a mess on the dump 4010 terminal emulator!
The following will start the PiDP software without using screens:

	cd /opt/pidp11/bin
	./pidp11.sh

Everything should run as expected, and you should be able to use the tek4010 terminal emulator with any of
the historical operating systems.

One word of caution! If you run the PiDP-11 software this way without using screens, you SHOULD
NOT detach or quit the terminal while your historical operating system is running, because
this will kill the PiDP-11 simh emulator right away. First run down your historical operating
system and simh properly, before detaching the terminal emulator!

**Reporting problems**

As this software is still in beta test, there will be problems. I just do not have enough
programs doing graphics to properly test all possible aspects.

If everything works properly for you, but your graphics application produces garbage on the
tek4010 emulator, you could send me your data as follows: On a historical Unix system, type

	your_graphics_program > captured_data

I don't know how this can be done on other operating systems. You can then mail your
captured_data file together with a description of the problem to rricharz77@gmail.com.
Pack it with zip or something else to make sure that the mailing program does not alter it.

**Screen resolution**

This tek4010 emulator creates a graphics window of 1024x780 points, which is the display size
of the Tektronix 4010 terminaland the Tektronix 4014 terminal without enhanced graphics module.
The Raspberry Pi can handle
sufficiently high refresh rates at this resolution. This emulator makes an attempt to filter
Tektronix 4014 graphics code with the enhanced graphics module installed, so that such
graphics codes can be displayed using this terminal emulator, but the lowest two bits of
each axis are not used in this case, as in the Tektronix 4014 without the enhanced graphics
module. It would be easy to add the capability to support the 4K resolution of the 4014 with
enhanced graphics module, but the current Raspberry Pi hardware cannot handle such a high
resolution.

**Compiling the tek4010 project**

If you want to compile the project, you need to install "libgtk-3-dev":

	sudo apt-get install libgtk-3-dev

There is a make file in the repo.


