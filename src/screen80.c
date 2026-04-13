#include "screen80.h"
#include "font_4x8.h"
#include "editor_state.h"
#include <string.h>

// Saved VIC-II register state for restoring 40-col mode
static uint8_t saved_d018;
static uint8_t saved_d011;
static uint8_t saved_dd00;

// Drawing depth counter - supports nested begin/end pairs
// Only the outermost pair actually banks ROM in/out
static uint8_t draw_depth = 0;
static uint8_t saved_01;

void screen80_init(void) {
    // Font is pre-built in font_4x8.h - no runtime init needed
}

void screen80_begin_draw(void) {
    if (draw_depth == 0) {
        __asm__ volatile("sei");
        saved_01 = PEEK(0x01);
        // Clear LORAM, HIRAM, CHAREN: pure RAM everywhere
        // $D000-$DFFF = RAM (so writes to $D800 go to DRAM, not color SRAM)
        // $E000-$FFFF = RAM (bitmap writable)
        POKE(0x01, saved_01 & 0xF8);
    }
    draw_depth++;
}

void screen80_end_draw(void) {
    if (draw_depth == 0) return;
    draw_depth--;
    if (draw_depth == 0) {
        POKE(0x01, saved_01);
        __asm__ volatile("cli");
    }
}

void screen80_enable(void) {
    saved_d018 = PEEK(0xD018);
    saved_d011 = PEEK(0xD011);
    saved_dd00 = PEEK(0xDD00);

    POKE(0xDD00, (PEEK(0xDD00) & 0xFC));
    // $68: video matrix at $1800 in bank ($C000+$1800=$D800), bitmap at $2000 ($E000)
    POKE(0xD018, 0x68);
    POKE(0xD011, PEEK(0xD011) | 0x20);
    POKE(0xD020, 0);
    POKE(0xD021, 0);

    screen_mode = MODE_80COL;
    clrscr_80();

}

void screen80_disable(void) {
    while (draw_depth > 0) screen80_end_draw();
    POKE(0xDD00, saved_dd00);
    POKE(0xD011, saved_d011);
    POKE(0xD018, saved_d018);
    screen_mode = MODE_40COL;
}

void clrscr_80(void) {
    // Must be inside banking block - $D800 writes need I/O banked out
    // so they go to DRAM (not color SRAM)
    screen80_begin_draw();
    memset(BITMAP_BASE, 0, 8000);
    memset(SCREEN_RAM_80, 0x10, 1000);
    screen80_end_draw();
}

// Core character renderer - draws one 4px-wide character into the bitmap
// Call screen80_begin_draw() before and screen80_end_draw() after batch rendering
void cputc_at_80(int x, int y, char c, uint8_t color) {
    int cell_col, side;
    uint8_t *bmp;
    const uint8_t *fnt;
    int font_idx;
    uint8_t need_end = 0;

    if (x < 0 || x >= 80 || y < 0 || y >= 25) return;

    cell_col = x >> 1;
    side = x & 1;

    font_idx = (uint8_t)c - 32;
    if (font_idx < 0 || font_idx >= 96) font_idx = 0;
    fnt = &font_4x8[font_idx * 8];

    bmp = BITMAP_BASE + (uint16_t)y * 320 + (uint16_t)cell_col * 8;

    // If not already in draw mode, enter it for this single char
    if (draw_depth == 0) {
        screen80_begin_draw();
        need_end = 1;
    }

    // Set color - must be inside banking block so write goes to DRAM at $D800
    SCREEN_RAM_80[y * 40 + cell_col] = (color << 4);

    if (side == 0) {
        bmp[0] = (bmp[0] & 0x0F) | (fnt[0] & 0xF0);
        bmp[1] = (bmp[1] & 0x0F) | (fnt[1] & 0xF0);
        bmp[2] = (bmp[2] & 0x0F) | (fnt[2] & 0xF0);
        bmp[3] = (bmp[3] & 0x0F) | (fnt[3] & 0xF0);
        bmp[4] = (bmp[4] & 0x0F) | (fnt[4] & 0xF0);
        bmp[5] = (bmp[5] & 0x0F) | (fnt[5] & 0xF0);
        bmp[6] = (bmp[6] & 0x0F) | (fnt[6] & 0xF0);
        bmp[7] = (bmp[7] & 0x0F) | (fnt[7] & 0xF0);
    } else {
        bmp[0] = (bmp[0] & 0xF0) | (fnt[0] >> 4);
        bmp[1] = (bmp[1] & 0xF0) | (fnt[1] >> 4);
        bmp[2] = (bmp[2] & 0xF0) | (fnt[2] >> 4);
        bmp[3] = (bmp[3] & 0xF0) | (fnt[3] >> 4);
        bmp[4] = (bmp[4] & 0xF0) | (fnt[4] >> 4);
        bmp[5] = (bmp[5] & 0xF0) | (fnt[5] >> 4);
        bmp[6] = (bmp[6] & 0xF0) | (fnt[6] >> 4);
        bmp[7] = (bmp[7] & 0xF0) | (fnt[7] >> 4);
    }

    if (need_end) screen80_end_draw();
}

void cputs_at_80(int x, int y, const char *s, uint8_t color) {
    screen80_begin_draw();
    while (*s && x < 80) {
        cputc_at_80(x, y, *s, color);
        s++;
        x++;
    }
    screen80_end_draw();
}

// Fast bulk renderer: draw an entire screen row (all 80 columns) into bitmap
// Caller must pass a pre-built 80-char buffer (line number + text + padding)
// Always cell-aligned for maximum speed - no per-char function calls
void render_line_80(int screen_row, const char *buf, int buf_len,
                    int start_col, int num_cols, uint8_t color) {
    uint8_t *bmp;
    uint8_t *col_row;
    int cell;
    const uint8_t *lfnt, *rfnt;
    uint8_t colbyte;
    int fi;

    // Set color for this row
    col_row = SCREEN_RAM_80 + (uint16_t)screen_row * 40;
    colbyte = color << 4;

    // Ensure ROM is banked out for bitmap writes
    screen80_begin_draw();

    // Pre-calculate row bitmap pointer
    bmp = BITMAP_BASE + (uint16_t)screen_row * 320;

    // Process all 40 cells (80 columns / 2 chars per cell)
    for (cell = 0; cell < 40; cell++) {
        int col = cell * 2;
        char lc, rc;

        // Get left and right characters from buffer
        lc = (col < buf_len) ? buf[col] : ' ';
        rc = (col + 1 < buf_len) ? buf[col + 1] : ' ';

        // Font lookup - left char
        fi = (uint8_t)lc - 32;
        if (fi >= 96) fi = 0;
        lfnt = &font_4x8[fi * 8];

        // Font lookup - right char
        fi = (uint8_t)rc - 32;
        if (fi >= 96) fi = 0;
        rfnt = &font_4x8[fi * 8];

        // Set color for this cell
        col_row[cell] = colbyte;

        // Write both chars at once - no read-modify-write needed
        bmp[0] = (lfnt[0] & 0xF0) | (rfnt[0] >> 4);
        bmp[1] = (lfnt[1] & 0xF0) | (rfnt[1] >> 4);
        bmp[2] = (lfnt[2] & 0xF0) | (rfnt[2] >> 4);
        bmp[3] = (lfnt[3] & 0xF0) | (rfnt[3] >> 4);
        bmp[4] = (lfnt[4] & 0xF0) | (rfnt[4] >> 4);
        bmp[5] = (lfnt[5] & 0xF0) | (rfnt[5] >> 4);
        bmp[6] = (lfnt[6] & 0xF0) | (rfnt[6] >> 4);
        bmp[7] = (lfnt[7] & 0xF0) | (rfnt[7] >> 4);

        bmp += 8;
    }

    screen80_end_draw();
}
