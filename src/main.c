/*
 * Whisper64 for Commodore 64
 * 
 * KEYS: F1=LOAD F2=SAVE F3=DRIVE F5=FIND F6=REPLACE F7=NEXT F8=HELP
 *       CTRL+K=MARK CTRL+C=COPY CTRL+V=PASTE
 * 
 * Compile with LLVM-MOS: cmake --build build
 */

#include "whisper64.h"
#include "editor_state.h"
#include "editor.h"
#include "screen.h"
#include "file_ops.h"
#include "search.h"
#include "clipboard.h"
#include "basic.h"
#include "help.h"
#include "undo.h"
#include "goto.h"
#include "mouse.h"

int main(void) {
    char c;
    unsigned char click;
    unsigned char clicked_line, clicked_col;
    
    init_editor();
    mouse_init();
    update_cursor();
    
    show_message("F1=LOAD F2=SAVE F4=BASIC CTRL+J=MOUSE", COL_CYAN);
    
    while (1) {
        // Always update mouse if enabled
        if (mouse_is_enabled()) {
            mouse_update();
            
            // Check for mouse clicks
            click = mouse_get_click();
            if (click == MOUSE_BTN_LEFT) {
                // Left click - position cursor
                if (mouse_in_edit_area()) {
                    // Hide mouse cursor during screen update
                    mouse_hide_cursor();
                    
                    mouse_to_editor_pos(&clicked_line, &clicked_col);
                    
                    // Validate the line number
                    if (clicked_line >= num_lines) {
                        clicked_line = num_lines - 1;
                    }
                    
                    // Update cursor position
                    cursor_y = clicked_line;
                    cursor_x = clicked_col;
                    
                    // Ensure cursor is within line bounds
                    if (cursor_x > strlen(lines[cursor_y])) {
                        cursor_x = strlen(lines[cursor_y]);
                    }
                    
                    // Adjust scroll if needed
                    if (cursor_y < scroll_offset) {
                        scroll_offset = cursor_y;
                    } else if (cursor_y >= scroll_offset + EDIT_HEIGHT) {
                        scroll_offset = cursor_y - EDIT_HEIGHT + 1;
                    }
                    
                    update_cursor();
                }
            }
            
            // Draw mouse cursor
            mouse_draw_cursor();
        }
        
        // Check for keyboard input (non-blocking)
        c = cbm_k_getin();
        if (c == 0) continue;  // No key pressed, loop again
        
        // Hide mouse cursor while processing keyboard
        if (mouse_is_enabled()) {
            mouse_hide_cursor();
        }
        
        // Toggle mouse with Ctrl+J (10)
        if (c == 10) {
            if (mouse_is_enabled()) {
                mouse_disable();
                show_message("MOUSE OFF", COL_RED);
            } else {
                mouse_enable();
                show_message("MOUSE ON - CLICK TO POSITION CURSOR", COL_GREEN);
            }
            continue;
        }

        
        // Function keys
        if (c == KEY_F1) {
            show_directory();
        } else if (c == KEY_F2) {
            save_file();
            update_cursor();
        } else if (c == KEY_F3) {
            select_drive();
        } else if (c == KEY_F4) {
            // BASIC mode toggle and renumber
            if (!basic_mode) {
                basic_mode = 1;
                update_cursor();
                show_message("BASIC MODE ON - F4 AGAIN=RENUMBER", COL_GREEN);
            } else {
                // Already in BASIC mode, ask to renumber
                show_message("RENUMBER? (Y/N)", COL_YELLOW);
                char response = cgetc();
                if (response == 'Y' || response == 'y') {
                    renumber_basic();
                } else {
                    basic_mode = 0;
                    update_cursor();
                    show_message("BASIC MODE OFF", COL_RED);
                }
            }
        } else if (c == KEY_F5) {
            search_text();
        } else if (c == KEY_F6) {
            find_and_replace();
        } else if (c == KEY_F7) {
            if (search_term[0]) {
                search_next();
            } else {
                show_message("NO SEARCH - USE F5 FIRST", COL_RED);
            }
        } else if (c == KEY_F8) {
            show_help();
        }
        // Control key combinations
        else if (c == 11) {  // Control+K for mark
            mark_toggle();
        } else if (c == 3 && cursor_y < num_lines) {  // Control+C for copy
            copy_marked();
        } else if (c == 22 && cursor_y < num_lines) {  // Control+V for paste
            paste_clipboard();
        } else if (c == 26) {  // Control+Z for undo
            undo_last_action();
        } else if (c == 25) {  // Control+Y for redo
            redo_last_action();
        } else if (c == 7) {  // Control+G for goto line
            goto_line();
        }
        // Navigation and editing
        else if (c == KEY_RETURN) {
            save_undo_state();
            new_line();
            update_cursor();
        } else if (c == KEY_DELETE) {
            save_undo_state();
            delete_char();
            update_cursor();
        } else if (c == KEY_LEFT) {
            if (cursor_x > 0) {
                cursor_x--;
            } else if (cursor_y > 0) {
                cursor_y--;
                cursor_x = strlen(lines[cursor_y]);
                if (cursor_y < scroll_offset) {
                    scroll_offset--;
                }
            }
            if (mark_active) {
                mark_end_x = cursor_x;
                mark_end_y = cursor_y;
            }
            update_cursor();
        } else if (c == KEY_RIGHT) {
            if (cursor_x < strlen(lines[cursor_y])) {
                cursor_x++;
            } else if (cursor_y < num_lines - 1) {
                cursor_y++;
                cursor_x = 0;
                if (cursor_y - scroll_offset >= EDIT_HEIGHT) {
                    scroll_offset++;
                }
            }
            if (mark_active) {
                mark_end_x = cursor_x;
                mark_end_y = cursor_y;
            }
            update_cursor();
        } else if (c == KEY_UP) {
            if (cursor_y > 0) {
                cursor_y--;
                if (cursor_x > strlen(lines[cursor_y])) {
                    cursor_x = strlen(lines[cursor_y]);
                }
                if (cursor_y < scroll_offset) {
                    scroll_offset--;
                }
            }
            if (mark_active) {
                mark_end_x = cursor_x;
                mark_end_y = cursor_y;
            }
            update_cursor();
        } else if (c == KEY_DOWN) {
            if (cursor_y < num_lines - 1) {
                cursor_y++;
                if (cursor_x > strlen(lines[cursor_y])) {
                    cursor_x = strlen(lines[cursor_y]);
                }
                if (cursor_y - scroll_offset >= EDIT_HEIGHT) {
                    scroll_offset++;
                }
            }
            if (mark_active) {
                mark_end_x = cursor_x;
                mark_end_y = cursor_y;
            }
            update_cursor();
        } else if (c == KEY_HOME) {
            cursor_x = 0;
            cursor_y = 0;
            scroll_offset = 0;
            update_cursor();
        }
        // Regular typing
        else if (c >= 32 && c < 128) {
            save_undo_state();
            insert_char(c);
            update_cursor();
        }
    }
    
    return 0;
}