# Tek4010 Quick Start

This is a Tektronix 4010 graphics terminal emulator.
It is not a full VT100/xterm terminal.

Basic usage:

    tek4010 [options]

---

## Examples

To use the included examples and demos, start tek4010 with the command

    tek4010
  
In the tek4010 window, change to the Tek4010 directory:

    cd Tek4010

Display a plot file:

    cat test.plt

Animation:

    cat animation.plt

Demo script:

    sh demos/demo.sh

---

## Options

By default, the screen clears automatically when the bottom is reached.
Use -noAutoClear to disable this.

| Option   | Description                              |
|----------|------------------------------------------|
| -fast    | Fast rendering without fading            |
| -full    | Full screen mode (Ctrl-Q to close)       |
| -half    | Use half of the screen width.            |
| -b9600…  | Emulate terminal baud rates              |

---

More options and detailed information, including
usage with historical systems, can be found in:

    docs/manual.pdf
