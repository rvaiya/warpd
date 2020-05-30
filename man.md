% WARP(1)
% Raheman Vaiya

# Overview

A small X program which facilitates warping the pointer using the keyboard.

# Usage

warp [-d] [-l] [-v]

# Args

 **-d**: Run warp in the background.

 **-l**: Prints a list of valid keys which can be used as config values.

 **-v**: Prints the current version.

# Overview

Warp has several modes which can be used to control the keyboard. The two main
modes are *grid mode* and *hint mode* which are used to position the cursor
before the user is dropped in to *discrete mode* (either by pressing a mouse
button in the case of grid mode or by selecting a label in the case of hint
mode) for further manipulation.  While in discrete mode the user can move
the cursor by a fixed (configurable) interval by using the discrete movement
keys (hjkl) and finally simulate a mouse click by pressing one of `buttons`
(default: m, comma, period). discrete mode can also be activated and used on
its own to facilitate short distance pointer control by pressing its dedicated
activation key (see **Config Options**).


## Grid Mode (M-x)

By default grid mode divides the screen into a 2x2 grid. Each time a key
is pressed the grid shrinks to cover the targetted area. Once the pointer
covers the target `m` can be pressed to simulate a mouse click.


E.G

```
         +--------+--------+            +--------+--------+
         |        |        |            |  u |  i |       |
         |   u    |   i    |            |----m----+       |
 M-x     |        |        |     u      |  j |  k |       |
----->   +-----------------+   ----->   +---------+       |
         |        |        |            |                 |
         |   j    |   k    |            |                 |
         |        |        |            |                 |
         +--------+--------+            +--------+--------+
```


## Hint Mode (M-z)

This is an experimental mode which populates the screen with a list of labels and
allows the user to immediately warp the pointer to a given location by pressing
the corresponding key sequence. It is similar to functionality provided by
browser plugins like Vimperator but works outside of the browser and
indiscriminately covers the entire screen. 

### Notes

Rather than saturating the screen with labels it is recommended that the user
leave a few gaps and then use the movement keys (hjkl) to move to the final
location. The rationale for this is as follows: 

- **Efficiency**: A large number of labels necessitates the use of longer keysequences (since there are a finite number of two-key sequences) at which point the value of the label system is supplanted by manual mouse movement.

- **Usability**: Packing every inch of the screen with labels causes a loss of context by obscuring UI elements.

By tweaking `hints_nc` and `hints_nr` it should be possible to make most screen
locations accessible with 2-4 key strokes. After a bit of practice this becomes
second nature and is (in the author's opinion) superior to the grid method for
quickly pinpointing text and UI elements.

# Config Options

The following configuration options can be placed in ~/.warprc to modify the behaviour of the program. Each option must be specified
on its own line and have the format 

```
<option>: <value>
```

**hint_activation_key**: Activates hint mode. (default: M-z).

**grid_activation_key**: Activates grid mode and allows for further manipulation of the pointer using the mapped keys. (default: M-x).

**discrete_activation_key**: Activate discrete movement mode (manual cursor movement). (default: M-c).

**movement_increment**: The movement increment used for grid and discrete mode. (default: 20).

**buttons**: The number of columns in the grid. (default: m,comma,period).

**double_click_timeout**: The length of time (in ms) before warp automatically deactivates after the last click. 0 disables automatic deactivation entirely. This option only applies to button 1 (left click). (default: 300).

**grid_nr**: The number of rows in the grid. (default: 2).

**grid_nc**: The number of columns in the grid. (default: 2).

**grid_up**: Move the entire grid up by a fixed interval. (default: w).

**grid_left**: Move the entire grid left by a fixed interval. (default: a).

**grid_down**: Move the entire grid down by a fixed interval. (default: s).

**grid_right**: Move the entire grid right by a fixed interval. (default: d).

**grid_keys**: A sequence of comma delimited keybindings which are ordered bookwise with respect to grid position. (default: u,i,j,k).

**grid_col**: The color of the grid. (default: #ff0000).

**grid_mouse_col**: The color of the mouse indicator. (default: #00ff00).

**grid_pointer_size**: The size of the grid pointer. (default: 20).

**grid_line_width**: The size of grid lines. (default: 5).

**hint_width**: The height (in pixels) of a hint. (default: 40).

**hint_height**: The width (in pixels) of a hint. (default: 30).

**hint_up**: Moves the cursor up by movement_increment once a label has been selected in hint mode. (default: k).

**hint_left**: Moves the cursor left by movement_increment once a label has been selected in hint mode. (default: h).

**hint_down**: Moves the cursor down by movement_increment once a label has been selected in hint mode. (default: j).

**hint_right**: Moves the cursor right by movement_increment once a label has been selected in hint mode. (default: l).

**hint_bgcol**: The background hint color. (default: #00ff00).

**hint_fgcol**: The foreground hint color. (default: #000000).

**hint_characters**: The set of hint characters used by hint mode. (default: asdfghjkl;'zxcvbm,./).

**hint_opacity**: Hint transparency (requires a compositor). (default: 100).

**discrete_left**: Move the cursor left in discrete mode. (default: h).

**discrete_down**: Move the cursor down in discrete mode. (default: j).

**discrete_up**: Move the cursor up in discrete mode. (default: k).

**discrete_right**: Move the cursor right in discrete mode. (default: l).

**discrete_indicator_color**: The color of the discrete status indicator (default: #00ff00).

**discrete_indicator_size**: The height (in pixels) of the discrete status indicator (default: 30).



# Examples

The following ~/.warprc causes warp to use a 3 by 3 grid instead of the default 2 by 2 grid with u,i,o corresponding to the columns in the top row and n,m,comma corresponding to the columns in the bottom row. The left, middle and right buttons can be clicked by pressing z, x, and c respectively.

```
grid_nr: 3
grid_nc: 3
grid_keys: u,i,o,j,k,l,n,m,comma
buttons: z,x,c
```

# Limitations/Bugs

- No multi monitor support (it may still work by treating the entire display as one giant screen, I haven't tried this). If you use this program and desire this feature feel free to harass me via email or file an issue on github.

- Programs which use Xinput to directly manipulate input devices may misbehave. See [Issue #3](https://github.com/rvaiya/warp/issues/3#issuecomment-628936249) for details.
