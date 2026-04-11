# Tek4010 on macOS

This page describes installation on macOS.

---

## Quick installation

Open a terminal and install required tools:

```
xcode-select --install
```

Install MacPorts from:

https://macports.org/install.php

Reboot your Mac.

Install required packages:

```
sudo port install gtk3-devel git cairo pkgconfig
```

Install XQuartz from:

https://xquartz.org

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

## Alternative using Homebrew

Instead of MacPorts, you can use Homebrew:

```
brew install gtk+3 git cairo pkgconfig
```

---

## Notes

### Running tek4010

See the quick start guide:

- [docs/quickstart.md](quickstart.md)

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
