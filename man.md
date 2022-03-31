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

_Note:_ All pixel based values are relative to a 1920x1080 virtual screen and
will be scaled according to the current dimensions.

*hint_activation_key*: Activates hint mode. (default: A-M-x).

*grid_activation_key*: Activates grid mode and allows for further manipulation of the pointer using the mapped keys. (default: A-M-g).

*activation_key*: Activate normal movement mode (manual (c)ursor movement). (default: A-M-c).

*hint_oneshot_key*: Activate hint mode and exit upon selection. (default: A-M-l).

*repeat_interval*: The number of miliseconds before repeating a movement event. (default: 20).

*speed*: Pointer speed in pixels/second. (default: 220).

*max_speed*: The maximum pointer speed. (default: 800).

*acceleration*: Pointer acceleration in pixels/second^2. (default: 700).

*buttons*: Mouse buttons (2 is middle click). (default: m, comma, .).

*oneshot_buttons*: Oneshot mouse buttons (deactivate on click). (default: n, -, /).

*oneshot_timeout*: The length of time in miliseconds to wait for a second click after a oneshot key has been pressed. (default: 300).

*exit*: Exit the currently active warpd session. (default: esc).

*drag*: Toggle drag mode (mneominc (v)isual mode). (default: v).

*copy_and_exit*: Send the copy key and exit (useful in combination with v). (default: c).

*hint*: Activate hint mode while in normal mode (mnemonic: x marks the spot?). (default: x).

*grid*: Activate (g)rid mode while in normal mode. (default: g).

*left*: Move the cursor left in normal mode. (default: h).

*down*: Move the cursor down in normal mode. (default: j).

*up*: Move the cursor up in normal mode. (default: k).

*right*: Move the cursor right in normal mode. (default: l).

*cursor_color*: The color of the pointer in normal mode (rgba hex value). (default: #00ff00).

*cursor_size*: The height of the pointer in normal mode. (default: 7).

*top*: Moves the cursor to the top of the screen in normal mode. (default: S-h).

*middle*: Moves the cursor to the middle of the screen in normal mode. (default: S-m).

*bottom*: Moves the cursor to the bottom of the screen in normal mode. (default: S-l).

*start*: Moves the cursor to the leftmost corner of the screen in normal mode. (default: 0).

*end*: Moves the cursor to the rightmost corner of the screen in normal mode. (default: S-4).

*hist_back*: Move to the last position in the history stack. (default: C-o).

*hist_forward*: Move to the next position in the history stack. (default: C-i).

*grid_nr*: The number of rows in the grid. (default: 2).

*grid_nc*: The number of columns in the grid. (default: 2).

*grid_up*: Move the grid up. (default: w).

*grid_left*: Move the grid left. (default: a).

*grid_down*: Move the grid down. (default: s).

*grid_right*: Move the grid right. (default: d).

*grid_keys*: A sequence of comma delimited keybindings which are ordered bookwise with respect to grid position. (default: u,i,j,k).

*grid_color*: The color of the grid. (default: #ff0000).

*grid_size*: The thickness of grid lines in pixels. (default: 4).

*grid_border_size*: The thickness of the grid border in pixels. (default: 0).

*grid_border_color*: The color of the grid border. (default: #ffffff).

*hint_size*: Hint size. (default: 28).

*hint_bgcolor*: The background hint color. (default: #1c1c1e).

*hint_fgcolor*: The foreground hint color. (default: #a1aba7).

*hint_border_radius*: Border radius. (default: 3).

*scroll_down*: Scroll down key. (default: e).

*scroll_up*: Scroll up key. (default: r).

*hint_chars*: The character set from which hints are generated. The total number of hints is the square of the size of this string. It may be desirable to increase this for larger screens or trim it to increase gaps between hints. (default: abcdefghijklmnopqrstuvwxyz).

*hint_font*: The font name used by hints. Note: This is platform specific, in X it corresponds to a valid xft font name, on macos it corresponds to a postscript name. (default: Arial).



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

- Unplugging the keyboard while warpd is one of its active modes will cause
  pandemonium.  If you do this (don't :P), you may need to remotely ssh into
  the machine or switch to a VT to kill the process.

- warpd uses Xinput for input processing to bypass certain limitation of the X
  input system. A byproduct of this is that certain remapping tools will not
  work (e.g xcape). If you are in the habit of making unorthodox changes to
  your keymap (like remapping capslock to control/escape) you may want to try
  an evdev based remapper like keyd (https://github.com/rvaiya/keyd).
