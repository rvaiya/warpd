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

This is an mode which populates the screen with a list of labels and
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

## Discrete Mode (M-c)

This is an auxiliary movement mode designed for short distance pointer
manipulation. It is the end stage of hint mode and grid mode (if button2 is
pressed) and is particularly useful for manipulating things like popup menus
and selecting text using the drag functionality (see **Dragging**). The default
behaviour is vi-like. Pressing the mapped directional keys (default hjkl) moves
the cursor by a fixed increment but the pointer can also be warped to the edges
of the screen using the home, middle, and last mappings (see **Config
Options**). Finally a numeric multiplier can be supplied to the directional
keys as an input prefix in order to affect movement in the corresponding
direction (e.g 10j moves 10 units down). 

# Dragging

Simulating a drag operation can be done by activating one of the movement
modes, moving to the desired starting location and pressing the drag activation
key (default v). At this point the same mode will be reopened to facilitate
destination selection. Warp will then simulate dragging the source to the
destination along the shortest linear path. The end result provides a concise
and reasonably flexible mechanism for simulating common mouse operations.
Paired with discrete mode, the feature can be useful for highlighting text,
while dragging and dropping a file between windows might better be acomplished
by using the feature in conjunction with grid or hint mode. 

## Notes

Activating discrete mode and pressing v can provide a familiar environment to a
*vi* user but it is important to remember that pointer manipulation is
application agnostic and consequently ignorant of the text on the screen. All
movement is necessarily based on *movement_increment*. consequently drag + hint
mode can be a superior method for surgically selecting text (though it may at
first be less intuitive).

# Config Options

The following configuration options can be placed in ~/.warprc to modify the behaviour of the program. Each option must be specified
on its own line and have the format 

```
<option>: <value>
```

{opts}

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
