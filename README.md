# Whisper64 🐕
A feature-rich text editor for the Commodore 64.

![Whisper64 Screenshot](screenshot.png)

## Features

- **BASIC Mode** with keyword syntax highlighting and automatic line renumbering
- **Smart Paging**: 256-line capacity with automatic 64-line pages
- **Directory Browser**: Multi-drive support (8-15) with file type display
- **Search & Replace**: Find text with wrap-around and replace all
- **Copy/Paste**: Visual mark mode for selecting and copying text
- **Undo/Redo**: One-level undo and redo for text editing operations
- **Goto Line**: Jump directly to any line number in your file
- **Mouse Support**: Experimental support for Commodore 1351 mouse (click to position cursor)
- **Status Bar**: Shows filename, cursor position, drive, page, and mode indicators
- **37×23 editing area** with line numbers

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
| **F1** | Load file (opens directory browser) |
| **F2** | Save file (with overwrite protection) |
| **F3** | Select drive (8-15) |
| **F4** | Toggle BASIC mode / Renumber BASIC lines |
| **F5** | Find text |
| **F6** | Find & replace (with replace all option) |
| **F7** | Find next occurrence |
| **F8** | Help screen |
| **CTRL+C** | Copy marked text (up to 8 lines) |
| **CTRL+G** | Jump directly to any line number |
| **CTRL+J** | Toggle mouse on/off (experimental) |
| **CTRL+K** | Toggle mark mode for selection |
| **CTRL+V** | Paste copied text |
| **CTRL+Y** | Redo the last undone change |
| **CTRL+Z** | Undo the last change |
| **HOME** | Go to top of file |
| **Arrows** | Move cursor (updates mark selection when active) |

## Keyboard Shortcuts Reference

### File Operations
- **F1** → Load file (directory browser)
- **F2** → Save file

### Editing
- **CTRL+C** → Copy marked text
- **CTRL+V** → Paste text
- **CTRL+Z** → Undo
- **CTRL+Y** → Redo
- **CTRL+K** → Toggle mark mode
- **CTRL+G** → Goto line

### Search
- **F5** → Find text
- **F6** → Find & replace
- **F7** → Find next

### Navigation
- **HOME** → Jump to top of file
- **Arrow Keys** → Move cursor

### Other
- **CTRL+J** → Toggle mouse support (experimental)
- **F3** → Select drive (8-15)
- **F4** → BASIC mode / Renumber
- **F8** → Help screen

## Mouse Support (Experimental)

Whisper64 includes experimental support for the **Commodore 1351 mouse** on **Control Port 1** (left port).

**To use the mouse:**
1. Connect your 1351 mouse to Control Port 1
2. Press **CTRL+J** to enable mouse mode
3. Move the mouse to position the cursor (shown as yellow `^`)
4. Click the left button to position the text cursor
5. Press **CTRL+J** again to disable mouse mode

**Notes:**
- Mouse cursor is only shown when mouse mode is active
- Clicking in the edit area (lines 1-23) positions the text cursor
- Mouse sensitivity can be adjusted in `mouse.c` (`MOUSE_DIVISOR` constant)

## BASIC Mode

Press **F4** to enable BASIC mode, which provides:
- Purple keyword highlighting for BASIC commands
- Press **F4** again to renumber all lines (10, 20, 30...)
- Automatic update of GOTO, GOSUB, THEN, and ELSE references
- Line number display in cyan

## Directory Browser

Press **F1** to open the directory browser:
- Shows disk name at top
- Displays file type (PRG, SEQ, DEL, USR, REL)
- Shows locked files with asterisk (*)
- Displays block size for each file
- Use **↑/↓** arrows to navigate
- Press **RETURN** to load selected file
- Press **RUN/STOP** to cancel

## Status Bar

The top status bar displays:
- **Filename**: Currently loaded/saved file (8 chars)
- **Line:Column**: Current cursor position
- **[BAS]**: BASIC mode indicator (purple)
- **[M]**: Mark mode active indicator (green)
- **D:n**: Current drive number (cyan)
- **Pn/n**: Current page / total pages (cyan, if multi-page)

## Copy/Paste Operations

To copy and paste text:
1. Press **CTRL+K** to enter mark mode
2. Use arrow keys to select text (selection shown in yellow)
3. Press **CTRL+C** to copy the selected text
4. Move cursor to destination
5. Press **CTRL+V** to paste
6. Press **CTRL+K** again to exit mark mode

You can copy up to 8 lines at once. The copied text remains in the clipboard until you copy something new.

## Search & Replace

**Simple Search:**
1. Press **F5** to open search
2. Type the search term and press RETURN
3. Use **F7** to find next occurrence
4. Search wraps around to beginning when reaching end

**Find & Replace:**
1. First search with **F5**
2. Press **F6** to open replace dialog
3. Type replacement text
4. Choose:
   - **Y** to replace all occurrences at once
   - **N** to manually replace each occurrence

## Goto Line
1. Press **CTRL+G**
2. Type the line number (1-64)
3. Press RETURN

## License
Free to use and modify.