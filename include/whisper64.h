#ifndef WHISPER64_H
#define WHISPER64_H

#include <stdio.h>
#include <string.h>
#include <cbm.h>
#include <ctype.h>

// Screen constants
#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 25
#define EDIT_WIDTH 37
#define EDIT_HEIGHT 23
#define SCREEN_RAM ((unsigned char *)0x0400)

// Buffer constants
#define MAX_LINES 256
#define MAX_LINE_LENGTH 220
#define MAX_DIR_ENTRIES 100
#define LINES_PER_PAGE 64

// Key codes
#define KEY_RETURN 13
#define KEY_DELETE 20
#define KEY_F1 133
#define KEY_F2 137
#define KEY_F3 134
#define KEY_F4 138
#define KEY_F5 135
#define KEY_F6 139
#define KEY_F7 136
#define KEY_F8 140
#define KEY_HOME 19
#define KEY_UP 145
#define KEY_DOWN 17
#define KEY_LEFT 157
#define KEY_RIGHT 29

// Colors
#define COL_WHITE 1
#define COL_CYAN 3
#define COL_YELLOW 7
#define COL_RED 2
#define COL_GREEN 5
#define COL_BLUE 6
#define COL_PURPLE 4
#define COL_ORANGE 8

// Macros
#define POKE(addr,val) (*(unsigned char*)(addr) = (val))
#define PEEK(addr) (*(unsigned char*)(addr))
#define CURSOR_CHAR 160

// Temp file for paging
#define TEMP_FILE "$WHISPER$"

#endif // WHISPER64_H