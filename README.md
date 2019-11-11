# Command-line Tools and C library for blink(1) USB RGB LED

This code lives at the repository: https://github.com/todbot/blink1-tool
It originally lived as a directory https://github.com/todbot/blink1 but now has its own repository.

For pre-built binaries, see the Releases page: https://github.com/todbot/blink1-tool/releases

To build, see the [Makefile](./Makefile). But in general you can do:

```
git clone https://github.com/todbot/blink1-tool
cd blink1-tool
make
```

If your OS is not detected automatically, you can force it with something like:
```
OS=linux make
```


The current tools are:

- `blink1-lib` -- C library for controlling blink(1)
- `blink1-tool` -- command-line tool for controlling blink(1)
- `blink1control-tool` -- blink1-tool for use with Blink1Control (uses HTTP REST API)
- `blink1-tiny-server` -- Simple HTTP API server to control blink1, uses blink1-lib
- `blink1-mini-tool` -- commandline tool using libusb-0.1 and minimal deps
- `blink1raw` -- example commandline tool using Linux hidraw

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


