<img align=right alt="Language grade: C/C++" src="https://img.shields.io/lgtm/grade/cpp/g/deltabeard/haiyajan-menu.svg?logo=lgtm&logoWidth=18"/>

# Haiyajan UI

UI toolkit for Haiyajan. Requires the SDL2 library only. Written in C99.

Simple to implement, simple to use.

![Image](test/img/main_menu_continue.png)

## Building

### Dependencies

- A C99 compiler
- GNU Make
- SDL2
- SDL2_ttf
  - SDL2_ttf depends upon FreeType.
- GNU Fribidi
- pkg-config

Pre-built dependencies for Windows NT are available at `./etc/MSVC/MSVC_DEPS.exe` and is used by the Makefile automatically.

### Unix-like

1. Make sure that the dependecies listed above are installed.
   - On Debian for example, the following command can be executed as root to install the required dependencies: `apt-get install gcc make libsdl2-dev libsdl2-ttf-dev libfribidi-dev pkg-config`.

2. Execute GNU Make in the project directory.


### Windows NT

Both methods require Visual Studio 2019. *Visual Studio 2019 Community* is free to download. *Visual Studio Code* does not provide a Windows NT compiler. For compiling within an *MSYS* or *MinGW* environment, use the [Unix](#Unix) instructions.

1. Open `./ext/MSVC/haiyajan-menu.vcxproj` within Visual Studio 2019 and build solution (Ctrl+Shift+B).

Or

1. Open *Native Tools Command Prompt for VS*.

2. Execute GNU Make in the project directory.

## License

Copyright (c) 2020 Mahyar Koshkouei<br/>
Licensed under GNU LGPL Version 3.
