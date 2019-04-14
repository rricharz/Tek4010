[![Build Status](https://travis-ci.org/rricharz/Tek4010.svg?branch=master)](https://travis-ci.org/rricharz/Tek4010)

# Tektronix 4010 and 4014 Storage Tube Terminal Emulator

This is a [Tektronix 4010 and 4014](https://en.wikipedia.org/wiki/Tektronix_4010) terminal emulator
for the Raspberry Pi and other Linux systems. It can also be used on Windows and Macintosh systems
with [Virtualbox](https://www.virtualbox.org/) and [Ubuntu](https://www.ubuntu.com/).

It can also display historical data for the
[MIT Project MAC](https://en.wikipedia.org/wiki/MIT_Computer_Science_and_Artificial_Intelligence_Laboratory#Project_MAC)
's ARDS (Advanced Remote Display Station).

![screen_shot](screendump.png?raw=true "tek4010 screendump")

It attempts to emulate the [storage tube display](https://en.wikipedia.org/wiki/Storage_tube)
of the Tektronix 4010, including the bright drawing spot.

To the best of my knowledge this is the only Tektronix 4010/4014 emulator, which makes an effort
to emulate the storage tube behavior. If the look and feel is not important, you can
use ["xterm"](https://en.wikipedia.org/wiki/Xterm) instead. "xterm" does not support all
graphics modes of the 4014.

It can be used to log into a historical Unix system such as
[2.11 BSD](https://en.wikipedia.org/wiki/Berkeley_Software_Distribution) on the
[pidp11](http://obsolescence.wixsite.com/obsolescence/pidp-11)
or a real historical system. At can also be used to display historical plot data.

This [video of a tek4010 demo](https://youtu.be/ioYiu6oUT88) was generated using
[simplescreenrecorder](https://www.maartenbaert.be/simplescreenrecorder). There is also a
video of a an [animation using the tek4010](https://youtu.be/7FMewaoEOmk).

If you want to make an installation on Windows or OSX, first install Virtualbox and Ubuntu on
your system. Here are examples of guides for
[OSX](https://www.dev2qa.com/how-to-install-ubuntu-on-virtualbox-mac/)
and [Windows](https://itsfoss.com/install-linux-in-virtualbox/). Once you are running in
virtual Ubuntu you can proceed.

Install the tek4010 emulator from this repo on a Raspberry Pi or Ubuntu. I propose using

	sudo apt-get install git
	git clone git://github.com/rricharz/Tek4010
	cd Tek4010

This allows you to get updates later easily as follows:

	cd Tek4010
	git pull
        
The built tek4010 file is for a Raspberry Pi. If you are on Ubuntu, do the follwing to recompile
the program. On the Raspberry Pi you can skip this step.

        sudo apt-get install libgtk-3-dev
        rm tek4010
        make
        
Thanks to Lars Brinkhoff (lars@nocrew.org) to pointing out how easy it is to compile tek4010
on Ubuntu. He also helped me to fix some bugs and proposed many nice features. Don't forget to
recompile the program each time you update from the repository if you are using Ubuntu. I have
tested this on my Macintosh with Virtualbox and Ubunto, and it works very well!

There is a file "dodekagon.plt" in the repo, which you can use to test the tek4010 emulator.
"dodekagon.plt" was produced in 2.11 BSD using my program "dodekagon". Type

	./tek4010 -noexit cat dodekagon.plt

If you want to test text output, type for example

	./tek4010 -noexit head -n 32 tek4010.c

If you want to test an animation, type

	./tek4010 cat animation.plt

Don't forget the option "-noexit", which tells tek4010 to stay alive after cat or
head has finished so that you have a chance to look at the output. For a list of
all possible options, see the chapter "Options of the command tek4010" below.

If you want to see a demo of historical Tektronix 4014 plot files, type

	./tek4010 ./demo.sh

The emulator does use "rsh" or "telnet", because historical Unix systems do not support
the secure ssh protocol, and because ssh does not allow using a virtual emulator such as tek4010
for security reasons. You need therefore to install rsh or telnet on the Raspberry Pi
or Ubuntu running the tek4010 emulator:

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
the chapter below if you prefer to login into the system, on which the emulator is
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

The hardcopy function on the Tektronix 4010 is emulated with a screen dump.

	control-w	Make a screen dump in current directory using scrot
                        Can be typed on the keyboard or sent by the computer during alpha mode

You can also use the following control key function to close tek4010:

	control-q	Close tek4010 window and quit tek4010.

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

	tek4010 telnet raspi_hostname 4000

or

	tek4010 telnet localhost 4000	

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

**Options of the command tek4010**

Call the command tek4010 using the following syntax:

	tek4010 [options of tek4010] command [options of command]

"command" is a mandatory command to be run by tek4010, such as telnet, rsh or cat.

tek4010 has the following options:

	-noexit		do not close window after completion of "command"

	-raw		do not execute an automatic CR (carriage return) after a LF (line feed)

	-tab1		execute a blank	instead of a tab to the next 8-character column

	-b9600, -b4800, -b2400, -b1200, -b600, -b300
			Emulate a baud rate. Without one of these arguments, the baud rate
			is 19200 baud. The original Tektronix 4010 had a maximal baud rate
			of 9600 baud. With the small baud rates you can emulate 1970s 
			style modem performance. Early modems had a baud rate of 300.

	-full		in this mode the tek4010 emulator creates a full screen window, and
			uses the full resolution of the 4014 with the enhanced graphics module
			installed, scaled down to the actual window size. Use control-q to
			close the tek4010 window. This option is experimental.

	-ARDS		display ARDS data			

**Reporting problems**

As this software is still in beta test, there will be problems.

If everything works properly for you, but your graphics application produces garbage on the
tek4010 emulator, you can send me your data as follows: On a historical Unix system, type

	your_graphics_program > captured_data.plt

I don't know how this can be done on other operating systems. You can then mail your
captured_data file together with a description of the problem to rricharz77@gmail.com.
Pack it with zip or something else to make sure that the mailing program does not alter it.

If you are registered on github, you can also open an issue.

**Screen resolution**

This tek4010 emulator creates a graphics window of 1024x780 points, which is the display size
of the Tektronix 4010 terminal and the Tektronix 4014 terminal without enhanced graphics module.
The Raspberry Pi can handle
sufficiently high refresh rates at this resolution. This emulator makes an attempt to filter
Tektronix 4014 graphics code with the enhanced graphics module installed, so that such
graphics codes can be displayed using this terminal emulator, but the lowest two bits of
each axis are not used in this case, as in the Tektronix 4014 without the enhanced graphics
module.

If called with the -full option, the tek4010 emulator creates creates a full screen window,
and uses the full 4K resolution of the 4014 with enhanced graphics
module installed, scaled down to the actual window size.
Use control-q to close the tek4010 window. This option is experimental.

**Compiling the tek4010 project**

If you want to compile the project, you need to install "libgtk-3-dev":

	sudo apt-get install libgtk-3-dev

There is a make file in the repo.

**Version**

See [versions.txt](versions.txt)

**Contributors**

The storage tube emulator and the Tektronix 4010/4014 decoder were witten by Rene Richarz.
The ARDS decoder was written by Lars Brinkhoff.
The historical plot data for the Tektronix 4014 was obtained from Jos Dreesen.
Thanks to many others who contributed important ideas and helped with the debugging.


