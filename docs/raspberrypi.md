# Tek4010 on Raspberry Pi

This page describes installation on Raspberry Pi OS.
The same steps usually work on other Linux systems.

---

## Quick installation

Open a terminal and install required packages:

sudo apt-get update
sudo apt-get install git libgtk-3-dev make gcc pkg-config

Download the source code:

git clone https://github.com/rricharz/Tek4010
cd Tek4010

Build and install:

make clean
make
make install

If this is your first installation, you may reboot:

sudo reboot

---

## First test

Go to the directory containing test.plt and run:

tek4010 -noexit cat test.plt

You should see a graphical display.

The display speed can be adjusted if needed.

---

## Examples

Additional examples are available in the repository:

- demos/demo.sh
- pltfiles/
- ardsfiles/

---

## Notes

Keeping the window open:

The option -noexit keeps the window open after the command has finished.

Remote systems:

When connecting to historical Unix systems, you may need rsh or telnet:

sudo apt-get install rsh-client

or

sudo apt-get install telnet

---

## For experienced users

- The program is built using make
- make install installs the binary in a user-local location
- Ensure that this location is included in your PATH

Manual build steps:

make clean
make
make install

If the system changes in the future, adapt package installation
and build commands as required.
