warpd(1)

# Overview

A modal keyboard driven cursor manipulation program.

# Usage

warpd [-f] [-l] [-v]

# Args

	*-f*: Run warpd in the foreground (i.e do not daemonize). Mainly useful for debugging.

	*-l*: Prints a list of valid keys which can be used as config values.

	*-v*: Prints the current version.

# Overview

warpd has three main modes which can be used to manipulate the cursor. The
primary mode is called 'normal mode' (A-M-c) and facilitates local cursor
movement using vi-like bindings (_h_ _j_ _k_ _l_). The other two modes, *hint*
and *grid* mode are used to effect larger movements across the screen and are
expected to be used in combination with normal mode to achieve the desired end.

For example, the user might activate warpd in hint mode (_A-M-x_) to pinpoint
the start of a text region before starting a drag operation (_v_) and
ultimately using normal mode to complete the selection. If the selection is a
large body of text, it may be desirable to activate grid (_g_) or hint (_x_)
mode for a second time to warp the pointer to the desired region's terminal
point.

A comprehensive description of each mode follows:

## Normal Mode (A-M-c)

This is the default mode (and the endpoint of both grid and normal mode) which
is designed for short distance pointer manipulation. It is particularly useful
for manipulating popup menus and selecting text (see _Dragging_). The default
behaviour is vi-like. Pressing the mapped directional keys (default hjkl) moves
the cursor by a fixed increment but the pointer can also be warped to the edges
of the screen using the home (_H_), middle (_M_), and last (_L_) mappings (see
_Config Options_). Finally, a numeric multiplier can be supplied to the
directional keys as an input prefix in order to magnify movement in the
corresponding direction (e.g 10j moves 10 units down). 

## Hint Mode (A-M-x or simply 'x' within normal mode)

This mode populates the screen with a list of labels and allows the
user to immediately warp the pointer to a given location by pressing the
corresponding key sequence. It is similar to functionality provided by browser
plugins like Vimperator but works outside of the browser and indiscriminately
covers the entire screen. Once a target has been selected 'normal mode' is
entered for further manipulation.

After a bit of practice the process becomes second nature and is (in the
author's opinion) superior to the grid method for quickly pinpointing text and
UI elements.

## Grid Mode ('A-M-g' or simply 'g' within normal mode)

By default grid mode divides the screen into a 2x2 grid. Each time a key
is pressed the grid shrinks to cover the targeted quadrant. The cursor is placed
in the middle of the grid.

covers the the desired target a mouse button (e.g 'm') can be pressed.


E.G

```
         +--------+--------+            +--------+--------+
         |        |        |            |  u |  i |       |
         |   u    |   i    |            |----m----+       |
 M-x     |        |        |     u      |  j |  k |       |
----->   +--------m--------+   ----->   +---------+       |
         |        |        |            |                 |
         |   j    |   k    |            |                 |
         |        |        |            |                 |
         +--------+--------+            +--------+--------+
```

# Dragging

Pressing _v_ whilst in normal mode toggles a drag operation. The cursor can
then be warped around the screen as normal in order to select text or move
objects until the drag key is hit again or a mouse button is pressed.
Additionally, the *copy_and_exit* key (_c_) may be used to copy the selected
text to the system clipboard and terminate the current session.

# Config Options

The following configuration options can be placed in *~/.config/warpd/config* to modify the
behaviour of the program. Each option must be specified on its own line and
have the format:

<option>: <value>

{opts}

# Notes

## On Dragging

Activating discrete mode and pressing v can provide a familiar environment to a
_vi_ user but it is important to remember that cursor manipulation is
application agnostic and consequently ignorant of the text on the screen. All
movement is necessarily pixel based, consequently, drag + hint
mode can be a superior method for surgically selecting text (though it may at
first be less intuitive).

# Limitations/Bugs

- No multi monitor support (it may still work by treating the entire display as
  one giant screen, I haven't tried this). If you desire this feature feel free
  to lobby for it in a github issue.

- Programs which use Xinput to directly manipulate input devices may misbehave.
  See [Issue #3](https://github.com/rvaiya/warpd/issues/3#issuecomment-628936249) 
  for details.
