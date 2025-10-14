#include "file_ops.h"
#include "editor_state.h"
#include "editor.h"
#include "screen.h"

void select_drive() {
    char msg[40];
    char c;
    
    sprintf(msg, "DRIVE (8-15) [NOW:%d]: ", current_drive);
    show_message(msg, COL_YELLOW);
    
    c = cgetc();
    if (c >= '8' && c <= '9') {
        current_drive = c - '0';
        sprintf(msg, "DRIVE=%d", current_drive);
        show_message(msg, COL_GREEN);
    } else if (c == '1') {
        c = cgetc();
        if (c >= '0' && c <= '5') {
            current_drive = 10 + (c - '0');
            sprintf(msg, "DRIVE=%d", current_drive);
            show_message(msg, COL_GREEN);
        } else {
            show_message("CANCELLED", COL_RED);
        }
    } else {
        show_message("CANCELLED", COL_RED);
    }
    
    update_cursor();
}

void load_directory() {
    unsigned char c;
    int blocks_free = 0;
    char line_buf[40];
    int line_pos = 0;
    int in_name = 0;
    unsigned char file_type_byte;
    int file_type;
    
    num_dir_entries = 0;
    disk_name[0] = '\0';
    
    show_message("READING DIR...", COL_YELLOW);
    
    cbm_k_setlfs(2, current_drive, 0);
    cbm_k_setnam("$");
    
    if (cbm_k_open() != 0) {
        show_message("DIR ERROR", COL_RED);
        return;
    }
    
    cbm_k_chkin(2);
    
    cbm_k_chrin();
    cbm_k_chrin();
    
    while (num_dir_entries < MAX_DIR_ENTRIES - 1) {
        int link_lo = cbm_k_chrin();
        int link_hi = cbm_k_chrin();
        
        if (cbm_k_readst() & 0x40) break;
        if (link_lo == 0 && link_hi == 0) break;
        
        int blocks_lo = cbm_k_chrin();
        int blocks_hi = cbm_k_chrin();
        int blocks = blocks_lo + (blocks_hi << 8);
        
        char full_line[40];
        int full_line_pos = 0;
        memset(full_line, 0, 40);
        
        while (full_line_pos < 39) {
            c = cbm_k_chrin();
            if (c == 0) break;
            full_line[full_line_pos++] = c;
        }
        full_line[full_line_pos] = '\0';
        
        line_pos = 0;
        in_name = 0;
        memset(line_buf, 0, 40);
        
        for (int i = 0; i < full_line_pos && line_pos < 16; i++) {
            if (full_line[i] == '"') {
                if (!in_name) {
                    in_name = 1;
                } else {
                    break;
                }
            } else if (in_name) {
                line_buf[line_pos++] = full_line[i];
            }
        }
        line_buf[line_pos] = '\0';
        
        if (num_dir_entries == 0 && blocks == 0) {
            strncpy(disk_name, line_buf, 16);
            disk_name[16] = '\0';
            continue;
        }
        
        if (line_buf[0] == '\0') {
            blocks_free = blocks;
            break;
        }
        
        file_type = 2;
        file_type_byte = 0x82;
        
        if (strstr(full_line, "DEL")) {
            file_type = 0;
            file_type_byte = 0x80;
        } else if (strstr(full_line, "SEQ")) {
            file_type = 1;
            file_type_byte = 0x81;
        } else if (strstr(full_line, "PRG")) {
            file_type = 2;
            file_type_byte = 0x82;
        } else if (strstr(full_line, "USR")) {
            file_type = 3;
            file_type_byte = 0x83;
        } else if (strstr(full_line, "REL")) {
            file_type = 4;
            file_type_byte = 0x84;
        }
        
        if (strchr(full_line, '*') != NULL) {
            file_type_byte |= 0x40;
        }
        
        strncpy(dir_entries[num_dir_entries].name, line_buf, 16);
        dir_entries[num_dir_entries].name[16] = '\0';
        dir_entries[num_dir_entries].blocks = blocks;
        
        switch(file_type) {
            case 0: strcpy(dir_entries[num_dir_entries].type, "DEL"); break;
            case 1: strcpy(dir_entries[num_dir_entries].type, "SEQ"); break;
            case 2: strcpy(dir_entries[num_dir_entries].type, "PRG"); break;
            case 3: strcpy(dir_entries[num_dir_entries].type, "USR"); break;
            case 4: strcpy(dir_entries[num_dir_entries].type, "REL"); break;
            default: strcpy(dir_entries[num_dir_entries].type, "???"); break;
        }
        
        if (file_type_byte & 0x40) {
            strcat(dir_entries[num_dir_entries].type, "*");
        }
        
        num_dir_entries++;
    }
    
    cbm_k_clrch();
    cbm_k_close(2);
}

void show_directory() {
    int i, selected = 0, scroll_pos = 0;
    char c;
    
    load_directory();
    
    if (num_dir_entries == 0) {
        show_message("NO FILES - CHECK DRIVE (F3)", COL_RED);
        return;
    }
    
    while (1) {
        clrscr();
        
        char title[40];
        sprintf(title, "DIRECTORY - DRIVE %d", current_drive);
        cputs_at(2, 0, title, COL_YELLOW);
        
        if (disk_name[0]) {
            int name_len = strlen(disk_name);
            int start_x = (40 - name_len) / 2;
            cputs_at(start_x, 1, disk_name, COL_CYAN);
        }
        
        for (i = 0; i < 40; i++) {
            cputc_at(i, 2, '-', COL_CYAN);
        }
        
        cputs_at(1, 3, "BLK  FILENAME         TYPE", COL_CYAN);
        
        for (i = scroll_pos; i < scroll_pos + 18 && i < num_dir_entries; i++) {
            unsigned char color;
            char line[40];
            
            if (i == selected) {
                color = COL_GREEN;
                for (int x = 0; x < 40; x++) {
                    cputc_at(x, i - scroll_pos + 4, ' ', COL_GREEN);
                }
            } else {
                color = COL_WHITE;
            }
            
            sprintf(line, "%4d  %-16s  %s", 
                    dir_entries[i].blocks, 
                    dir_entries[i].name,
                    dir_entries[i].type);
            cputs_at(1, i - scroll_pos + 4, line, color);
        }
        
        cputs_at(0, 22, "                                        ", COL_BLUE);
        char info[40];
        sprintf(info, "%d FILES", num_dir_entries);
        cputs_at(1, 22, info, COL_BLUE);
        
        cputs_at(0, 23, " \x91=UP \x11=DOWN  RETURN=LOAD  STOP=EXIT", COL_CYAN);
        
        c = cgetc();
        
        if (c == KEY_UP && selected > 0) {
            selected--;
            if (selected < scroll_pos) scroll_pos = selected;
        } else if (c == KEY_DOWN && selected < num_dir_entries - 1) {
            selected++;
            if (selected >= scroll_pos + 18) scroll_pos = selected - 17;
        } else if (c == KEY_RETURN) {
            show_message("LOADING...", COL_YELLOW);
            
            cbm_k_setlfs(2, current_drive, 2);
            cbm_k_setnam(dir_entries[selected].name);
            
            if (cbm_k_open() == 0) {
                int line = 0, pos = 0;
                unsigned char ch;
                
                memset(lines, 0, sizeof(lines));
                cbm_k_chkin(2);
                
                while (line < LINES_PER_PAGE) {
                    ch = cbm_k_chrin();
                    if (cbm_k_readst() & 0x40) break;
                    
                    if (ch == '\r' || ch == '\n') {
                        lines[line][pos] = '\0';
                        line++;
                        pos = 0;
                    } else if (pos < MAX_LINE_LENGTH - 1) {
                        lines[line][pos++] = ch;
                    }
                }
                
                if (pos > 0 || line == 0) {
                    lines[line][pos] = '\0';
                    line++;
                }
                
                cbm_k_clrch();
                cbm_k_close(2);
                
                num_lines = line > 0 ? line : 1;
                total_lines = num_lines;
                num_pages = 1;
                current_page = 0;
                cursor_x = 0;
                cursor_y = 0;
                scroll_offset = 0;
                page_modified = 0;
                
                strcpy(current_filename, dir_entries[selected].name);
                
                update_cursor();
                show_message("LOADED!", COL_GREEN);
                return;
            } else {
                show_message("ERROR LOADING", COL_RED);
            }
        } else if (c == 3) {
            update_cursor();
            show_message("CANCELLED", COL_RED);
            return;
        }
    }
}

void save_file() {
    char filename[20];
    char full_filename[25];
    char msg[40];
    int i, len;
    int overwrite = 0;
    
    save_current_page_to_temp();
    
    if (current_filename[0] != '\0') {
        sprintf(msg, "SAVE AS [%s]: ", current_filename);
        show_message(msg, COL_YELLOW);
        strcpy(filename, current_filename);
        i = strlen(filename);
        
        for (int k = 0; k < i; k++) {
            cputc_at(9 + strlen(current_filename) + 3 + k, 24, filename[k], COL_YELLOW);
        }
    } else {
        show_message("SAVE AS: ", COL_YELLOW);
        filename[0] = '\0';
        i = 0;
    }
    
    while (1) {
        char c = cgetc();
        if (c == KEY_RETURN) break;
        if (c == KEY_DELETE && i > 0) {
            i--;
            if (current_filename[0] != '\0') {
                cputc_at(9 + strlen(current_filename) + 3 + i, 24, ' ', COL_YELLOW);
            } else {
                cputc_at(9 + i, 24, ' ', COL_YELLOW);
            }
        } else if (c >= 32 && c < 128 && i < 16) {
            filename[i] = c;
            if (current_filename[0] != '\0') {
                cputc_at(9 + strlen(current_filename) + 3 + i, 24, c, COL_YELLOW);
            } else {
                cputc_at(9 + i, 24, c, COL_YELLOW);
            }
            i++;
        }
    }
    filename[i] = '\0';
    
    if (i == 0 && current_filename[0] == '\0') {
        show_message("CANCELLED", COL_RED);
        return;
    }
    
    if (i == 0 && current_filename[0] != '\0') {
        strcpy(filename, current_filename);
    }
    
    cbm_k_setlfs(15, current_drive, 0);
    cbm_k_setnam(filename);
    if (cbm_k_open() == 0) {
        cbm_k_close(15);
        
        show_message("FILE EXISTS! OVERWRITE? (Y/N)", COL_YELLOW);
        char response = cgetc();
        
        if (response != 'Y' && response != 'y') {
            show_message("SAVE CANCELLED", COL_RED);
            return;
        }
        overwrite = 1;
    }
    
    show_message("SAVING...", COL_YELLOW);
    
    if (overwrite) {
        cbm_k_setlfs(15, current_drive, 15);
        sprintf(full_filename, "S0:%s", filename);
        cbm_k_setnam(full_filename);
        if (cbm_k_open() == 0) {
            cbm_k_close(15);
        }
    }
    
    sprintf(full_filename, "%s,S,W", filename);
    
    cbm_k_setlfs(2, current_drive, 2);
    cbm_k_setnam(full_filename);
    
    if (cbm_k_open() != 0) {
        show_message("SAVE ERROR - OPEN FAIL", COL_RED);
        return;
    }
    
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
    
    cbm_k_setlfs(15, current_drive, 15);
    cbm_k_setnam("");
    if (cbm_k_open() == 0) {
        char status[40];
        int j = 0;
        cbm_k_chkin(15);
        while (j < 39) {
            char c = cbm_k_chrin();
            if (cbm_k_readst() & 0x40) break;
            if (c == 13) break;
            status[j++] = c;
        }
        status[j] = '\0';
        cbm_k_clrch();
        cbm_k_close(15);
        
        if (status[0] != '0' || status[1] != '0') {
            show_message(status, COL_RED);
            return;
        }
    }
    
    strcpy(current_filename, filename);
    page_modified = 0;
    
    show_message("SAVED!", COL_GREEN);
}