# What

A modal keyboard driven interface for mouse manipulation.

# Demo

## Hint Mode `Alt-Meta-x`

<p align="center">
<img src="demo/hints.gif" height="400px"/>
</p>

## Grid Mode `Alt-Meta-g`

<p align="center">
<img src="demo/warp.gif" height="400px"/>
</p>

## Normal Mode `Alt-Meta-c`

<p align="center">
<img src="demo/discrete.gif" height="400px"/>
<img src="demo/discrete2.gif" height="400px" width="711px"/>
</p>


# Dependencies

## Linux (X only):

The usual array of X libraries:

 - libxi
 - libxinerama
 - libxft
 - libxfixes
 - libxtst
 - libx11

Wayland is currently unsupported and may or may not be in
the pipeline :P.

## MacOS:

 - The standard Xcode command line developer tools.

# Installation

Make sure you have the appropriate dependencies for your system:

E.G 

debian/ubuntu:

```
sudo apt-get install \
	libxi-dev \
	libxinerama-dev \
	libxft-dev \
	libxfixes-dev \
	libxtst-dev \
	libx11-dev
```


macos:

```
xcode-select --install
```

Then simply do:

```
make && sudo make install
```

# Quickstart

1. Run `warpd` 

## Hint Mode
2. Press `A-M-x` (`alt+meta+x`) to generate a list of hints
3. Enter the key sequence associated with the desired target to warp the pointer to that location and enter normal mode.
4. Use the normal mode movement keys to select the final desination (see Normal Mode). 

## Grid Mode
2. Press `A-M-g` (meta is the command key) to activate the warping process.
3. Use `u`,`i`,`j`,`k` to repeatedly navigate to different quadrants.
4. Press `m` to left click, `,` to middle click or `.` to right click. 
5. See Normal Mode

## Normal Mode
2. Press `A-M-c` to activate normal mode.
3. Use the normal movement keys (default `hjkl`) to adjust the cursor.
4. Press `m` to left click, `,` to middle click or `.` to right click. 
5. Press `escape` to quit.

A drag movement can be simulated from any of the above modes by focusing on the
source and then pressing the `drag_key` (default `v`) which will cause normal
mode to be activated for selection of the drag target.

A more comprehensive description can be found in the [man page](man.md) (along with a list of options).

# Packages:

`warpd` is currently available on the following distributions:

## Arch

Available as an [AUR](https://aur.archlinux.org/packages/warpd-git/) package (maintained by Matheus Fillipe).

If you are interesting in adding warpd to your distribution's repository please contact me.

# Limitations/Bugs

- Programs which use Xinput and or Xtest for keyboard may not work correctly
  (e.g synergy). If a specific program which you feel should be working does
  not please file an [issue](https://github.com/rvaiya/warpd/issue).

- The hack used for pointer hiding on OSX doesn't work on some programs (e.g
  iTerm). The original cursor will consequently be visible in such cases,
  though functionality should be otherwise unaffected.

# Contributions

A special thanks to

 - Pete Fein - For encouragement and early adoption.
 - Matheus Fillipe - For the original border radius patch as well as numerous bug reports and feature requests.
 - The Kaleidoscope/Vimperator projects - For inspiration.
