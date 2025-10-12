# Whisper64 🐕

A simple text editor for the Commodore 64.

![Whisper64 Screenshot](screenshot.png)

## Features

- BASIC keyword syntax highlighting
- Line numbers with cursor position display (line:column)
- 256-line capacity with automatic paging (64 lines per page)
- Directory browser with multi-drive support (drives 8-15)
- Search and replace functionality
- Copy/paste with mark mode
- 37×23 editing area

## Installation

### Prerequisites

Install the [LLVM-MOS SDK](https://github.com/llvm-mos/llvm-mos-sdk#getting-started)

### Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The compiled program will be at `build/whisper64.prg`

### Running

Load on your C64 or emulator:
```
LOAD "WHISPER64.PRG",8,1
RUN
```

## Key Commands

| Key | Function |
|-----|----------|
| **F1** | Load file (directory browser) |
| **F2** | Save file |
| **F3** | Select drive (8-15) |
| **F5** | Find text |
| **F6** | Find & replace |
| **F7** | Find next |
| **F8** | Help screen |
| **CBM+M** | Toggle mark mode |
| **CBM+C** | Copy marked text |
| **CBM+V** | Paste text |
| **HOME** | Go to top |
| **Arrows** | Move cursor |

## License

Free to use and modify.