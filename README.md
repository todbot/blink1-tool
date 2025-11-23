# Command-line Tools and C library for blink(1) USB RGB LED

[![Build Status linux](https://github.com/todbot/blink1-tool/workflows/linux/badge.svg)](https://github.com/todbot/blink1-tool/actions?query=workflow%3Alinux)
[![Build Status macos](https://github.com/todbot/blink1-tool/workflows/macos/badge.svg)](https://github.com/todbot/blink1-tool/actions?query=workflow%3Amacos)
[![Build Status windows](https://github.com/todbot/blink1-tool/workflows/windows/badge.svg)](https://github.com/todbot/blink1-tool/actions?query=workflow%3Awindows)


This is an official collection of C/C++ commandline tools for controlling
the [blink1 USB RGB LED](https://blink1.thingm.com/).

This code lives at https://github.com/todbot/blink1-tool.

For pre-built binaries, see the [Releases page](https://github.com/todbot/blink1-tool/releases).

The current tools are:

- `blink1-tool` -- command-line tool for controlling blink(1)
- `blink1control-tool` -- blink1-tool for use with Blink1Control (uses HTTP REST API)
- `blink1-tiny-server` -- ([README](server/README.md)) Simple HTTP API server to control blink1, uses blink1-lib
- `blink1-lib` -- C library for controlling blink(1)
- `blink1-mini-tool` -- commandline tool using libusb-0.1 and minimal deps
- `blink1raw` -- small example commandline tool using Linux hidraw

Type `make help` for a full list.

Also see in this directory:
- `scripts` -- examples shell scripts using blink1-tool

## Supported platforms

Supported platforms for `blink1-tool` and `blink1-lib`:

- Mac OS X 10.6.8, 10.7+
- Windows XP+ (built with MinGW & MSYS)
- Linux (most all, primary development on Ubuntu)
- FreeBSD 8.2+
- Raspberry Pi (Raspian)
- BeagleBone (Ubuntu)
- OpenWRT / DD-WRT
- ... just about anything else with Gnu Make & a C-compiler

In general, the `blink1-tool` builds as a static binary where possible,
eliminating the need for shared library dependencies on the target.
However, static builds can be problematic for some systems with different
libusb implementations, so doing `make EXEFLAGS=` will generally build a non-static version.

## Commmand-line options for blink1-tool

```
% blink1-tool --help
Usage: 
  blink1-tool <cmd> [options]
where <cmd> is one of:
  --list                      List connected blink(1) devices 
  --rgb=<red>,<green>,<blue>  Fade to RGB value
  --rgb=[#]RRGGBB             Fade to RGB value, as hex color code
  --hsb=<hue>,<sat>,<bri>     Fade to HSB value
  --blink <numtimes>          Blink on/off (use --rgb to blink a color)
  --flash <numtimes>          Flash on/off (same as blink)
  --on | --white              Turn blink(1) full-on white 
  --off                       Turn blink(1) off 
  --red                       Turn blink(1) red 
  --green                     Turn blink(1) green 
  --blue                      Turn blink(1) blue 
  --cyan                      Turn blink(1) cyan (green + blue) 
  --magenta                   Turn blink(1) magenta (red + blue) 
  --yellow                    Turn blink(1) yellow (red + green) 
  --rgbread                   Read last RGB color sent (post gamma-correction)
  --setpattline <pos>         Write pattern RGB val at pos (--rgb/hsb to set)
  --getpattline <pos>         Read pattern RGB value at pos
  --savepattern               Save RAM color pattern to flash (mk2)
  --clearpattern              Erase color pattern completely 
  --play <1/0,pos>            Start playing color pattern (at pos)
  --play <1/0,start,end,cnt>  Playing color pattern sub-loop (mk2)
  --playstate                 Return current status of pattern playing (mk2)
  --playpattern <patternstr>  Play Blink1Control pattern string in blink1-tool
  --writepattern <patternstr> Write Blink1Control pattern string to blink(1)
  --readpattern               Download full blink(1) patt as Blink1Control str
  --servertickle <1/0>[,1/0,start,end] Turn on/off servertickle (w/on/off, uses -t msec)
  --chase, --chase=<num,start,stop> Multi-LED chase effect. <num>=0 runs forever
  --random, --random=<num>    Flash a number of random colors, num=1 if omitted 
  --glimmer, --glimmer=<num>  Glimmer a color with --rgb (num times)
 Nerd functions: 
  --fwversion                 Display blink(1) firmware version 
  --version                   Display blink1-tool version info 
  --setstartup                Set startup parameters (v206+,mk3) 
  --getstartup                Get startup parameters (v206+,mk3) 
  --gobootload                Enable bootloader (mk3 only)
  --lockbootload              Lock bootloader (mk3 only)
  --getid                     Get unique id (mk3 only)
and [options] are: 
  -d dNums --id all|deviceIds Use these blink(1) ids (from --list) 
  -g -nogamma                 Disable autogamma correction
  -b b --brightness b         Set brightness (0=use real vals, 1-255 scaled)
  -m ms,   --millis=millis    Set millisecs for color fading (default 300)
  -q, --quiet                 Mutes all stdout output (supercedes --verbose)
  -t ms,   --delay=millis     Set millisecs between events (default 500)
  -l <led>, --led=<led>       Which LED to use, 0=all/1=top/2=bottom (mk2+)
  --ledn 1,3,5,7              Specify a list of LEDs to light
  -v, --verbose               verbose debugging msgs

Examples: 
  blink1-tool -m 100 --rgb=255,0,255    # Fade to #FF00FF in 0.1 seconds 
  blink1-tool -t 2000 --random=100      # Every 2 seconds new random color
  blink1-tool --led 2 --random=100      # Random colors on both LEDs 
  blink1-tool --rgb 0xff,0x00,0x00 --blink 3  # blink red 3 times
  blink1-tool --rgb '#FF9900'           # Make blink1 pumpkin orange
  blink1-tool --rgb FF9900 --led 2      # Make blink1 orange on lower LED
  blink1-tool --chase=5,3,18            # Chase pattern 5 times, on leds 3-18

Pattern Examples: 
  # Play purple-green flash 10 times (pattern runs in blink1-tool so blocks)
  blink1-tool --playpattern '10,#ff00ff,0.1,0,#00ff00,0.1,0'
  # Change the 2nd color pattern line to #112233 with a 0.5 sec fade
  blink1-tool -m 500 --rgb 112233 --setpattline 1 
  # Erase all lines of the color pattern and save to flash 
  blink1-tool --clearpattern ; blink1-tool --savepattern 

Servertickle Examples: 
  # Enable servertickle to play pattern after 2 seconds 
  # (Keep issuing this command within 2 seconds to prevent it firing)
  blink1-tool -t 2000 --servertickle 1 
  # Enable servertickle after 2 seconds, play sub-pattern 2-3 
  blink1-tool -t 2000 --servertickle 1,1,2,3 

Setting Startup Params Examples (mk2 v206+ & mk3 only):
  blink1-tool --setstartup 1,5,7,10  # enable, play 5-7 loop 10 times
  blink1-tool --savepattern          # must do this to save to flash 

User notes (mk3 only):
  blink1-tool --writenote 1 --notestr 'hello there' 
  blink1-tool --readnote 1 
  blink1-tool --readnotes    # reads all notes out 


Notes: 
 - To blink a color with specific timing, specify 'blink' command last:
   blink1-tool -t 200 -m 100 --rgb ff00ff --blink 5 
 - If using several blink(1)s, use '-d all' or '-d 0,2' to select 1st,3rd: 
   blink1-tool -d all -t 50 -m 50 -rgb 00ff00 --blink 10 
```

## OS-specific Notes

### Linux (including Raspberry Pi)

To use blink(1) as a non-root user, udev rules must be installed.
On Debian-like systems (Ubuntu, Raspian), these udev rules are installed with:

```
sudo cp 51-blink1.rules /etc/udev/rules.d/51-blink1.rules
sudo udevadm control --reload
sudo udevadm trigger
```


### Building from source

In general you can do:

```
git clone https://github.com/todbot/blink1-tool
cd blink1-tool
make
```

On Linux you will need to install [some requirements](#linux-debian-flavored-including-raspberry-pi) beforehand.

If your OS is not detected automatically, you can force it with something like:
```
OS=linux make
```

To see the supported platforms, please consult the [Makefile](./Makefile)


### Build variants

There are two primary USB libraries that `blink1-tool` can be built for:
- `USBLIB_TYPE=HIDAPI` -- Uses the feature-rich cross-platform `hidapi` library (default)
- `USBLIB_TYPE=HIDDATA` -- Uses a simple, cross-platform `hiddata` library (included)

For Linux, there are to HIDAPI_TYPEs you can choose from:
- `HIDAPI_TYPE=HIDRAW` -- Uses standard `hidraw` kernel API for HID devices  (default)
- `HIDAPI_TYPE=LIBUSB` -- Uses lower-level `libusb` commands (good for older Linuxes)

To compile for a particular `USBLIB_TYPE` or `HIDAPI_TYPE`, specify them when buildling:

```
HIDAPI_TYPE=LIBUSB make
```

## OS-specific prerequisites for compiling

If you have the ability to compile programs on your system,
you may already have everything you need to compile `blink1-tool`.

### Linux (Debian-flavored, including Raspberry Pi)

- In a terminal, install pre-reqs and build:

  ```shell
  sudo apt install build-essential git pkg-config libudev-dev
  sudo apt install libusb-1.0-0-dev  # (optional, only for libusb variant)
  cd blink1-tool
  make blink1-tool
  make blink1-tiny-server
  HIDAPI_TYPE=libusb make blink1-tool # (if you instead you want libusb version)
  ```

### FreeBSD

- Install pre-reqs and build:

  ```shell
  sudo pkg install gmake gcc git libiconv
  cd blink1-tool
  gmake`
  sudo ./blink1-tool --red --flash 3
  ```
  
If you'd like `devd` support, see the `devd-51-blink1.conf` file. 

### MacOS

- Xcode
- In Terminal, setup Xcode and build:

  ```shell
  xcode-select --install
  cd blink1-tool
  make blink1-tool
  ```

### Windows

- Install Visual Studio 2017 or 2019
- Install MSYS2: https://www.msys2.org/
- In MSYS2 bash shell:
  ```
  pacman -S base-devel make git zip unzip mingw-w64-x86_64-gcc
  export PATH=${PATH}:/c/msys64/mingw64/bin
  export PATH=${PATH}:"/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin"
  export PATH=${PATH}:'/c/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64'"
  make
  ```

### Other OSes

See the blink1-tool/Makefile for details


## Using blink1-lib in your C/C++ project

See Makefile for your platform


## Docker and blink(1)

To build a image from `Dockerfile-ubuntu`:

- `docker build -f Dockerfile-ubuntu -t todbot/blink1 .`

Running the container:

- `docker run --rm -it --privileged todbot/blink1 blink1-tool --on`
- `docker run --rm -it --device /dev/bus/usb:/dev/bus/usb todbot/blink1 blink1-tool --on`

Note the `--privileged` tag, docker needs this to access the hosts USB controllers

Docker resources
- [Install Guide](https://docs.docker.com/installation/)
- [Run Command](https://docs.docker.com/engine/reference/run/)
- [Ubuntu Dockerfile](https://github.com/todbot/blink1-tool/blob/master/Dockerfile-ubuntu)
