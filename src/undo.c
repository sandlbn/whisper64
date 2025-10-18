#include "undo.h"
#include "editor_state.h"
#include "screen.h"

// Undo state storage - 3 lines (maybe on C128)
#define UNDO_LINES 0

static char undo_lines[UNDO_LINES][MAX_LINE_LENGTH];
static int undo_num_lines = 0;
static int undo_cursor_x = 0;
static int undo_cursor_y = 0;
static int undo_scroll_offset = 0;
static int undo_start_line = 0;
static int undo_available = 0;

// Redo state storage
static char redo_lines[UNDO_LINES][MAX_LINE_LENGTH];
static int redo_num_lines = 0;
static int redo_cursor_x = 0;
static int redo_cursor_y = 0;
static int redo_scroll_offset = 0;
static int redo_start_line = 0;
static int redo_available = 0;

void save_undo_state(void) {
    int i;
    int lines_to_save;
    
    // Only save current line and one line before/after
    int start_line = cursor_y > 0 ? cursor_y - 1 : 0;
    int end_line = start_line + UNDO_LINES;
    if (end_line > num_lines) {
        end_line = num_lines;
        start_line = end_line - UNDO_LINES;
        if (start_line < 0) start_line = 0;
    }
    
    lines_to_save = end_line - start_line;
    
    // Copy relevant lines to undo buffer
    for (i = 0; i < lines_to_save && i < UNDO_LINES; i++) {
        strcpy(undo_lines[i], lines[start_line + i]);
    }
    
    undo_num_lines = num_lines;
    undo_cursor_x = cursor_x;
    undo_cursor_y = cursor_y;
    undo_scroll_offset = scroll_offset;
    undo_start_line = start_line;
    undo_available = 1;
    
    // Clear redo when new action happens
    redo_available = 0;
}

void undo_last_action(void) {
    int i;
    
    if (!undo_available) {
        show_message("NOTHING TO UNDO", COL_RED);
        return;
    }
    
    // Calculate which lines to save for redo
    int start_line = cursor_y > 0 ? cursor_y - 1 : 0;
    int end_line = start_line + UNDO_LINES;
    if (end_line > num_lines) {
        end_line = num_lines;
        start_line = end_line - UNDO_LINES;
        if (start_line < 0) start_line = 0;
    }
    
    int lines_to_save = end_line - start_line;
    
    // Save current state to redo buffer
    for (i = 0; i < lines_to_save && i < UNDO_LINES; i++) {
        strcpy(redo_lines[i], lines[start_line + i]);
    }
    redo_num_lines = num_lines;
    redo_cursor_x = cursor_x;
    redo_cursor_y = cursor_y;
    redo_scroll_offset = scroll_offset;
    redo_start_line = start_line;
    redo_available = 1;
    
    // Restore from undo buffer
    for (i = 0; i < UNDO_LINES && undo_start_line + i < num_lines; i++) {
        strcpy(lines[undo_start_line + i], undo_lines[i]);
    }
    
    num_lines = undo_num_lines;
    cursor_x = undo_cursor_x;
    cursor_y = undo_cursor_y;
    scroll_offset = undo_scroll_offset;
    
    page_modified = 1;
    update_cursor();
    show_message("UNDONE", COL_GREEN);
}

void redo_last_action(void) {
    int i;
    
    if (!redo_available) {
        show_message("NOTHING TO REDO", COL_RED);
        return;
    }
    
    // Restore from redo buffer
    for (i = 0; i < UNDO_LINES && redo_start_line + i < num_lines; i++) {
        strcpy(lines[redo_start_line + i], redo_lines[i]);
    }
    
    num_lines = redo_num_lines;
    cursor_x = redo_cursor_x;
    cursor_y = redo_cursor_y;
    scroll_offset = redo_scroll_offset;
    
    page_modified = 1;
    update_cursor();
    show_message("REDONE", COL_GREEN);
}

int can_undo(void) {
    return undo_available;
}

int can_redo(void) {
    return redo_available;
}