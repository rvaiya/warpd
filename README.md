# Overview

A small X program which facilitates recursively warping the pointer to different quadrants on the screen. The program was inspired by the mousekeys feature of Kaleidoscope, a firmware for the Keyboardio.

# Installation

Requires libxfixes-dev, libxtst-dev, and libx11-dev on debian. You will need to install the equivalent packages containing the appropriate header files on your distribution.

```
apt-get install libxfixes-dev libxtst-dev libx11-dev && make && sudo make install
```

## Getting Started

1. Run `warp -d` 
2. Press M-u (meta is the command key) to activate the warping process.
3. Use u,i,j,k to repeatedly navigate to different quadrants.
4. Press m to click.
5. Press enter to finish.
6. RTFM
7. For luck.

# Demo

<img src="demo.gif" height="400px"/>

# Options

 **-k** <key>[,<key>...]: A sequence of comma delimited keybindings which are ordered by their corresponding position in the grid. If '-a' is not specified then the first key is the activation key. Default values are M-k,u,i,j,k,m. Any extraneous keys are interpreted as mouse buttons (left, middle, and right click respectively).

 **-c** <num>: The number of columns in the grid.

 **-r** <num>: The number of rows in the grid.

 **-a**: Runs the warp in 'always active' mode. In this mode the navigation keys are always active and when pressed will immediately begin the warping process. If you use this you will likely want to remap the default keybindings to involve modifiers in order to avoid interference with typing.

 **-d**: Runs warp in the background.

# Examples

## Example 1

Uses a 3 by 3 grid instead of the default 2x2 grid.

```
warp -c 3 -r 3
```

## Example 2
Uses the keybindings Control+uijk instead of the standard uihj for warping, Control+m is the first muse button  and Control+a is the activation key.

```
warp -k C-a,C-u,C-i,C-j,C-k,C-m
```

## Example 3

Starts warp in active mode so that C-u will always immediately move to the first grid element,
C-i will immediately move to the second, and so on.

```
warp -a -k C-u,C-i,C-j,C-k,C-m 

```
# Limitations/Bugs

- No multi monitor support (it may still work by treating the entire display as one giant screen, I haven't tried this). If you use this program and desire this feature feel free to harass me via email or submit a pull request.

- This was a small one off c file that ballooned into a small project, I did not originally plan to publish it. Consequently the code is ugly/will eat your face. You have been warned.
