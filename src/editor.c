#include "editor.h"
#include "editor_state.h"
#include "screen.h"
#include "reu.h"

void save_current_page_to_temp(void) {
    char temp_name[20];
    int i, len;
    
    if (!page_modified) {
        return;
    }
    
    if (reu_is_available()) {
        reu_save_page(current_page);
        page_modified = 0;
        return;
    }
    
    sprintf(temp_name, "@0:%s.P%d,S,W", TEMP_FILE, current_page);
    
    cbm_k_setlfs(2, current_drive, 2);
    cbm_k_setnam(temp_name);
    
    if (cbm_k_open() == 0) {
        cbm_k_chkout(2);
        
        for (i = 0; i < num_lines; i++) {
            len = strlen(lines[i]);
            for (int j = 0; j < len; j++) {
                cbm_k_chrout(lines[i][j]);
            }
            if (i < num_lines - 1) {
                cbm_k_chrout(13);
            }
        }
        
        cbm_k_clrch();
        cbm_k_close(2);
        page_modified = 0;
    }
}

void load_page(int page_num) {
    char temp_name[20];
    int i, pos;
    unsigned char ch;
    
    if (page_num == current_page) return;
    if (page_num < 0 || page_num >= num_pages) return;
    
    save_current_page_to_temp();
    
    current_page = page_num;
    memset(lines, 0, sizeof(lines));
    num_lines = 1;
    
    if (reu_is_available()) {
        int loaded = reu_load_page(page_num);
        if (loaded > 0) {
            num_lines = loaded;
            page_modified = 0;
            return;
        }
    }
    
    sprintf(temp_name, "%s.P%d,S,R", TEMP_FILE, page_num);
    
    cbm_k_setlfs(2, current_drive, 2);
    cbm_k_setnam(temp_name);
    
    if (cbm_k_open() == 0) {
        i = 0;
        pos = 0;
        cbm_k_chkin(2);
        
        while (i < LINES_PER_PAGE) {
            ch = cbm_k_chrin();
            if (cbm_k_readst() & 0x40) break;
            
            if (ch == '\r' || ch == '\n') {
                lines[i][pos] = '\0';
                i++;
                pos = 0;
            } else if (pos < MAX_LINE_LENGTH - 1) {
                lines[i][pos++] = ch;
            }
        }
        
        if (pos > 0) {
            lines[i][pos] = '\0';
            i++;
        }
        
        cbm_k_clrch();
        cbm_k_close(2);
        
        if (i > 0) {
            num_lines = i;
        }
    }
    
    if (num_lines < 1) {
        num_lines = 1;
    }
    
    page_modified = 0;
}

static void create_new_page(void) {
    save_current_page_to_temp();
    
    num_pages++;
    current_page++;
    
    memset(lines, 0, sizeof(lines));
    num_lines = 1;
    cursor_x = 0;
    cursor_y = 0;
    scroll_offset = 0;
    page_modified = 1;
}

void check_page_boundary(void) {
    if (cursor_y >= num_lines && cursor_y >= LINES_PER_PAGE) {
        if (current_page < num_pages - 1) {
            load_page(current_page + 1);
            cursor_y = 0;
            cursor_x = 0;
            scroll_offset = 0;
        }
    } else if (cursor_y < 0 && current_page > 0) {
        load_page(current_page - 1);
        cursor_y = num_lines - 1;
        if (cursor_y < 0) cursor_y = 0;
        scroll_offset = cursor_y - EDIT_HEIGHT + 1;
        if (scroll_offset < 0) scroll_offset = 0;
    }
}

void init_editor(void) {
    clrscr();
    memset(lines, 0, sizeof(lines));
    num_lines = 1;
    total_lines = 1;
    current_page = 0;
    num_pages = 1;
    cursor_x = 0;
    cursor_y = 0;
    scroll_offset = 0;
    search_term[0] = '\0';
    replace_term[0] = '\0';
    mark_active = 0;
    clipboard_lines = 0;
    current_filename[0] = '\0';
    page_modified = 0;
    basic_mode = 0;
    
    POKE(0xD020, 0);
    POKE(0xD021, 0);
}

void insert_char(char c) {
    int len = strlen(lines[cursor_y]);
    
    if (len < MAX_LINE_LENGTH - 1) {
        memmove(&lines[cursor_y][cursor_x + 1], 
                &lines[cursor_y][cursor_x], 
                len - cursor_x + 1);
        
        lines[cursor_y][cursor_x] = c;
        cursor_x++;
        page_modified = 1;
        
        if (cursor_x >= edit_width && len >= edit_width) {
            if (num_lines < LINES_PER_PAGE) {
                memmove(&lines[cursor_y + 2], &lines[cursor_y + 1], 
                        (num_lines - cursor_y - 1) * sizeof(lines[0]));
                
                strcpy(lines[cursor_y + 1], &lines[cursor_y][edit_width]);
                lines[cursor_y][edit_width] = '\0';
                
                num_lines++;
                total_lines++;
                cursor_y++;
                cursor_x = 0;
                
                if (cursor_y - scroll_offset >= EDIT_HEIGHT) {
                    scroll_offset++;
                }
            }
        }
    }
}

void delete_char(void) {
    int len = strlen(lines[cursor_y]);
    
    if (cursor_x > 0) {
        memmove(&lines[cursor_y][cursor_x - 1], 
                &lines[cursor_y][cursor_x], 
                len - cursor_x + 1);
        cursor_x--;
        page_modified = 1;
    } else if (cursor_y > 0) {
        int prev_len = strlen(lines[cursor_y - 1]);
        if (prev_len + len < MAX_LINE_LENGTH) {
            strcat(lines[cursor_y - 1], lines[cursor_y]);
            
            memmove(&lines[cursor_y], &lines[cursor_y + 1], 
                    (num_lines - cursor_y - 1) * sizeof(lines[0]));
            lines[num_lines - 1][0] = '\0';
            num_lines--;
            total_lines--;
            
            cursor_y--;
            cursor_x = prev_len;
            page_modified = 1;
            
            if (scroll_offset > 0 && cursor_y < scroll_offset) {
                scroll_offset--;
            }
        }
    }
}

void new_line(void) {
    if (num_lines >= LINES_PER_PAGE) {
        if (cursor_y == num_lines - 1) {
            char remainder[MAX_LINE_LENGTH];
            strcpy(remainder, &lines[cursor_y][cursor_x]);
            lines[cursor_y][cursor_x] = '\0';
            
            page_modified = 1;
            create_new_page();
            
            strcpy(lines[0], remainder);
            cursor_x = 0;
            cursor_y = 0;
            total_lines++;
            return;
        } else {
            show_message("PAGE FULL - MOVE TO END", COL_YELLOW);
            return;
        }
    }
    
    memmove(&lines[cursor_y + 2], &lines[cursor_y + 1], 
            (num_lines - cursor_y - 1) * sizeof(lines[0]));
    
    strcpy(lines[cursor_y + 1], &lines[cursor_y][cursor_x]);
    lines[cursor_y][cursor_x] = '\0';
    
    num_lines++;
    total_lines++;
    cursor_y++;
    cursor_x = 0;
    page_modified = 1;
    
    if (cursor_y - scroll_offset >= EDIT_HEIGHT) {
        scroll_offset++;
    }
    
    num_pages = (total_lines + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    if (num_pages < 1) num_pages = 1;
}