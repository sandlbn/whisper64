#ifndef MOUSE_H
#define MOUSE_H

#include "whisper64.h"

// Mouse button states
#define MOUSE_BTN_NONE 0
#define MOUSE_BTN_LEFT 1
#define MOUSE_BTN_RIGHT 2

// Mouse state structure
typedef struct {
    unsigned char x;          // Character position (0-39)
    unsigned char y;          // Character position (0-24)
    unsigned char pixel_x;    // Pixel position for smoothness
    unsigned char pixel_y;    // Pixel position for smoothness
    unsigned char button;     // Current button state
    unsigned char prev_button; // Previous button state
    unsigned char enabled;    // Mouse detected and enabled
} MouseState;

// Mouse functions
void mouse_init(void);
void mouse_update(void);
void mouse_draw_cursor(void);
void mouse_hide_cursor(void);
unsigned char mouse_get_click(void);
void mouse_get_position(unsigned char *x, unsigned char *y);
unsigned char mouse_is_enabled(void);
void mouse_enable(void);
void mouse_disable(void);

// Mouse position helpers
unsigned char mouse_in_edit_area(void);
unsigned char mouse_in_status_line(void);
void mouse_to_editor_pos(unsigned char *line, unsigned char *col);

#endif // MOUSE_H