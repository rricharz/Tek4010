# Tek4010 on Windows

This page describes installation on Windows using WSL
(Windows Subsystem for Linux) with Ubuntu.

---

## Quick installation

Open PowerShell and type:

```
wsl --install -d Ubuntu
```

Reboot your system.

Start Ubuntu from the Start menu.

Install required packages:

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

---

## First test

Run:

```
make check
```

This runs a demonstration and verifies that tek4010 works.

---

## Notes

### Running tek4010

See the quick start guide:

- [docs/quickstart.md](quickstart.md)

### Ubuntu setup

When Ubuntu starts for the first time, it will ask you
to choose a user name and password.

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
