# Overview

A small X program which provides different mechanisms for manipulating the cursor using the keyboard. The program was originally inspired by the mousekeys feature of Kaleidoscope, the firmware for the Keyboardio.

# Installation

Requires libxi-dev libxinerama-dev, libxft-dev, libxfixes-dev, libxtst-dev, and libx11-dev on debian. You will need to install the equivalent packages containing the appropriate header files on your distribution.

### On Debian:

```
sudo apt-get install libxi-dev libxinerama-dev libxft-dev libxfixes-dev libxtst-dev libx11-dev && make && sudo make install
```

### On Arch:

You can use the aur package: [warpd-git](https://aur.archlinux.org/packages/warpd-git/), or just use your favorite aur helper like:
```
yay -S warpd-git
```

# Demo

## Hint Mode

<p align="center">
<img src="demo_hints.gif" height="400px"/>
</p>

## Grid Mode

<p align="center">
<img src="demo_warp.gif" height="400px"/>
</p>

## Discrete Mode

<p align="center">
<img src="demo_discrete.gif" height="200px"/>
<img src="demo_discrete2.gif" height="200px"/>
</p>

# Quickstart

1. Run `warpd` 

## Grid Mode
2. Press M-x (meta is the command key) to activate the warping process.
3. Use u,i,j,k to repeatedly navigate to different quadrants.
4. Press m to left click, comma to middle click or period to right click.

## Hint Mode
2. Press M-z (meta is the command key) to generate a list of hints
3.  Enter the key sequence associated with the desired target. It will then start discrete mode. Use the discrete movement keys (default hjkl) to adjust the cursor if necessary
4. Press m to left click, comma to middle click or period to right click. You can also hit "v" (as visual mode in vi default keybindings) and the hints will show up again so you can select a text area or drag on the screen.

## Discrete Mode
2. Press M-c (meta is the command key) to activate the discrete cursor control.
3. Use the discrete movement keys (default hjkl) to adjust the cursor if necessary
4. Press m to left click, comma to middle click or period to right click. You can also hit "v" (as visual mode in vi default keybindings) and start selecting text or dragging on the screen by using the movement keys.

## Now fully get rid of your mouse!
5. Hold Alt+e to scroll down and Alt+y to scroll up.
6. Run `man warpd` :P.
7. Edit `~/.warprc` to taste. You can change any of the keybindings, hints sizes and colors amoung other options (listed in the man page or you can take a quick look at [cfg.opts](cfg.opts)).

A more comprehensive description can be found in the man page (along with a list of options).

# Limitations/Bugs

- No multi monitor support (it may still work by treating the entire display as one giant screen, I haven't tried this). If you use this program and desire this feature feel free to harass me via email or file an issue.

- ~~Warp wont activate if the keyboard has already been grabbed by another program (including many popup menus). Using a minimalistic window manager is recommended :P.~~ See [Issue #3](https://github.com/rvaiya/warpd/issues/3#issuecomment-628936249) for details.

- This was a small one off c file that ballooned into a small project, I did not originally plan to publish it. Consequently the code is ugly/will eat your face. You have been warned.

