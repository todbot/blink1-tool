# Command-line Tools and C library for blink(1) USB RGB LED

[![Build Status linux](https://github.com/todbot/blink1-tool/workflows/linux/badge.svg)](https://github.com/todbot/blink1-tool/actions?query=workflow%3Alinux)
[![Build Status macos](https://github.com/todbot/blink1-tool/workflows/macos/badge.svg)](https://github.com/todbot/blink1-tool/actions?query=workflow%3Amacos)
[![Build Status windows](https://github.com/todbot/blink1-tool/workflows/windows/badge.svg)](https://github.com/todbot/blink1-tool/actions?query=workflow%3Awindows)


This is an official collection of C/C++ commandline tools for controlling
the [blink1 USB RGB LED](https://blink1.thingm.com/).

This code lives at https://github.com/todbot/blink1-tool.
It started as the `commandline` directory in https://github.com/todbot/blink1.

For pre-built binaries, see the Releases page: https://github.com/todbot/blink1-tool/releases

To build, see the [Makefile](./Makefile) and down below
for build variations and OS-specific requirements.

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

If your OS is not detected automatically, you can force it with something like:
```
OS=linux make
```

To see the supported platforms, please consule the Makefile.

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

### Linux (Debian-flavored)
- In a terminal, install pre-reqs and build:
- `sudo apt-get install build-essential pkg-config libudev-dev`
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
- Install [Chocolatey package manager](https://chocolatey.org/)
- In an admin CMD shell, install VisualStudio, MinGW, & MSYS2:
- `choco install visualstudio2017community visualstudio2017-workload-vctools mingw msys2 cmake`
- In a normal CMD shell, set paths and build:
- `set PATH=C:\tools\msys64\usr\bin;%PATH%`
- `pacman -S zip unzip`
- `call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars64.bat"`
- `set MAKE=mingw32-make`
- `cd blink1-tool`
- `mingw32-make`

### Windows (old way)
- Install Visual Studio 2015
- Install MSYS2 : https://github.com/msys2/msys2/wiki/MSYS2-installation
- In MSYS2 bash shell:
- `pacman -S base-devel make git zip unzip mingw-w64-x86_64-toolchain`
- `export PATH=${PATH}:/c/msys64/mingw64/bin`
- `export PATH=${PATH}:"/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin"`
- `make`


For other OSes, see the Makefile for details


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
