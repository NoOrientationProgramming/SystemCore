
<h2 id="processing-start" style="display:none;"></h2>

![GitHub](https://img.shields.io/github/license/NoOrientationProgramming/SystemCore?style=plastic&color=blue)
[![GitHub Release](https://img.shields.io/github/v/release/NoOrientationProgramming/SystemCore?color=blue&style=plastic)](https://github.com/NoOrientationProgramming/SystemCore/releases)
[![Standard](https://img.shields.io/badge/standard-C%2B%2B11-blue.svg?style=plastic&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)

![Windows](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/windows.yml?style=plastic&logo=github&label=Windows)
![Linux](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/linux.yml?style=plastic&logo=linux&logoColor=white&label=Linux)
![MacOS](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/macos.yml?style=plastic&logo=apple&label=MacOS)
![FreeBSD](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/freebsd.yml?style=plastic&logo=freebsd&label=FreeBSD)
![ARM, RISC-V & MinGW](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/cross.yml?style=plastic&logo=gnu&label=ARM%2C%20RISC-V%20%26%20MinGW)

[![Discord](https://img.shields.io/discord/960639692213190719?style=plastic&color=purple&logo=discord)](https://discord.gg/FBVKJTaY)
[![Twitch Status](https://img.shields.io/twitch/status/Naegolus?label=twitch.tv%2FNaegolus&logo=Twitch&logoColor=%2300ff00&style=plastic&color=purple)](https://twitch.tv/Naegolus)

## Nop/SystemCore

A lightweight C++ framework for task scheduling and multitasking applications.

### Advantages
- **Recursive structure** > Uniform system architecture from simple to large systems
- **Memory-safe design** > Process and data lifetimes are bound together, reducing leaks and other bugs
- **Integrated debugging** > Process overview, logs and command interface via TCP
- **Cross-platform** > Windows, Linux, MacOS, FreeBSD, uC (STM32, ESP32, ARM, RISC-V)

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/NopTutorials/main/doc/motivation-system-core.png" style="width: 800px; max-width:100%"/>
  </kbd>
</p>

### Add to your project

`git submodule add https://github.com/NoOrientationProgramming/SystemCore.git`

### Use Templates!

To implement a new process you can use the provided shell scripts on linux: [cppprocessing.sh](https://github.com/NoOrientationProgramming/SystemCore/blob/main/tools/cppprocessing.sh) / [cppprocessing_simple.sh](https://github.com/NoOrientationProgramming/SystemCore/blob/main/tools/cppprocessing_simple.sh)

Or just create your own..

### Learn how to use it

The [Tutorials](https://github.com/NoOrientationProgramming/NopTutorials) provide more information on how to delve into this wonderful (recursive) world ..

Another great example for using the SystemCore is [CodeOrb](https://github.com/NoOrientationProgramming/code-orb#codeorb-start)!

### Requirements

- C++ standard as low as C++11 can be used
- On Microcontrollers, recommended
  - Min. 20k RAM
  - Min. 32k Flash
