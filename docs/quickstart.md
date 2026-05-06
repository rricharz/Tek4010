# Tek4010 Quick Start

This is a Tektronix 4010/4014 graphics terminal emulator.
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
    
If typing feels slow, start tek4010 with the -fast option.

Without -fast, tek4010 will automatically switch to fast mode when it
detects slow graphics response. This may take a while.

---

## Keyboard

The following keys are handled locally by tek4010 and are not passed
to the child process or remote system.

Clear screen

    Mac:       Fn-Left Arrow
    Linux:     Home
    Windows:   Home

Break

    Mac:       Left Option-B
    Linux:     Alt-B
    Windows:   Alt-B

Quit

    Window close button
    Ctrl-D

APL mode (only if APL support enabled)

    Left Option-N    switch to APL character set
    Left Option-O    switch back to normal character set

All normal ASCII keys and control characters (Ctrl-C, Ctrl-D, etc.)
are passed through unchanged.

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
