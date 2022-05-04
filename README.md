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

## Linux

# X

The usual array of X libraries:

 - libxi
 - libxinerama
 - libxft
 - libxfixes
 - libxtst
 - libx11

# Wayland (sway/wlroots only)

 - libwayland-client
 - cairo
 - xkbcommon

*Note:* The wayland port has several limitations due
to the nature of Wayland's architecture.

See the LIMITATIONS section of the man page for more details.

## MacOS:

 - The standard Xcode command line developer tools.

# Installation

Make sure you have the appropriate dependencies for your system:

E.G 

debian/ubuntu (X):

```
sudo apt-get install \
	libxi-dev \
	libxinerama-dev \
	libxft-dev \
	libxfixes-dev \
	libxtst-dev \
	libx11-dev
```

debian/ubuntu (Wayland):

```
sudo apt-get install \
	libcairo2-dev \
	libxkbcommon-dev \
	libwayland-dev
```

macos:

```
xcode-select --install
```

*Note:* Some programs (e.g iTerm) have a 'secure input mode' that may need to be
disabled in order for warpd to work properly.

Then simply do:

```
make && sudo make install
```

or (for macos)

```
PLATFORM=macos make && sudo PLATFORM=macos make install
```

or (for wayland)

```
PLATFORM=wayland make && sudo make install
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

## Wayland

*Note:* Wayland does not permit clients to globally bind hotkeys. These must be
bound within the compositor using warpd's oneshot flags.

E.G

On sway:

```
bindsym Mod4+Mod1+x exec warpd --hint
bindsym Mod4+Mod1+c exec warpd --normal
bindsym Mod4+Mod1+g exec warpd --grid
```

# Packages:

`warpd` is currently available on the following distributions:

## Arch

Available as an [AUR](https://aur.archlinux.org/packages/warpd-git/) package (maintained by Matheus Fillipe).

If you are interesting in adding warpd to your distribution's repository please contact me.

# Limitations/Bugs

- Programs which use Xinput and or Xtest for keyboard may not work correctly
  (e.g synergy). If a specific program which you feel should be working does
  not please file an [issue](https://github.com/rvaiya/warpd/issues).

- The hack used for pointer hiding on OSX doesn't work on some programs (e.g
  iTerm). The original cursor will consequently be visible in such cases,
  though functionality should be otherwise unaffected.

- Wayland support has several limitations (see CAVEATS in the man page).

# Contributions

A special thanks to

 - Pete Fein - For encouragement and early adoption.
 - Matheus Fillipe - For the original border radius patch as well as numerous bug reports and feature requests.
 - The Kaleidoscope/Vimperator projects - For inspiration.
 - Drew Devault - For making the Wayland ecosystem inhabitable.
