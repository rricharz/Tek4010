# Tektronix 4010 Emulator

This is a [Tektronix 4010](https://en.wikipedia.org/wiki/Tektronix_4010) terminal emulator
for the Raspberry Pi.

It attempts to emulate the storage tube display of the Tektronix 4010, including the bright
drawing spot. At the moment, it only supports persistent drawing, but there are plans to
emulate the Tektronix 4014 with its ability for fading objects.

It is currently in alpha-testing and updated daily.

It can be used to log into a historical Unix system such as
[2.11 BSD](https://en.wikipedia.org/wiki/Berkeley_Software_Distribution) on the
[pidp11](http://obsolescence.wixsite.com/obsolescence/pidp-11)
or a real historical system.

Install the tek4010 emulator from this repo on a Raspberry Pi. I propose using

	git clone git://github.com/rricharz/Tek4010
	cd Tek4010

This allows you to get updates later easily as follows:

	cd Tek4010
	git pull

There is a file "captured_data" in the repo, which you can use to test the tek4010 emulator.
"captured_data" was produced in 2.11 BSD using my program "dodekagon". Type

	./tek4010 /bin/cat captured_data -noexit

If you want to test text output, type for example

	./tek4010 /usr/bin/head -n 32 tek4010.c -noexit

Don't forget the absolute path to "cat" or "head" and the LAST argument "-noexit", which tells
tek4010 to stay alive after cat has finished so that you have a chance to look at the output.
By the way, the "-noexit" as the LAST argument might also be helpful if you want to
experiment with other commands. Let me know if you find anything which works and makes sense.
Because tek4010 pipes from stdout of that program and into stdin of that program, some
programs will not work.

The emulator uses "rsh", because historical Unix systems do not support the secure ssh,
and because ssh does not allow using a virtual emulator such as tek4010 for security reasons.
You need therefore to install rsh on the Raspberry Pi running the tek4010 emulator:

	sudo apt-get install rsh-client

**Login in a remote historical Unix system**

This can either be a real historical computer, or a virtual system using simh such
as the PiDP-11.

First, you need to login remotely from your client machine into your historical system, using

	rsh -l user_name system

where "user_name" is the name of the user on the historical Unix system, and "system" is the name
of the system, for example

	rsh -l rene pdp11

If this works properly, you can use the tek4010 emulator. Call it as follows:

	./tek4010 /usr/bin/rsh -l user_name system

In the current alpha-testing version, there are very few useful hints if this does not work.
If the terminal window is closed right away, there is a problem with your rsh call or you
forgot to use the absolute path for rsh.

The following keys are not transmitted to the Unix system, but are executed locally
in the terminal emulator and clear the persistent screen:

	home
	page up
	page down
	control arrow up
	control arrow left

These keys emulate the "page" key of the Tektronix 4010. You need to use one of these
keys frequently to avoid to get a mess on the screen, as on a real Tektronix 4010.

**Login in PiDP11 running on the same Raspberry Pi**

This is really work in progress, but works amazingly well already. It is running
with a screen and keyboard attached to the Raspberry Pi, or almost equally well using
VNC viewer from a laptop!

Expect a bit of slow down from time to time. In my test version the
PiDP11 software and the tek4010 software are using all 4 cores of the Raspberry Pi 3B+ running
at 60% CPU usage! It's amazing how powerful the Raspberry Pi 3B+ is!

You cannot use the tek4010 emulator running screens, as it is done in the standard setup
of the PiDP using the console, because screens filters the output stream of simh and is
therefore unsuitable for graphics terminals such as the tek4010 emulator. If you don't
want to change the standard setup, use control-e to stop simh, and then "exit" to quit simh.

Because tek4010 needs rsh, you need to install rsh-server and rsh-client on
the Raspberry Pi:

	sudo apt-get install rsh-server
	sudo apt-get install rsh-client

Now start tek4010 as follows:

	./tek4010 /usr/bin/rsh -l pi localhost

This should give you a login prompt into your Raspberry Pi. If not, test the rsh call first.

Once your password has been accepted, be prepared to use the "home" key or any of the other
keys described above frequently to avoid to get a mess on the dump 4010 terminal emulator!
The following will start the PiDP software:

	cd /opt/pidp11/bin
	./pidp11.sh

Everything should run as expected, and you should be able to use the tek4010 terminal emulator with any of
the historical operating systems. It has not yet been tested on other systems than 2.11 BSD.
Your feedback to rricharz77@gmail.com or the PiDP11 forum is therefore very much appreciated.

One word of caution! If you run the PiDP11 software this way without using screens, you SHOULD
NOT detach or quit the terminal while your historical operating system is running, because
this will kill the PiDP11 simh emulator right away. First run down your historical operating
system and simh properly, before detaching the terminal emulator!

**Reporting problems**

As this software is still under development, there will be problems. I just do not have enough
programs doing graphics to properly test the program.

If everything works properly for you, but your graphics application produces garbage on the
tek4010 emulator, you could send me your data as follows: On a historical Unix system, type

	your_graphics_program > captured_data

I don't know how this can be done on other operating systems. You can then mail your
captured_data file together with a description of the problem to rricharz77@gmail.com.
Pack it with zip or something else to make sure that the mailing program does not alter it.	 

**Compiling the tek4010 project**

If you want to compile the project, you need to install "libgtk-3-dev":

	sudo apt-get install libgtk-3-dev

There is a make file in the repo.


