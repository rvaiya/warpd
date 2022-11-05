# v1.3.5
- macos: Various input bugfixes
- macos: Introduce launchd service (the user should no longer run warpd explicitly)
- macos: Fix multi-user support
- linux: Merge X/Wayland support into a single binary
- Improve portrait mode support
- Make the drag button configurable
- Fix config key precedence bug

# v1.3.4
 - Fix initial pointer position on wayland
 - Add support middle click support for macos
 - Add support for platform keymaps (non-qwerty layouts)
 - Various bugfixes

# v1.3.3
 - Fix wayland initialization issue

# v1.3.2
 - Improve config handling (allow for shadowed values to gracefully handle conflicts)
 - Make undo keys in hint based modes configurable

# v1.3.1

 - Allow secondary mode selection with --oneshot --normal.
 - Add identifying information to print keys and -q
 - Fix -c

# v1.3.0

 - Add more scripting facilities (e.g --oneshot --record)
 - Allow oneshot activation while the daemon is running
 - Fix -q flag
 - Add print key
 - Make hint size specification consistent
 - Add --query
 - Fix dialog occlusion issue (#134)
 - Implement 2 stage hint mode
 - Add history mode
 - Fix modified button presses (#86)
 - Fix macos compilation issues
 - Add an optional visual indicator for normal mode (#121)
 - Check for zxdg_output_manager before adding screens (#116)
 - Make scroll speed configurable (#112)
 - wayland: Make font configurable (#137)
 - wayland: Fix multi-screen support (#135)
 - Various bugfixes

# v1.2.2

 - Use XDG compliant config paths + eliminate ~/.config pollution
 - wayland: Lower the required layer_shell version
 - Eliminate hint asymmetry (alters the unit of hint_size)
 - Add dedicated `hint_exit` key (#99)
 - Add --config, and --help
 - Misc bugfixes

# v1.2.1

 - Add multi-screen support to wayland
 - Add dedicated hint_exit key (distinct from exit)

# v1.2.0

 - Add experimental wayland support
 - Add --hint/--normal/--grid flags
 - Fix missing key up bug

# v1.1.4-beta

 - Fix alternate (english) layout support

# v1.1.3-beta

 - Hide cursor when scrolling
 - Fix full hint drawing issue caused by switching VTs

# v1.1.2-beta

 - Add support for modified button presses
 - Add accelerator key

# v1.1.1-beta

 - Add support for shifted hint characters
 - Fix screen boundary issue
 - Limit maximum hint size for multi-screen/resolution setups
 - Change the config list delimiter from comma to space
 - Update man page
 - Misc bugfixes

# v1.1.0-beta

 - Add Multiscreen support
 - Make the default colour scheme less appalling
 - Make other modes accessible from grid mode

# v1.0.3-beta

 - macos: solicit accessibility whitelisting on initialization (#64)
 - macos: cleanup input handling code + fix freezing (#65)
 - Account for different X maps

# v1.0.2-beta

 - macos: Fix sticky key visual indicators
 - Add pointer acceleration (`acceleration`)
 - Add `grid_border_size` and `grid_border_color`
 - Add `hint_oneshot_key`
 - Show cursor in grid mode

# v1.0.0-beta

 - Add MacOS support
 - Internal code cleanup
 - Normal movement keys are now continuous rather than discrete.
 - Remove 'normal_' prefix from normal options.
 - Eliminate hint opacity variable in favour of rgba hex values.
 - Eliminate scroll flinging

# v0.0.1

In the beginning there was only darkness.

