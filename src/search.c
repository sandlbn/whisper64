#include "search.h"
#include "editor_state.h"
#include "screen.h"

void search_next() {
    int i;
    int start_line = search_line;
    int start_pos = search_pos;
    
    for (i = start_line; i < num_lines; i++) {
        int search_from = (i == start_line) ? start_pos : 0;
        char *found = strstr(&lines[i][search_from], search_term);
        
        if (found) {
            search_line = i;
            search_pos = found - lines[i] + strlen(search_term);
            
            cursor_y = i;
            cursor_x = found - lines[i];
            
            if (cursor_y < scroll_offset) {
                scroll_offset = cursor_y;
            } else if (cursor_y >= scroll_offset + EDIT_HEIGHT) {
                scroll_offset = cursor_y - EDIT_HEIGHT + 1;
            }
            
            update_cursor();
            show_message("FOUND", COL_GREEN);
            return;
        }
    }
    
    for (i = 0; i < start_line; i++) {
        char *found = strstr(lines[i], search_term);
        if (found) {
            search_line = i;
            search_pos = found - lines[i] + strlen(search_term);
            
            cursor_y = i;
            cursor_x = found - lines[i];
            
            if (cursor_y < scroll_offset) {
                scroll_offset = cursor_y;
            }
            
            update_cursor();
            show_message("FOUND (WRAPPED)", COL_GREEN);
            return;
        }
    }
    
    show_message("NOT FOUND", COL_RED);
}

void search_text() {
    int i;
    
    show_message("SEARCH FOR: ", COL_YELLOW);
    
    i = 0;
    while (1) {
        char c = cgetc();
        if (c == KEY_RETURN) break;
        if (c == KEY_DELETE && i > 0) {
            i--;
            cputc_at(12 + i, 24, ' ', COL_YELLOW);
        } else if (c >= 32 && c < 128 && i < 39) {
            search_term[i] = c;
            cputc_at(12 + i, 24, c, COL_YELLOW);
            i++;
        }
    }
    search_term[i] = '\0';
    
    if (i == 0) {
        show_message("SEARCH CANCELLED", COL_RED);
        return;
    }
    
    search_line = cursor_y;
    search_pos = cursor_x;
    
    search_next();
}

void find_and_replace() {
    int i;
    char *found;
    int replace_count = 0;
    
    if (search_term[0] == '\0') {
        show_message("USE F5 TO SEARCH FIRST", COL_RED);
        return;
    }
    
    show_message("REPLACE: ", COL_YELLOW);
    
    i = 0;
    while (1) {
        char c = cgetc();
        if (c == KEY_RETURN) break;
        if (c == KEY_DELETE && i > 0) {
            i--;
            cputc_at(9 + i, 24, ' ', COL_YELLOW);
        } else if (c >= 32 && c < 128 && i < 30) {
            replace_term[i] = c;
            cputc_at(9 + i, 24, c, COL_YELLOW);
            i++;
        }
    }
    replace_term[i] = '\0';
    
    show_message("REPLACE ALL? (Y/N)", COL_YELLOW);
    char choice = cgetc();
    
    if (choice == 'Y' || choice == 'y') {
        for (i = 0; i < num_lines; i++) {
            while ((found = strstr(lines[i], search_term)) != NULL) {
                int pos = found - lines[i];
                int search_len = strlen(search_term);
                int replace_len = strlen(replace_term);
                int line_len = strlen(lines[i]);
                
                if (line_len - search_len + replace_len < MAX_LINE_LENGTH) {
                    memmove(found + replace_len, found + search_len, 
                            line_len - pos - search_len + 1);
                    memcpy(found, replace_term, replace_len);
                    replace_count++;
                    page_modified = 1;
                }
            }
        }
        
        update_cursor();
        char msg[40];
        sprintf(msg, "REPLACED %d", replace_count);
        show_message(msg, COL_GREEN);
    } else {
        search_line = 0;
        search_pos = 0;
        search_next();
        show_message("Y=YES N=SKIP ESC=DONE", COL_CYAN);
    }
}