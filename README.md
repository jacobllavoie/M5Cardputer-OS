# M5Cardputer-OS

[![GitHub license](https://img.shields.io/github/license/jacobllavoie/M5Cardputer-OS.svg)](https://github.com/jacobllavoie/M5Cardputer-OS/blob/main/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/jacobllavoie/M5Cardputer-OS.svg?style=social)](https://github.com/jacobllavoie/M5Cardputer-OS)

This repository contains a custom, lightweight operating system for the M5Stack Cardputer. It's built on the Arduino framework and is designed to be a flexible platform for running standalone applications from an SD card.

## Table of Contents
- [Features](#features)
- [Built-in Apps](#built-in-apps)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [License](#license)

## Features
- **App Launcher**: The core of the OS is an application launcher that loads and runs binary files (.bin) from the `/apps` directory on an SD card.
- **Over-the-Air (OTA) Updates**: Update the launcher firmware over Wi-Fi without needing a USB connection.
- **System Settings**: A settings menu to control features like display text size and perform a factory reset.

## Built-in Apps
The OS comes with pre-built applications to get you started:
- **Radio App**: Stream internet radio stations.
- **Ohm's Law Calculator**: A handy tool for electronics calculations.
- **Wi-Fi Manager**: Scan for networks, save credentials, and automatically connect on boot.
- **Web Server File Manager**: When connected to Wi-Fi, you can manage the files on the SD card through a web browser on your computer.

## Getting Started

### Prerequisites
- An M5Stack Cardputer
- A microSD Card
- PlatformIO installed in your IDE (e.g., Visual Studio Code).

### Installation
1.  **Clone the Repository**:
    ```bash
    git clone <repository-url>
    ```
2.  **Prepare the SD Card**:
    - Format a microSD card (FAT32 is recommended).
    - Create a folder named `apps` in the root of the SD card.
3.  **Build and Upload**:
    - Open the project in Visual Studio Code with the PlatformIO extension.
    - The project is configured to build the launcher by default. Simply click the "Upload" button in PlatformIO to flash the main OS.
    - To build an application (e.g., the radio), select its environment (`env:radio`) in the PlatformIO project task list and run the "Build" task.
    - Copy the resulting `.bin` file (e.g., `radio.bin`) from the `.pio/build/<env_name>/` directory to the `/apps` folder on your SD card.

## Usage
- **Navigation**: Use the `;` (up) and `.` (down) keys to navigate menus. Press `Enter` to select an option.
- **Exiting Apps**: To return to the main launcher from any application, press the `fn + 
` key combination (emulating an `Esc` key).

## Project Structure
The project is organized into several key directories:
- `src/launcher/`: Contains the source code for the main OS and launcher.
- `src/apps/`: Contains the individual applications like the radio and Ohm's Law calculator.
- `lib/`: Holds shared libraries used by the launcher and applications, such as `M5CardputerOS_core`, `wifi`, `sd_card`, etc.
- `platformio.ini`: The main configuration file for PlatformIO, defining build environments and dependencies.

## Contributing
(Add contributing guidelines here if applicable)

## License
MIT License

Copyright (c) 2025 Jacob Lavoie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Acknowledgments

The Radio app was inspired by and adapted from the [M5Cardputer_WebRadio](https://github.com/cyberwisk/m5cardputer_webradio) project by cyberwisk. Thank you for the excellent foundation.