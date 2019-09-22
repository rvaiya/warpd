% WARP(1)
% Raheman Vaiya

# Overview

A small X program which facilitates warping the pointer using the keyboard.

# Usage

warp [-a] [-k \<first grid element key\>, \<second grid element key\>..] [-c \<num\>] [-r \<num\>]

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
