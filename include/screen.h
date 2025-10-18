#ifndef SCREEN_H
#define SCREEN_H

#include "whisper64.h"

// Forward declare mouse functions to avoid circular dependency
unsigned char mouse_is_enabled(void);
void mouse_hide_cursor(void);
void mouse_draw_cursor(void);

// Basic screen functions
void clrscr(void);
char cgetc(void);
void cputc_at(int x, int y, char c, unsigned char color);
void cputs_at(int x, int y, const char *s, unsigned char color);

// Display functions
void draw_line_number(int screen_row, int line_num);
void draw_text_line(int screen_row, int line_num);
void redraw_screen(void);
void draw_cursor(void);
void update_cursor(void);
void show_message(const char *msg, unsigned char col);

// Helper
int is_basic_keyword(const char *word);

#endif // SCREEN_H