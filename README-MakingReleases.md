Making Releases of blink1-tool
==============================

[this is mostly notes to Tod and other maintainers on how to do new releases]

Steps when a new release of blink1-tool (and blink1-lib) is to be made

Assumptions
------------

- Target platforms are primarily:
  - Mac OS X 10.8+
  - Windows XP+
  - Linux 32-bit - Ubuntu 14
  - Raspberry Pi
  - Arduino Yun / OpenWrt ar71xx
  - OpenWrt brcm47xx


- Build primarily 64-bit, not 32-bit
- Unix command-line build tools available for each platform
- Shared mounted checkout of github.com/todbot/ blink1 repo,
   or at least same revision checkout

General Process
---------------

1. `cd  blink1-tool`
2. git tag release w/ `git tag -a v2.0.2 -m 'some update msg' && git push --tags`
3. Build code with `make clean && make`
3a. Don't forget about `make blink1control-tool` & `make blink1-tiny-server`
3b. Verify no extra shared libs in MacOS build: `otool -L blink1-tool blink1control-tool/blink1control-tool`
4. Package up zipfiles with `make package-all`
5. Copy zip packages to and test on separate test systems
6. Publish zip packages to github release


Example
-------
```
% cd blink1-tool
% make clean
% make
% make blink1control-tool && make blink1-tiny-server
% make package-all
```

Platform Specifics (for Tod mostly)
-----------------------------------

### Mac

Environment: Build on 11.5 on MacBookPro Retina x64, test on 10.13.6 Mac Mini x64, 11.5 MacBookPro M1

Command: `make distclean && make package-all`


### Windows

Build on Win7-64bit, test on Win7-32 bit, Win8-64bit (all in VMs)

Command: `make distclean && make && make package-all`

### Linux - Ubuntu

Build on Ubuntu 20 VM, test on Ubuntu 18 VM

Command: `make distclean && make && make package-all`

### Linux - Raspberry Pi

Environemt: Build on Raspberry Pi 4 running Raspbian latest.

Command: `make distclean && make && make package-all`

Rename zip packages from "armv6l" to "raspi".

### OpenWrt - Yun / ar71xx

On Fedora 14 VM using OpenWrt-SDK-ar71xx-for-linux-x86_64-gcc-4.6-linaro_uClibc-0.9.33.2

Command: `make distclean && make OS=yun && make package`

### OpenWrt - brcm71xx
On Fedora 14 VM using OpenWrt-SDK-brcm47xx-for-linux-i486-gcc-4.6-linaro_uClibc-0.9.33.2

Command: `make distclean && make OS=wrt && make package`



