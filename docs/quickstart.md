# Tek4010 Quick Start

Basic usage:

    tek4010 [options] command [command options]

The command is required, for example: cat, head,
telnet, or rsh.

---

## Examples

To use the included examples and demos, go to the
Tek4010 directory first

Display a plot file:

    tek4010 -noexit cat test.plt

ARDS example:

    tek4010 -noexit -ARDS cat ardsfiles/trek.pic

Text output:

    tek4010 -noexit head -n 32 src/tek4010.c

Animation:

    tek4010 cat animation.plt

Demo script:

    tek4010 demos/demo.sh

---

## Important option

-noexit

Keeps the window open after the command finishes.

---

## Options

| Option   | Description                              |
|----------|------------------------------------------|
| -noexit  | Keep window open after command           |
| -fast    | Fast rendering without fading            |
| -full    | Full screen mode (Ctrl-Q to close)       |
| -b9600…  | Emulate terminal baud rates              |

---

More options and detailed information, including
usage with historical systems, can be found in:

    docs/manual.pdf
