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
- `sudo apt-get install libudev-dev pkg-config build-essential`
- `sudo apt-get install libusb-1.0-0-dev`  (optional, only for libusb variant)
- `cd blink1-tool`
- `make`
- `HIDAPI_TYPE=libusb make` (if you instead you want libusb version)

### FreeBSD

- Install pre-reqs and build:
- `sudo pkg install gmake gcc git libiconv`
- `cd blink1-tool`
- `gmake`
- `sudo ./blink1-tool --red --flash 3`

### MacOS

- Xcode
- In Terminal, setup Xcode and build:
- `xcode-select --install`
- `cd blink1-tool`
- `make`

### Windows

- Install Visual Studio 2015 or 2019
- Install MSYS2: https://www.msys2.org/
- In MSYS2 bash shell:
- `pacman -S base-devel make git zip unzip mingw-w64-x86_64-gcc` 
- `export PATH=${PATH}:/c/msys64/mingw64/bin`
- `export PATH=${PATH}:"/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin"`
- `export PATH=${PATH}:'/c/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64'"`
- `make`

### Other OSes

See the blink1-tool/Makefile for details


## Using blink1-lib in your C/C++ project

See Makefile for your platform


## Docker and blink(1)
[this may be out of date after the repo move]

To build the image from the `Dockerfile`

- `docker build -t robtec/blink1 .`

Running the container

- `docker run -d --privileged robtec/blink1`

Note the `--privileged` tag, docker needs this to access the hosts USB controllers

Docker resources
- [Install Guide](https://docs.docker.com/installation/)
- [Run Command](https://docs.docker.com/engine/reference/run/)
- [Ubuntu Dockerfile](https://github.com/todbot/blink1-tool/blob/master/Dockerfile-ubuntu)
