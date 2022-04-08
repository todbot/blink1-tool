## Building blink1-tool binaries on Windows

blink1-tool and its friends are built in POSIX way,
but using native Windows compilers to produce stand-alone binaries
dependant only on standard Windows shared libraries.

I've found acheiving this to be frustrating over the years.
These are some notes for how to do this on the desktop

For a repeatable environment, see the "windows" github action.


### Windows (w/ Chocalatey)

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

### Windows (w /Scoop)

- Install Scoop - https://scoop.sh/ 
- Install Git GUI
- `scoop install gcc make`
- Open `git-bash.exe` (in Start Menu > Git > Git Bash)
- 

### Windows (another way)

- Turn off "real-time virus protection" to speed up compilation
- Install MSYS2 from installer
- open "MSYS2 MSYS" app from Start Menu
- In shell:
-  pacman -S  mingw-w64-x86_64-toolchain
-  export PATH=${PATH}:/c/msys64/mingw64/bin
- OR...
- Open CMD shell
- `set PATH=%PATH%;c:\msys64\usr\bin`
- call "c:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
- run "bash"   # to get bash shell
- cd blink1-tool
- make
