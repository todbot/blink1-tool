# Command-line Tools and C library for blink(1) USB RGB LED

This is a collection of C/C++ tools for controlling the [blink1 USB RGB LED](https://blink1.thingm.com/).

This code lives at https://github.com/todbot/blink1-tool.
It started as the `commandline` directory in https://github.com/todbot/blink1. 

For pre-built binaries, see the Releases page: https://github.com/todbot/blink1-tool/releases

To build, see the [Makefile](./Makefile) and down below
for build variations and OS-specific requirements.

The current tools are:

- `blink1-tool` -- command-line tool for controlling blink(1)
- `blink1control-tool` -- blink1-tool for use with Blink1Control (uses HTTP REST API)
- `blink1-tiny-server` -- Simple HTTP API server to control blink1, uses blink1-lib
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

### Linux
- `sudo apt-get install build-essential pkg-config libudev-dev`
- And if you want libusb variant, add: `sudo apt-get install libusb-1.0-0-dev`

### MacOS
- Xcode
- In Terminal, "xcode-select --install" to install command-line tools

### Windows
- Install Visual Studio 2015
- Install MSYS2 : https://github.com/msys2/msys2/wiki/MSYS2-installation
- pacman -S base-devel make git zip unzip
- pacman -S mingw-w64-x86_64-toolchain
   - add to PATH compiler and Windows linker:
         export PATH=${PATH}:/c/msys64/mingw64/bin
         export PATH=${PATH}:"/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin"
   - git clone https://github.com/todbot/blink1-tool


For other OSes, see the Makefile for details


## Using blink1-lib in your C/C++ project
[tbd, but basically look at the makefile for blink1-tool]


## Docker and blink(1)
[this may be out of date after the repo move]

To build the image from the `Dockerfile`

- `docker build -t robtec/blink1 .`

Running the container

- `docker run -d --privileged robtec/blink1`

Note the `--privileged` tag, docker needs this to access the hosts USB controllers

Docker resources
- [Install Guide](https://docs.docker.com/installation/)
- [Run Command](https://docs.docker.com/reference/run/)


