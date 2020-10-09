% WARP(1)
% Raheman Vaiya

# Overview

A small X program which facilitates warping the pointer using the keyboard.

# Usage

warpd [-f] [-l] [-v]

# Args

 **-f**: Run warpd in the foreground (i.e do not daemonize).Mainly useful for debugging.

 **-l**: Prints a list of valid keys which can be used as config values.

 **-v**: Prints the current version.

# Overview

Warp has several modes which can be used to control the keyboard. The two main
modes are *grid mode* and *hint mode* which are used for long distance cursor
manipulation. In addition to grid and hint mode there also exists a dedicated
mode for short distance pointer manipulation called *discrete mode* which
has vi-like keybindings and also serves as the end point of hint mode
(to facilitate more precise control).


## Grid Mode (M-x)

By default grid mode divides the screen into a 2x2 grid. Each time a key
is pressed the grid shrinks to cover the targeted area. Once the pointer
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

This mode populates the screen with a list of labels and allows the
user to immediately warp the pointer to a given location by pressing the
corresponding key sequence. It is similar to functionality provided by browser
plugins like Vimperator but works outside of the browser and indiscriminately
covers the entire screen. 

By tweaking `hints_nc` and `hints_nr` it should be possible to make most screen
locations accessible with 2-4 key strokes. After a bit of practice the process becomes
second nature and is (in the author's opinion) superior to the grid method for
quickly pinpointing text and UI elements.


## Discrete Mode (M-c)

This is an auxiliary movement mode designed for short distance pointer
manipulation. It is the end stage of hint mode mode and is particularly useful
for manipulating things like popup menus and selecting text using the drag
functionality (see **Dragging**). The default behaviour is vi-like. Pressing
the mapped directional keys (default hjkl) moves the cursor by a fixed
increment but the pointer can also be warped to the edges of the screen using
the home, middle, and last mappings (see **Config Options**). Finally a numeric
multiplier can be supplied to the directional keys as an input prefix in order
to affect movement in the corresponding direction (e.g 10j moves 10 units
down). 

# Dragging

Simulating a drag operation can be done by activating one of the movement
modes, moving to the desired starting location and pressing the drag activation
key (default v). At this point the same mode will be reopened to facilitate
destination selection. warpd will then simulate dragging the source to the
destination along the shortest linear path. The end result provides a concise
and reasonably flexible mechanism for simulating common mouse operations.
Paired with discrete mode, the feature can be useful for highlighting text,
while dragging and dropping a file between windows might better be accomplished
by using the feature in conjunction with grid or hint mode. 

# Scrolling

Inertial scroll can be activated by double tapping the desired scroll key
(buttons 4/5). This is the analogue of 'flinging' the cursor on most trackpads.
Once inertial scroll has been activated (by double tapping) an impulse can be
imparted to the scrolling cursor by tapping the same key. This feature is
particularly useful for navigating a lot of content (e.g long web pages) but
can effectively be disabled by setting `scroll_fling_timeout` to 1 if desired
(see Config Options).

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

**buttons**: Keys corresponding to mouse buttons (4/5 == scroll down/scroll up). (default: m,comma,period,x,c).

**exit**: The key used to prematurely terminate the current movement session. (default: Escape).

**double_click_timeout**: The length of time (in ms) before warp automatically deactivates after the last click. 0 disables automatic deactivation entirely. This option only applies to button 1 (left click). (default: 300).

**drag_key**: Drag mode activation key. (default: v).

**grid_nr**: The number of rows in the grid. (default: 2).

**grid_nc**: The number of columns in the grid. (default: 2).

**grid_up**: Move the entire grid up by a fixed interval. (default: w).

**grid_left**: Move the entire grid left by a fixed interval. (default: a).

**grid_down**: Move the entire grid down by a fixed interval. (default: s).

**grid_right**: Move the entire grid right by a fixed interval. (default: d).

**grid_keys**: A sequence of comma delimited keybindings which are ordered bookwise with respect to grid position. (default: u,i,j,k).

**grid_color**: The color of the grid. (default: #ff0000).

**grid_mouse_color**: The color of the mouse indicator. (default: #00ff00).

**grid_pointer_size**: The size of the grid pointer. (default: 20).

**grid_line_width**: The size of grid lines. (default: 5).

**hint_width**: The height (in pixels) of a hint. (default: 40).

**hint_height**: The width (in pixels) of a hint. (default: 30).

**hint_up**: Moves the cursor up by movement_increment once a label has been selected in hint mode. (default: k).

**hint_left**: Moves the cursor left by movement_increment once a label has been selected in hint mode. (default: h).

**hint_down**: Moves the cursor down by movement_increment once a label has been selected in hint mode. (default: j).

**hint_right**: Moves the cursor right by movement_increment once a label has been selected in hint mode. (default: l).

**hint_bgcolor**: The background hint color. (default: #00ff00).

**hint_fgcolor**: The foreground hint color. (default: #000000).

**hint_characters**: The set of hint characters used by hint mode. (default: asdfghjkl;'zxcvbm,./).

**hint_opacity**: Hint transparency (requires a compositor). (default: 100).

**discrete_left_word**: Move the cursor left by word size in discrete mode. (default: b).

**discrete_right_word**: Move the cursor right by word size in discrete mode. (default: w).

**discrete_down_word**: Move the cursor down by word size in discrete mode. (default: S-j).

**discrete_up_word**: Move the cursor up by word size in discrete mode. (default: S-k).

**discrete_left**: Move the cursor left in discrete mode. (default: h).

**discrete_down**: Move the cursor down in discrete mode. (default: j).

**discrete_up**: Move the cursor up in discrete mode. (default: k).

**discrete_right**: Move the cursor right in discrete mode. (default: l).

**discrete_color**: The color of the pointer in discrete mode. (default: #00ff00).

**discrete_size**: The height (in pixels) of the pointer n discrete mode. (default: 20).

**discrete_home**: Moves the cursor to the top of the screen in discrete mode. (default: S-h).

**discrete_middle**: Moves the cursor to the middle of the screen in discrete mode. (default: S-m).

**discrete_last**: Moves the cursor to the bottom of the screen in discrete mode. (default: S-l).

**discrete_beginning**: Moves the cursor to the leftmost corner of the screen in discrete mode. (default: S-6).

**discrete_end**: Moves the cursor to the rightmost corner of the screen in discrete mode. (default: S-4).

**discrete_word_size**: Size of the increment used by discrete_*_word. (default: 80).

**scroll_fling_timeout**: Double tap timeout used to determine fling intention. (default: 150).

**scroll_fling_velocity**: Initial fling velocity. (default: 40).

**scroll_fling_acceleration**: Fling acceleration. (default: 10).

**scroll_fling_deceleration**: Fling deceleration. (default: 30).

**scroll_velocity**: Scroll velocity. (default: 10).

**scroll_acceleration**: Scroll acceleration. (default: 30).

**scroll_down_key**: Scroll down key. (default: A-e).

**scroll_up_key**: Scroll up key. (default: A-y).



# Hint Generation

In addition to *hint_characters* hints can be explicitly specified in `~/.warprc_hints`, if this file exists it is automatically used as a hint source. Each line contains a hint which is used to populate the screen and may optionally contain spaces. Spaces are aesthetic and not used during matching. Note that unlike *hint_characters* (from which hints are generated) this file contains all of the hints that will be shown on the screen. If hints in this file are longer than two characters you will likely also want to alter *hint_width*.


# Examples

## Grid Modification

The following ~/.warprc causes warpd to use a 3 by 3 grid instead of the default 2 by 2 grid with u,i,o corresponding to the columns in the top row and n,m,comma corresponding to the columns in the bottom row. The left, middle and right buttons can be clicked by pressing z, x, and c respectively.

```
grid_nr: 3
grid_nc: 3
grid_keys: u,i,o,j,k,l,n,m,comma
buttons: z,x,c
```

## Hint Specification

The following command might be used to generate a hint file that replicates the default behaviour of warpd.

```
printf "abcdefghijklmnopqrstuvwxyz"| \
awk -vFS='' '{for(j=1;j<=NF;j++) \
		for(i =1;i<=NF;i++) \
			print $i$j}' > ~/.warprc_hints
```

# Notes

## Hint Mode

Rather than saturating the screen with labels it is recommended that the user
leave a few gaps and then use the movement keys (hjkl) to move to the final
location. The rationale for this is as follows: 

- **Efficiency**: A large number of labels necessitates the use of longer keysequences (since there are a finite number of two-key sequences) at which point the value of the label system is supplanted by manual mouse movement.

- **Usability**: Packing every inch of the screen with labels causes a loss of context by obscuring UI elements.

## On Dragging

Activating discrete mode and pressing v can provide a familiar environment to a
*vi* user but it is important to remember that pointer manipulation is
application agnostic and consequently ignorant of the text on the screen. All
movement is necessarily based on *movement_increment*. consequently drag + hint
mode can be a superior method for surgically selecting text (though it may at
first be less intuitive).


# Limitations/Bugs

- No multi monitor support (it may still work by treating the entire display as one giant screen, I haven't tried this). If you use this program and desire this feature feel free to harass me via email or file an issue on github.

- Programs which use Xinput to directly manipulate input devices may misbehave. See [Issue #3](https://github.com/rvaiya/warpd/issues/3#issuecomment-628936249) for details.
