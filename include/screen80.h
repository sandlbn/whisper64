#ifndef SCREEN80_H
#define SCREEN80_H

#include "whisper64.h"
#include <stdint.h>

// 80-column mode uses VIC-II Bank 3 ($C000-$FFFF)
// Bitmap at $E000 (8000 bytes), screen/color RAM at $C400
// Bitmap at $E000 (8KB, under Kernal ROM - VIC sees RAM here)
#define BITMAP_BASE     ((uint8_t *)0xE000)
// Video matrix at $D800 - uses the VIC color RAM hardware
// This avoids conflicting with program BSS which extends to ~$C430
#define SCREEN_RAM_80   ((uint8_t *)0xD800)

// 80-col layout: 4px wide chars, 80 columns, 25 rows
#define SCREEN_WIDTH_80  80
#define EDIT_WIDTH_80    77   // 80 - 3 (line number column)
#define LINE_NUM_WIDTH   3

// Screen mode flags
#define MODE_40COL  0
#define MODE_80COL  1

// Initialize 80-column subsystem (generate font from ROM)
void screen80_init(void);

// Switch between modes
void screen80_enable(void);
void screen80_disable(void);

// Batch drawing - bank Kernal ROM out once for many draw calls
void screen80_begin_draw(void);
void screen80_end_draw(void);

// 80-column rendering primitives
void clrscr_80(void);
void cputc_at_80(int x, int y, char c, uint8_t color);
void cputs_at_80(int x, int y, const char *s, uint8_t color);

// Fast bulk line renderer - processes entire row at once
void render_line_80(int screen_row, const char *text, int text_len,
                    int start_col, int num_cols, uint8_t color);

#endif
