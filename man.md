warpd(1)

# DESCRIPTION

A modal keyboard driven pointer manipulation program.

# SYNOPSIS

warpd [options]

# OPTIONS

	*-f*, *--foreground*: Run warpd in the foreground (i.e do not daemonize). Mainly useful for debugging.

	*-l*, *--list-keys*: Print a list of valid keys which can be used as config values.

	*-v*, *--verbose*: Print the current version.

	*--hint*: Run warpd in oneshot (daemonless) hint mode.

	*--grid*: Run warpd in oneshot grid mode.

	*--normal*: Run warpd in oneshot normal mode.

# DESCRIPTION

warpd has three main modes which can be used to manipulate the pointer. The
primary mode is called 'normal mode' (A-M-c) and facilitates local pointer
movement using vi-like bindings (_h_ _j_ _k_ _l_). The other two modes, *hint*
and *grid* mode are used to effect larger movements across the screen and are
expected to be used in combination with normal mode to achieve the desired end.

For example, the user might activate warpd in hint mode (_A-M-x_) to pinpoint
the start of a text region before starting a drag operation (_v_) and
ultimately using normal mode to complete the selection. If the selection is a
large body of text, it may be desirable to activate grid (_g_) or hint (_x_)
mode for a second time to warp the pointer to the desired region's terminal
point.  

See _CONFIG_OPTIONS_ for a comprehensive list of keys and their corresponding
configuration options (see also _USAGE_NOTES_).

A description of each mode follows:

## Normal Mode (A-M-c)

This is the default mode (and the endpoint of both grid and normal mode) which
is designed for short distance pointer manipulation. It is particularly useful
for manipulating popup menus and selecting text (see _Dragging_). The default
behaviour is vi-like. Pressing the mapped directional keys (default hjkl) moves
the cursor in a continuous fashion, but the pointer can also be warped to the edges
of the screen using the home (_H_), middle (_M_), and last (_L_) mappings (see
_CONFIG OPTIONS_). Finally, a numeric multiplier can be supplied to the
directional keys as an input prefix in order to move the cursor by a
proportional increment in the given direction (e.g 10j moves 10 units down). 

## Hint Mode (A-M-x or simply 'x' within normal mode)

This mode populates the screen with a list of labels and allows the
user to immediately warp the pointer to a given location by pressing the
corresponding key sequence. It is similar to functionality provided by browser
plugins like Vimperator but works outside of the browser and indiscriminately
covers the entire screen. Once a target has been selected 'normal mode' is
entered for further manipulation.

*Note:* While it may at first be tempting to saturate the screen with hints,
the user is cautioned against this. A balance must be struck between hint size,
the number of hints, and the size of the screen. Enough space must be left to
provide contextual awareness, while enough hints must be present to facilitate
targetting UI elements without the need for too much adjustment. Once this
equilibrium has been achieved, using hint mode become second nature and is (in
the author's opinion) superior to grid mode for quickly pinpointing elements.

## Grid Mode ('A-M-g' or simply 'g' within normal mode)

By default grid mode divides the screen into a 2x2 grid. Each time a key
is pressed the grid shrinks to cover the targeted quadrant. The cursor is placed
in the middle of the grid.

covers the desired target a mouse button (e.g 'm') can be pressed.


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

## Screen Selection Mode ('A-M-s' or simply 's' within normal mode)

This mode is intended for multi-screen setups and provides the user with a
dedicated set of hints for switching monitors and dropping them into normal
mode.

## Dragging

Pressing _v_ whilst in normal mode toggles a drag operation. The cursor can
then be warped around the screen as normal in order to select text or move
objects until the drag key is hit again or a mouse button is pressed.
Additionally, the *copy_and_exit* key (_c_) may be used to copy the selected
text to the system clipboard and terminate the current session.

## Wayland

Wayland's architecture does not allow clients to globally bind hotkeys. In
order to run warpd, one of the oneshot flags must be used. These can be bound
to the corresponding hotkeys in your compositor.

E.G

On sway:

```
bindsym Mod4+Mod1+x exec warpd --hint
bindsym Mod4+Mod1+c exec warpd --normal
bindsym Mod4+Mod1+g exec warpd --grid
```

Non-wayland users should favour the daemon, since it also caches some of the
draw operations to improve performance.

# CONFIG OPTIONS

The following configuration options can be placed in *~/.config/warpd/config*
to modify the behaviour of the program. Each option must be specified on its
own line and have the format:

<option>: <value>

All values which contain more than one key are space delimited.

*hint_activation_key*: Activates hint mode. (default: A-M-x).

*grid_activation_key*: Activates grid mode and allows for further manipulation of the pointer using the mapped keys. (default: A-M-g).

*screen_activation_key*: Activate (s)creen selection mode. (default: A-M-s).

*activation_key*: Activate normal movement mode (manual (c)ursor movement). (default: A-M-c).

*hint_oneshot_key*: Activate hint mode and exit upon selection. (default: A-M-l).

*repeat_interval*: The number of milliseconds before repeating a movement event. (default: 20).

*speed*: Pointer speed in pixels/second. (default: 220).

*max_speed*: The maximum pointer speed. (default: 1600).

*acceleration*: Pointer acceleration in pixels/second^2. (default: 700).

*accelerator_acceleration*: Pointer acceleration while the accelerator is depressed. (default: 2900).

*accelerator*: Increase the speed of the pointer while held. (default: a).

*buttons*: A space separated list of mouse buttons (2 is middle click). (default: m , .).

*oneshot_buttons*: Oneshot mouse buttons (deactivate on click). (default: n - /).

*oneshot_timeout*: The length of time in milliseconds to wait for a second click after a oneshot key has been pressed. (default: 300).

*grid_exit*: Exit grid mode and return to normal mode. (default: c).

*hint_exit*: The exit key used for hint mode. (default: esc).

*exit*: Exit the currently active warpd session. (default: esc).

*drag*: Toggle drag mode (mneominc (v)isual mode). (default: v).

*copy_and_exit*: Send the copy key and exit (useful in combination with v). (default: c).

*hint*: Activate hint mode while in normal mode (mnemonic: x marks the spot?). (default: x).

*grid*: Activate (g)rid mode while in normal mode. (default: g).

*screen*: Activate (s)creen selection while in normal mode. (default: s).

*left*: Move the cursor left in normal mode. (default: h).

*down*: Move the cursor down in normal mode. (default: j).

*up*: Move the cursor up in normal mode. (default: k).

*right*: Move the cursor right in normal mode. (default: l).

*cursor_color*: The color of the pointer in normal mode (rgba hex value). (default: #FF4500).

*cursor_size*: The height of the pointer in normal mode. (default: 7).

*top*: Moves the cursor to the top of the screen in normal mode. (default: H).

*middle*: Moves the cursor to the middle of the screen in normal mode. (default: M).

*bottom*: Moves the cursor to the bottom of the screen in normal mode. (default: L).

*start*: Moves the cursor to the leftmost corner of the screen in normal mode. (default: 0).

*end*: Moves the cursor to the rightmost corner of the screen in normal mode. (default: $).

*hist_back*: Move to the last position in the history stack. (default: C-o).

*hist_forward*: Move to the next position in the history stack. (default: C-i).

*grid_nr*: The number of rows in the grid. (default: 2).

*grid_nc*: The number of columns in the grid. (default: 2).

*grid_up*: Move the grid up. (default: w).

*grid_left*: Move the grid left. (default: a).

*grid_down*: Move the grid down. (default: s).

*grid_right*: Move the grid right. (default: d).

*grid_keys*: A sequence of comma delimited keybindings which are ordered bookwise with respect to grid position. (default: u i j k).

*grid_color*: The color of the grid. (default: #1c1c1e).

*grid_size*: The thickness of grid lines in pixels. (default: 4).

*grid_border_size*: The thickness of the grid border in pixels. (default: 0).

*grid_border_color*: The color of the grid border. (default: #ffffff).

*hint_size*: Hint size. Must be a number between 1 and 100 and corresponds to the approximate percentage of horizontal/vertical space occupied by the drawn hints. The default value covers approximately 71% of horizental/vertical space, which corresponds to 50% (the square of 71%) of total screen area. (default: 71).

*hint_bgcolor*: The background hint color. (default: #1c1c1e).

*hint_fgcolor*: The foreground hint color. (default: #a1aba7).

*hint_border_radius*: Border radius. (default: 3).

*scroll_down*: Scroll down key. (default: e).

*scroll_up*: Scroll up key. (default: r).

*screen_chars*: The characters used for screen selection. (default: jkl;asdfg).

*hint_chars*: The character set from which hints are generated. The total number of hints is the square of the size of this string. It may be desirable to increase this for larger screens or trim it to increase gaps between hints. (default: abcdefghijklmnopqrstuvwxyz).

*hint_font*: The font name used by hints. Note: This is platform specific, in X it corresponds to a valid xft font name, on macos it corresponds to a postscript name. (default: Arial).



# USAGE NOTES

The key to using warpd effectively is to learn when to exit normal mode. Much
of one's time at a computer is spent moving the mouse between windows,
interacting with UI elements, and reading text. What one might call 'browse
mode'. It is in this mode of operation that it makes sense to keep warpd
active.

Developing facility with the scroll and oneshot mouse buttons is key to
achieving this. For example, if you happen to have two documents open and wish
to switch between them, you can simply type _x fx_ (where _fx_ is a hint) if
normal mode is active. Scrolling can subsequently be achieved using _e_ and
_r_. Once you finally wish to type something, you can do _x fx n_ to focus on
the UI element, click, and exit.

Conversely, warpd can complement an input heavy workflow with its oneshot
functionality and dedicated activation keys (E.G _n_, _A-M-l_, _A-M-x_, etc).  

It is important to note that warpd is not intended to replace mouse heavy
workflows, and will likely always be inferior for rapid precise local
movements. When confronted with an IDE, or some other pointer driven
abomination, the author still sometimes reaches for his mouse.

## On Dragging

Activating discrete mode and pressing v can provide a familiar environment to a
_vi_ user but it is important to remember that cursor manipulation is
application agnostic and consequently ignorant of the text on the screen. All
movement is necessarily pixel based, consequently, drag + hint
mode can be a superior method for surgically selecting text (though it may at
first be less intuitive).

# BUGS/LIMITATIONS

warpd uses various platform specific hacks to bypass limitations
of the display server. All implementations were written by the
same author, who presently uses X.

Consequently testing on non-X platforms has been minimal.

YMMV

Bugs can be reported here:

https://github.com/rvaiya/warpd/issues/

A list of known limitations follow:

- Multiscreen support currently does not support hotplugging. This means that
  you must restart warpd after making any changes to your screen configuration.

- For implementation reasons, the cursor position is not horizontally centered,
  but to the right of the actual pointer. This generally isn't an issue,
  but may become more noticeable as you increase _cursor_size_.

## X

- Unplugging the keyboard while warpd is one of its active modes will cause
  pandemonium.  If you do this (don't :P), you may need to remotely ssh into
  the machine or switch to a VT to kill the process.

- warpd uses Xinput for input processing to bypass certain limitation of the X
  input system. A byproduct of this is that certain remapping tools will not
  work (e.g xcape). If you are in the habit of making unorthodox changes to
  your keymap (like remapping capslock to control/escape) you may want to try
  an evdev based remapper like keyd (https://github.com/rvaiya/keyd).

- Programs which use Xinput to directly manipulate input devices may misbehave.
  See [Issue #3](https://github.com/rvaiya/warpd/issues/3#issuecomment-628936249) for details.

## MacOS

- Cursor hiding relies on a hack that some programs ignore (e.g iTerm).
- Some programs (e.g iTerm) have a 'secure input mode' (which can usually be
disabled) that causes interference.

## Wayland

- Cursor hiding doesn't work.
- Running as a daemon doesn't work (can't listen for hotkeys).
- UI elements (e.g input fields) which require focus can't be selected.

# AUTHORS

Written and maintained by Raheman Vaiya (2019-).
See https://github.com/rvaiya/warpd for more information.
