# cso2-launcher-modularizated

**This is a modularizatied version of Ochii/cso2-launcher.**

# Plugins

**Plugins are mainly patches to and extra functions like custom gamefiles**

## MainPatch

> Main patches for cso2 to get game running.

## Login

> Get game to work with Ochii's [cso2 services](https://github.com/Ochii/cso2-master-services)

## FsPlugin

> Allow user to modify game files


# Bug reporting and improvements
Have a look at the [issues](https://github.com/GEEKiDoS/cso2-launcher/issues) for a list of bugs found or to report them yourself.

If you have any improvements that you would like to share, go ahead and [open a pull request](https://github.com/GEEKiDoS/cso2-launcher/pulls).

# How to build

### Requirements
- [CMake](https://cmake.org/download/)
- [Visual Studio 2017](https://www.visualstudio.com/downloads/)
- [Windows PowerShell](https://docs.microsoft.com/en-us/powershell/scripting/setup/installing-windows-powershell)

Only **Windows** is supported.

### Downloading the source code
Clone the project with ```git clone --recursive https://github.com/GEEKiDoS/cso2-launcher.git```.

### Before building
Start a PowerShell instance and change directory to the project's directory, then, run ```./setuplibs.ps1```.

The script will setup PolyHook, its dependency Capstone.

Then download libcurl and build it with static lib. Then change the dir of libcurl in Login plugin project.

### Building
Open the solution ```CSO2Launcher.sln``` and build it in your preferred configuration.

If built successfully, you will find your binaries inside ```out/bin/Win32/[your configuration]```.

## Used libraries
- **[PolyHook](https://github.com/stevemk14ebr/PolyHook)** by [stevemk14ebr](https://github.com/stevemk14ebr)
- **[Capstone](https://github.com/aquynh/capstone)** by [aquynh](https://github.com/aquynh/capstone)
- **[libcurl](https://curl.haxx.se/libcurl/)**

## Credits
- [Valve Software](https://github.com/ValveSoftware/source-sdk-2013) for their SDK
- [Ochii](https://github.com/Ochii) for Original version
- [NTAuthority](https://twitter.com/ntauthority) Awesome IW4M(4D1) project for reference code.

## Thank you's (by no particular order)
- All "Thank you's" on readme of original repo

## License
Read ```LICENSE``` for license information.

I'm(and Ochii's) not affiliated with either Valve or Nexon, just like I don't own Counter-Strike Online 2.
