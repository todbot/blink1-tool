Making Releases of blink1-tool
==============================

[this is mostly notes to Tod and other maintainers on how to do new releases]

Steps when a new release of blink1-tool (and blink1-lib) is to be made

Assumptions
------------

- Target platforms are primarily:
  - Mac OS X 11+
  - Windows 7+
  - Linux 64-bit - Ubuntu
  - Raspberry Pi
  - Arduino Yun / OpenWrt ar71xx
  - OpenWrt brcm47xx

- Build primarily 64-bit, not 32-bit
- Version comes from the latest git tag (or a `.git-tag` file if present)


Release Process (CI)
--------------------

1. Make sure `main` builds clean and `make test` passes

2. Tag the release and push the tag:
   ```
   git tag -a v2.5.1 -m 'some update msg'
   git push --tags
   ```

3. On GitHub, create a release from that tag and **publish** it
   (Releases -> Draft a new release -> pick tag -> Publish).
   Publishing triggers the `linux`, `macos`, and `windows` workflows, which:
   - build blink1-tool, blink1-tiny-server, blink1control-tool
   - macos: sign + notarize (Developer ID cert, see README-codesigning.md)
   - windows: sign with Azure Trusted Signing
   - package zips and attach them to the release

4. Raspberry Pi is manual: Actions -> raspberrypi -> Run workflow,
   pick the release tag in "Use workflow from", and tick
   "Upload installers to the latest GitHub release".
   This attaches to the *latest* release, so run it after step 3.

5. Check the release page has zips for each platform, then download
   and spot-test them on real systems

Notes:
- To test signing/notarizing without making a release, run the `macos` or
  `windows` workflow by hand with the "notarize" / "sign" box ticked.
  Zips end up as workflow artifacts, not on a release.
- Uploads use `--clobber`, so re-running a workflow replaces its zips.
- Signing needs these repo secrets:
  - macos: `MACOS_CERTIFICATE`, `MACOS_CERTIFICATE_PWD`, `APPLE_ID`,
    `APPLE_APP_SPECIFIC_PASSWORD`, `APPLE_TEAM_ID`
  - windows: `AZURE_TENANT_ID`, `AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`


Manual Process (no CI)
----------------------

1. `cd blink1-tool`
2. Tag the release (as above)
3. Build: `make clean && make blink1-tool blink1-tiny-server blink1control-tool`
4. Check for non-system shared libs:
   - MacOS: `otool -L blink1-tool blink1control-tool/blink1control-tool`
   - Windows: `ldd blink1-tool` (no MSYS libs)
5. Codesign: `make codesign` (be sure to set needed environment vars)
6. Package: `make package-all` and `make cpbuilds`
7. Copy zips to separate test systems and test
8. Upload zips to the GitHub release

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
