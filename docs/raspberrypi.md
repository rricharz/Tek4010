# Tek4010 on Raspberry Pi

This page describes installation on Raspberry Pi OS.
The same steps usually work on other Linux systems.

---

## Quick installation

Open a terminal and install required packages:

```
sudo apt-get update
sudo apt-get install git libgtk-3-dev make gcc pkg-config
```

Download the source code:

```
git clone https://github.com/rricharz/Tek4010
cd Tek4010
```

Build and install:

```
make clean
make
make install
```

## First test

Run the following while you are in the Tek4010 directory:

```
make check
```

This runs a demonstration and verifies that tek4010 works.

---

## Examples

Many additional examples are available in the tek4010 directory:

- demos/
- pltfiles/
- ardsfiles/

---

## Notes

### Running tek4010

See the quick start guide:

- [docs/quickstart.md](quickstart.md)

### Remote systems

When connecting to historical Unix systems, you may need:

```
sudo apt-get install rsh-client
```

or:

```
sudo apt-get install telnet
```

---

## For experienced users

- The program is built using make
- make install installs the binary in a user-local location

Manual build steps:

```
make clean
make
make install
```

If the system changes in the future, adapt package
installation and build commands as required.
