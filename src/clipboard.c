#include "clipboard.h"
#include "editor_state.h"
#include "editor.h"
#include "screen.h"

void mark_toggle() {
    if (!mark_active) {
        mark_active = 1;
        mark_start_x = cursor_x;
        mark_start_y = cursor_y;
        mark_end_x = cursor_x;
        mark_end_y = cursor_y;
        show_message("MARK ON - ARROWS, CTRL+C=COPY", COL_GREEN);
    } else {
        mark_active = 0;
        show_message("MARK OFF", COL_RED);
        update_cursor();
    }
}

void copy_marked() {
    int i;
    int start_y = mark_start_y < mark_end_y ? mark_start_y : mark_end_y;
    int end_y = mark_start_y < mark_end_y ? mark_end_y : mark_start_y;
    
    if (!mark_active) {
        show_message("NO MARK - PRESS CTRL+K", COL_RED);
        return;
    }
    
    clipboard_lines = 0;
    
    for (i = start_y; i <= end_y && clipboard_lines < 8; i++) {
        if (start_y == end_y) {
            int start_x = mark_start_x < mark_end_x ? mark_start_x : mark_end_x;
            int end_x = mark_start_x < mark_end_x ? mark_end_x : mark_start_x;
            strncpy(clipboard[clipboard_lines], &lines[i][start_x], end_x - start_x);
            clipboard[clipboard_lines][end_x - start_x] = '\0';
        } else if (i == start_y) {
            strcpy(clipboard[clipboard_lines], &lines[i][mark_start_y == start_y ? mark_start_x : 0]);
        } else if (i == end_y) {
            strncpy(clipboard[clipboard_lines], lines[i], mark_end_y == end_y ? mark_end_x : strlen(lines[i]));
            clipboard[clipboard_lines][mark_end_y == end_y ? mark_end_x : strlen(lines[i])] = '\0';
        } else {
            strcpy(clipboard[clipboard_lines], lines[i]);
        }
        clipboard_lines++;
    }
    
    show_message("COPIED - CTRL+V TO PASTE", COL_GREEN);
}

void paste_clipboard() {
    int i, j;
    
    if (clipboard_lines == 0) {
        show_message("CLIPBOARD EMPTY", COL_RED);
        return;
    }
    
    for (i = 0; i < clipboard_lines; i++) {
        for (j = 0; clipboard[i][j]; j++) {
            insert_char(clipboard[i][j]);
        }
        if (i < clipboard_lines - 1) {
            new_line();
        }
    }
    
    show_message("TEXT PASTED", COL_GREEN);
}