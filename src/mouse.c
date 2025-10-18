#include "mouse.h"
#include "editor_state.h"
#include "screen.h"
#include <stdio.h>

// CIA1 and SID registers
#define CIA1_PRA (*(unsigned char*)0xDC00)
#define CIA1_PRB (*(unsigned char*)0xDC01)
#define CIA1_DDRA (*(unsigned char*)0xDC02)
#define CIA1_DDRB (*(unsigned char*)0xDC03)
#define SID_POTX (*(unsigned char*)0xD419)
#define SID_POTY (*(unsigned char*)0xD41A)

// Mouse cursor character
#define MOUSE_CURSOR 94

// Which port? 1 = control port 1 (left), 2 = control port 2 (right)
#define MOUSE_PORT 1

// Mouse sensitivity (higher = slower). Try values between 16-32
#define MOUSE_DIVISOR 24

static MouseState mouse;
static unsigned char mouse_visible = 0;
static unsigned char old_char = ' ';
static unsigned char old_color = COL_WHITE;
static unsigned char old_x = 0;
static unsigned char old_y = 0;

// Previous POT values for delta calculation
static unsigned char oldpotx = 0;
static unsigned char oldpoty = 0;

void mouse_init(void) {
    mouse.enabled = 0;
    mouse.x = 20;
    mouse.y = 12;
    mouse.button = MOUSE_BTN_NONE;
    mouse.prev_button = MOUSE_BTN_NONE;
    oldpotx = 0;
    oldpoty = 0;
    mouse_visible = 0;
    old_x = 0;
    old_y = 0;
}

void mouse_update(void) {
    unsigned char newpotx, newpoty;
    signed int deltax, deltay;
    int new_x, new_y;
    unsigned char moved = 0;
    
    if (!mouse.enabled) return;
    
    // Read POT values
    newpotx = SID_POTX;
    newpoty = SID_POTY;
    
    // Calculate raw deltas
    deltax = (signed int)newpotx - (signed int)oldpotx;
    deltay = (signed int)newpoty - (signed int)oldpoty;
    
    // Handle wraparound for 8-bit values
    if (deltax > 127) deltax -= 256;
    if (deltax < -128) deltax += 256;
    if (deltay > 127) deltay -= 256;
    if (deltay < -128) deltay += 256;
    
    // CRITICAL: Ignore huge jumps (likely errors or initialization)
    if (deltax > 50 || deltax < -50) deltax = 0;
    if (deltay > 50 || deltay < -50) deltay = 0;
    
    // Apply movement with scaling
    if (deltax != 0) {
        deltax = deltax / MOUSE_DIVISOR;
        
        // Ensure minimum movement if there was any delta
        if (deltax == 0 && newpotx != oldpotx) {
            // Only move if the difference is significant enough
            int diff = (int)newpotx - (int)oldpotx;
            if (diff > 127) diff -= 256;
            if (diff < -128) diff += 256;
            
            if (diff > 2) deltax = 1;
            else if (diff < -2) deltax = -1;
        }
        
        if (deltax != 0) {
            new_x = (int)mouse.x + deltax;
            if (new_x < 0) new_x = 0;
            if (new_x >= SCREEN_WIDTH) new_x = SCREEN_WIDTH - 1;
            mouse.x = (unsigned char)new_x;
            moved = 1;
        }
    }
    
    // Y axis (inverted)
    if (deltay != 0) {
        deltay = -deltay / MOUSE_DIVISOR;  // Invert and scale
        
        if (deltay == 0 && newpoty != oldpoty) {
            int diff = (int)newpoty - (int)oldpoty;
            if (diff > 127) diff -= 256;
            if (diff < -128) diff += 256;
            
            if (diff > 2) deltay = -1;
            else if (diff < -2) deltay = 1;
        }
        
        if (deltay != 0) {
            new_y = (int)mouse.y + deltay;
            if (new_y < 0) new_y = 0;
            if (new_y >= SCREEN_HEIGHT) new_y = SCREEN_HEIGHT - 1;
            mouse.y = (unsigned char)new_y;
            moved = 1;
        }
    }
    
    // If mouse moved, we need to redraw it
    if (moved && mouse_visible) {
        // Restore old position first
        int old_pos = old_y * SCREEN_WIDTH + old_x;
        SCREEN_RAM[old_pos] = old_char;
        COLOR_RAM[old_pos] = old_color;
        
        // Now draw at new position
        int new_pos = mouse.y * SCREEN_WIDTH + mouse.x;
        old_char = SCREEN_RAM[new_pos];
        old_color = COLOR_RAM[new_pos];
        old_x = mouse.x;
        old_y = mouse.y;
        
        SCREEN_RAM[new_pos] = MOUSE_CURSOR;
        COLOR_RAM[new_pos] = COL_YELLOW;
    }
    
    // Update stored values
    oldpotx = newpotx;
    oldpoty = newpoty;
    
    // Read button state - Port 1 uses PRB
    mouse.prev_button = mouse.button;
    
    if (!(CIA1_PRB & 0x10)) {
        mouse.button = MOUSE_BTN_LEFT;
    } else {
        mouse.button = MOUSE_BTN_NONE;
    }
}

void mouse_hide_cursor(void) {
    if (!mouse.enabled || !mouse_visible) return;
    
    int pos = old_y * SCREEN_WIDTH + old_x;
    SCREEN_RAM[pos] = old_char;
    COLOR_RAM[pos] = old_color;
    
    mouse_visible = 0;
}

void mouse_draw_cursor(void) {
    if (!mouse.enabled) return;
    
    // Don't redraw if already visible and hasn't moved
    if (mouse_visible && old_x == mouse.x && old_y == mouse.y) {
        return;
    }
    
    // If visible but moved, hide old position first
    if (mouse_visible) {
        int old_pos = old_y * SCREEN_WIDTH + old_x;
        SCREEN_RAM[old_pos] = old_char;
        COLOR_RAM[old_pos] = old_color;
    }
    
    // Draw at current position
    int pos = mouse.y * SCREEN_WIDTH + mouse.x;
    old_char = SCREEN_RAM[pos];
    old_color = COLOR_RAM[pos];
    old_x = mouse.x;
    old_y = mouse.y;
    
    SCREEN_RAM[pos] = MOUSE_CURSOR;
    COLOR_RAM[pos] = COL_YELLOW;
    
    mouse_visible = 1;
}

unsigned char mouse_get_click(void) {
    if (mouse.button != MOUSE_BTN_NONE && 
        mouse.prev_button == MOUSE_BTN_NONE) {
        return mouse.button;
    }
    return MOUSE_BTN_NONE;
}

void mouse_get_position(unsigned char *x, unsigned char *y) {
    *x = mouse.x;
    *y = mouse.y;
}

unsigned char mouse_is_enabled(void) {
    return mouse.enabled;
}

void mouse_enable(void) {
    unsigned int i;
    volatile unsigned char temp;
    
    // Configure CIA1 DDR for Port 1 (PRB bit 4 as input for fire button)
    CIA1_DDRB &= ~0x10;  // Set bit 4 as input
    
    // Long delay to let POT values completely stabilize
    for (i = 0; i < 5000; i++) {
        temp = SID_POTX;
        temp = SID_POTY;
    }
    
    // Read initial POT values many times
    for (i = 0; i < 50; i++) {
        oldpotx = SID_POTX;
        oldpoty = SID_POTY;
    }
    
    // Start at center of screen
    mouse.x = 20;
    mouse.y = 12;
    old_x = mouse.x;
    old_y = mouse.y;
    mouse_visible = 0;
    
    mouse.enabled = 1;
}

void mouse_disable(void) {
    mouse.enabled = 0;
    if (mouse_visible) {
        mouse_hide_cursor();
    }
}

unsigned char mouse_in_edit_area(void) {
    return (mouse.y >= 1 && mouse.y <= EDIT_HEIGHT);
}

unsigned char mouse_in_status_line(void) {
    return (mouse.y == 24);
}

void mouse_to_editor_pos(unsigned char *line, unsigned char *col) {
    if (mouse.y < 1) {
        *line = 0;
        *col = 0;
        return;
    }
    
    int screen_line = mouse.y - 1;
    *line = scroll_offset + screen_line;
    
    if (mouse.x < 3) {
        *col = 0;
    } else {
        *col = mouse.x - 3;
    }
    
    if (*line >= num_lines) *line = num_lines - 1;
    if (*col > EDIT_WIDTH) *col = EDIT_WIDTH;
}