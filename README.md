
<h2 id="processing-start" style="display:none;"></h2>

[![GitHub](https://img.shields.io/github/license/NoOrientationProgramming/SystemCore?style=plastic&color=blue)](https://en.wikipedia.org/wiki/MIT_License)
[![GitHub Release](https://img.shields.io/github/v/release/NoOrientationProgramming/SystemCore?color=blue&style=plastic)](https://github.com/NoOrientationProgramming/SystemCore/releases)
[![Standard](https://img.shields.io/badge/standard-C%2B%2B11-blue.svg?style=plastic&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)

![Windows](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/SystemCore/windows.yml?style=plastic&logo=github&label=Windows)
![Linux](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/SystemCore/linux.yml?style=plastic&logo=linux&logoColor=white&label=Linux)
![macOS](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/SystemCore/macos.yml?style=plastic&logo=apple&label=macOS)
![FreeBSD](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/SystemCore/freebsd.yml?style=plastic&logo=freebsd&label=FreeBSD)
![ARM, RISC-V & MinGW](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/SystemCore/cross.yml?style=plastic&logo=gnu&label=ARM%2C%20RISC-V%20%26%20MinGW)

[![Discord](https://img.shields.io/discord/960639692213190719?style=plastic&color=purple&logo=discord)](https://discord.gg/FBVKJTaY)
[![Twitch Status](https://img.shields.io/twitch/status/Naegolus?label=twitch.tv%2FNaegolus&logo=Twitch&logoColor=%2300ff00&style=plastic&color=purple)](https://twitch.tv/Naegolus)

## Nop/SystemCore

A lightweight C++ framework for task scheduling and recursive multitasking across all system levels.

Each task can run in its own thread for parallel execution.

### Advantages
- **Recursive structure** > Every system level uses the same building block
- **Memory-safe design** > Process and data lifetimes are tied together, reducing leaks and other bugs
- **Integrated debugging** > Process overview, logs, and a remote command interface via TCP (desktop) or UART (uC)
- **Cross-platform** > Windows, Linux, macOS, FreeBSD, uC (STM32, ESP32, ARM, RISC-V)

<p align="center">
  <kbd>
    <img src="https://github.com/NoOrientationProgramming/NopExamples/blob/main/doc/SystemCore/motivation-system-core.png" style="width: 800px; max-width:100%"/>
  </kbd>
</p>

### Learn how to use it

- The [Hello World](https://github.com/NoOrientationProgramming/SystemCore/tree/main/tools/hello-world) can be found in this repository. Check it out! It has a lot of comments explaining the basics.
- The [Examples](https://github.com/NoOrientationProgramming/NopExamples) provide more information on how to delve into this wonderful (recursive) world ..
- Another great example for using the SystemCore is [CodeOrb](https://github.com/NoOrientationProgramming/code-orb#codeorb-start)!

### Use Templates!

To implement a new process you can use the provided shell scripts on linux: [cppprocessing.sh](https://github.com/NoOrientationProgramming/SystemCore/blob/main/tools/cppprocessing.sh) / [cppprocessing_simple.sh](https://github.com/NoOrientationProgramming/SystemCore/blob/main/tools/cppprocessing_simple.sh)

Or just create your own..

### Add to your project

Just download the sources and compile.

### Requirements

- C++ standard as low as C++11 can be used
- On Microcontrollers, recommended
  - Min. 20k RAM
  - Min. 32k Flash
