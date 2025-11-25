Making Releases of blink1-tool
==============================

[this is mostly notes to Tod and other maintainers on how to do new releases]

Steps when a new release of blink1-tool (and blink1-lib) is to be made

Assumptions
------------

- Target platforms are primarily:
  - Mac OS X 11+
  - Windows 7+
  - Linux 32-bit - Ubuntu 20
  - Raspberry Pi
  - Arduino Yun / OpenWrt ar71xx
  - OpenWrt brcm47xx

- Build primarily 64-bit, not 32-bit
- Unix command-line build tools available for each platform
- Shared mounted checkout of github.com/todbot/ blink1 repo,
   or at least same revision checkout

General Process
---------------
[most of this is superceded by CI now]

1. `cd  blink1-tool`
2. git tag release w/ `git tag -a v2.0.2 -m 'some update msg' && git push --tags`
3. Build code with `make clean && make blink1-tool blink1-tiny-server blink1control-tool`
4a. Verify no non-system shared libs in MacOS build: `otool -L blink1-tool blink1control-tool/blink1control-tool`
4b. Verify no MSYS shared libs in Windows build: `ldd blink1-tool`
5. Codesign: `make codesign`  (be sure to set needed environment vars)
6. Package up zipfiles with `make package-all` and `make cpbuilds` 
7. Copy zip packages to and test on separate test systems
8. Publish zip packages to github release


Example
-------
```
% cd blink1-tool
% make clean
% make blink1-tool
% make blink1-tiny-server blink1control-tool
% make codesign
% make codesign-check
% make package-all
% make cpbuilds
```


Platform Specifics (for Tod mostly)
-----------------------------------

### OpenWrt - Yun / ar71xx

On Fedora 14 VM using OpenWrt-SDK-ar71xx-for-linux-x86_64-gcc-4.6-linaro_uClibc-0.9.33.2

Command: `make distclean && make OS=yun && make package`

### OpenWrt - brcm71xx
On Fedora 14 VM using OpenWrt-SDK-brcm47xx-for-linux-i486-gcc-4.6-linaro_uClibc-0.9.33.2

Command: `make distclean && make OS=wrt && make package`



