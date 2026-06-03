# FroxComposband

## Disclaimer

FroxComposband may ruin your social life, work productivity, or daily exercise
routine. You play the game at your own risk; in no event shall the
FroxComposband authors owe you a new keyboard, or be liable to you for any
other direct, indirect, punitive, magical, or other injuries or damages of any
nature whatsoever.

## What is FroxComposband?

FroxComposband is a single-player roguelike dungeon-crawler descended from
Angband and Moria, and much more recently from the excellent FrogComposband.
It keeps Frog's impressive class, race, monster, dungeon, and item variety,
while incorporating over 50 fixes and dozens of quality-of-life updates and
UI improvements, along with compatibility with and import of saves from older
FrogComposband versions.

## How to install

### Main download page is currently https://ask.band/frox - no need to build it yourself! 🤓👍️

### Manual build instructions for the nerdy:

### Linux

Instructions originally by Chris Kousky, lightly edited.

Download and unpack the source archive, or clone the git repository:

```sh
git clone https://github.com/gliktch/froxcomposband.git
```

Make sure you have the appropriate development packages installed. For example,
on Ubuntu or Mint:

```sh
sudo apt-get install autoconf gcc libc6-dev libncurses-dev libx11-dev
```

If that does not work on your distro, try `libncursesw6-dev` instead.

From the root of the source archive:

```sh
sh autogen.sh
./configure
make clean
make
```

To install, you may need to elevate your credentials:

```sh
sudo make install
```

or:

```sh
su
make install
exit
```

Then run FroxComposband as desired:

```sh
froxcomposband -- -n<number of windows>
```

for normal ASCII graphics, or:

```sh
froxcomposband -g -- -n<# of windows>
```

for 8x8 tile graphics.

You can override user and save paths without moving the full `lib` tree:

```sh
froxcomposband -du=~/Documents/Frox -ds=~/Documents/Frox/save -- -n1
```

Some users have reported installation problems when compiling under Linux and
macOS. Running `./configure --with-no-install` will bypass this issue by
building an executable to run in place.

You can change game windows' font, location, and size with environment
variables, for example:

```sh
ANGBAND_X11_FONT='-*-*-medium-r-normal--24-*-*-*-*-*-iso8859-1' \
froxcomposband -- -n
```

You can set `ANGBAND_X11_FONT_n` for a specific window `n`. Window locations
are `ANGBAND_X11_AT_X_n` and `ANGBAND_X11_AT_Y_n`. Window sizes are
`ANGBAND_X11_COLS_n` and `ANGBAND_X11_ROWS_n`.

Thanks to Nick McConnell for implementing and improving building under
Linux/macOS.

### Linux development with AddressSanitizer

For development, doing an install is often undesirable. Try:

```sh
sudo apt-get install clang-3.5 llvm-3.5
sh autogen.sh
./configure SANITIZE_FLAGS=-fsanitize=address --with-no-install CC=clang-3.5
make clean
make -j4
ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-3.5 ./froxcomposband -g -u<Savefile> -- -n1
```

Never pass sanitizer flags through `CFLAGS`, as sanitizing `configure` causes
it to fail. Instead, `configure` appends `SANITIZE_FLAGS` to `CFLAGS` and
`LDFLAGS` once it has finished generating test programs. See `configure.ac`.

The build now places the game binary in the repository root automatically.

I like to develop with the address sanitizer enabled at all times. This works
best with `clang` rather than `gcc`. Of course, `3.5` should be replaced with
whatever current version of clang you have available.

**Fonts on Linux:** My experience with Linux has been that the fonts are often
just plain awful. Here I document how I managed to install better fonts and use
them in FroxComposband. I spent nearly a day on this, so I am hoping this
might prove useful to somebody else. But mostly, it is here to remind me how I
did so I do not stumble so much next time.

1. Install some better fonts, for example:

   ```sh
   sudo apt-get install fonts-liberation
   ```

2. See what fonts are on your system that FroxComposband can use:

   ```sh
   xlsfonts
   ```

   Notice that the new fonts are not there.

3. Find where the new fonts were installed, for example:

   ```sh
   fc-list | grep liberation
   ```

4. Bang head on wall in frustration.

5. Tell the X server about the fonts:

   ```sh
   cd /usr/share/fonts/truetype/liberation/
   su
   mkfontscale
   mkfontdir
   exit
   cat fonts.dir
   xset +fp /usr/share/fonts/truetype/liberation
   xset fp rehash
   xlsfonts | grep liberation
   ```

6. Now try to find a font you like. For example:

   ```sh
   cd [path to froxcomposband]
   ANGBAND_X11_FONT='-misc-liberation mono-medium-r-normal--20-0-0-0-m-0-iso8859-1' ./froxcomposband -mx11
   ```

   You can play with the point size since these are vector fonts. I chose 20pt
   because my eyes suck.

7. Smile. Grab a beer to recover from step 4.

### Curses

Curses is for Linux, of course, so everything said above also applies here. To
run with a single big terminal:

```sh
./froxcomposband -mgcu -uCrusher
```

To add additional terminal windows, you need to specify sub-options. You can
configure, from the command line, a strip of terminals on the right-hand side
of the screen, on the bottom of the screen, or both.

For example:

```sh
./froxcomposband -mgcu -uCrusher -- -right 57x26,*
```

This specifies that the right-hand strip will be 57 columns wide and will
contain two additional terminals. The first one, on top, will be 57x26 while
the second terminal will be 57x(LINES-26). These terminals will be numbered 1
and 2 respectively.

Another example:

```sh
./froxcomposband -mgcu -uCrusher -- -bottom -bottom *x10
```

This adds a bottom strip 10 rows high, containing a single additional terminal
window numbered as Term-1.

You can combine `-right` and `-bottom` in either order. For example:

```sh
./froxcomposband -mgcu -uCrusher -- -right 57x26,* -bottom *x10
```

Here, Term-1 and Term-2 are on the right strip, while Term-3 is on the bottom
strip. Term-0, the main terminal, is in the top left and uses the remaining
space. The map terminal will always use as much room as possible.

The order of `-right` and `-bottom` is significant, since it affects both the
child terminal numbering and the overlap region in the bottom-right corner.

For example:

```sh
./froxcomposband -mgcu -uCrusher -- -bottom *x10 -right 57x26,*
```

You cannot specify more than seven child terminals.

### Windows

Download the binary archive for Windows, unzip it anywhere you have full
permissions, and launch `froxcomposband` to play.

To compile the source code in MinGW:

```sh
./autogen.sh
./configure --enable-win
make
```

MinGW sometimes randomly chokes on one of these steps, in which case you will
need to redo that step.

## Basics

The in-game documentation covers a great deal of the game. Press `?` to
activate the help system, then select your favorite topic; for example, press
`a` twice for General Information, `a` followed by `b` for an introduction to
the help files and how best to read them, or `a` followed by `j` for the
Newbie Guide. If you get lost in the depths of the help system, press `!` to
return to the help system's main page.

Help files can also be generated as HTML files for viewing outside the game in
your favorite browser.

If you are still stuck, you can usually find friendly people at
[angband.live](https://angband.live) who will try to help you out.

## Commands

Again, please refer to the in-game help: press `?` for help and then `c` for
Commands. Choose the section that interests you. `Command Descriptions` is a
separate, lengthy file that explains all commands in detail.

Most of this file was inherited from PosChengband and later adapted through
FrogComposband into FroxComposband.
