#include "goto.h"
#include "editor_state.h"
#include "screen.h"

void goto_line(void) {
    char input[6];
    int i = 0;
    int line_num;
    char msg[40];
    
    show_message("GOTO LINE: ", COL_YELLOW);
    
    // Get line number input
    while (1) {
        char c = cgetc();
        
        if (c == KEY_RETURN) {
            break;
        } else if (c == KEY_DELETE && i > 0) {
            i--;
            cputc_at(11 + i, 24, ' ', COL_YELLOW);
        } else if (c >= '0' && c <= '9' && i < 5) {
            input[i] = c;
            cputc_at(11 + i, 24, c, COL_YELLOW);
            i++;
        }
    }
    
    input[i] = '\0';
    
    if (i == 0) {
        show_message("CANCELLED", COL_RED);
        return;
    }
    
    // Convert to number
    line_num = 0;
    for (int j = 0; j < i; j++) {
        line_num = line_num * 10 + (input[j] - '0');
    }
    
    // Convert to 0-based index
    line_num--;
    
    // Check if line exists
    if (line_num < 0 || line_num >= num_lines) {
        sprintf(msg, "LINE %d NOT FOUND", line_num + 1);
        show_message(msg, COL_RED);
        return;
    }
    
    // Jump to line
    cursor_y = line_num;
    cursor_x = 0;
    
    // Adjust scroll if needed
    if (cursor_y < scroll_offset) {
        scroll_offset = cursor_y;
    } else if (cursor_y >= scroll_offset + EDIT_HEIGHT) {
        scroll_offset = cursor_y - EDIT_HEIGHT + 1;
    }
    
    update_cursor();
    sprintf(msg, "LINE %d", line_num + 1);
    show_message(msg, COL_GREEN);
}