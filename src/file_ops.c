#include "file_ops.h"
#include "editor_state.h"
#include "editor.h"
#include "screen.h"
#include "mouse.h"
#include "reu.h"

// Compact extension table
static const char ext_table[] = 
    "C  S" "H  S" "TXTS" "BASS" "ASMS" "S  S" "INCS" 
    "CFGS" "MD S" "DOCS" "LOGS" "INIS" "XMLS" "JSNS"
    "CSVS" "DATS" "PRGP" "BINP" "A65S" "CC S" "CPPS"
    "SEQS" "USRU" "RELR" "DELD" "\0";

// Optimized extension checking - uses much less memory
static char check_ext_type(const char* filename) {
    int len = strlen(filename);
    if (len < 2) return 'P'; // Default PRG
    
    // Find last dot
    const char* ext = NULL;
    for (int i = len - 1; i >= 0; i--) {
        if (filename[i] == '.') {
            ext = &filename[i + 1];
            break;
        }
    }
    
    if (!ext) return 'P'; // No extension = PRG
    
    // Convert first 3 chars of extension to uppercase
    char ext_up[4];
    int i;
    for (i = 0; i < 3 && ext[i]; i++) {
        ext_up[i] = (ext[i] >= 'a' && ext[i] <= 'z') ? ext[i] - 32 : ext[i];
    }
    while (i < 3) ext_up[i++] = ' '; // Pad with spaces
    ext_up[3] = '\0';
    
    // Search in compact table
    const char* p = ext_table;
    while (*p) {
        if (p[0] == ext_up[0] && p[1] == ext_up[1] && p[2] == ext_up[2]) {
            return p[3]; // Return type character
        }
        p += 4; // Move to next entry
    }
    
    return 'P'; // Default PRG for unknown
}

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
    char line_buf[40];
    int line_pos = 0;
    int in_name = 0;
    unsigned char file_type_byte;
    
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
            break;
        }
        
        // Determine file type - check native CBM types first
        const char* type_str = "PRG";
        file_type_byte = 0x82;
        
        if (strstr(full_line, "DEL")) {
            type_str = "DEL";
            file_type_byte = 0x80;
        } else if (strstr(full_line, "SEQ")) {
            type_str = "SEQ";
            file_type_byte = 0x81;
        } else if (strstr(full_line, "PRG")) {
            type_str = "PRG";
            file_type_byte = 0x82;
        } else if (strstr(full_line, "USR")) {
            type_str = "USR";
            file_type_byte = 0x83;
        } else if (strstr(full_line, "REL")) {
            type_str = "REL";
            file_type_byte = 0x84;
        } else {
            // Check extension for file type
            char ext_type = check_ext_type(line_buf);
            if (ext_type == 'S') {
                type_str = "SEQ";
                file_type_byte = 0x81;
            } else if (ext_type == 'U') {
                type_str = "USR";
                file_type_byte = 0x83;
            } else if (ext_type == 'R') {
                type_str = "REL";
                file_type_byte = 0x84;
            } else if (ext_type == 'D') {
                type_str = "DEL";
                file_type_byte = 0x80;
            }
        }
        
        if (strchr(full_line, '*') != NULL) {
            file_type_byte |= 0x40;
        }
        
        strncpy(dir_entries[num_dir_entries].name, line_buf, 16);
        dir_entries[num_dir_entries].name[16] = '\0';
        dir_entries[num_dir_entries].blocks = blocks;
        strcpy(dir_entries[num_dir_entries].type, type_str);
        
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

            // Determine how to open based on the CBM file type from directory
            char load_name[30];
            if (strcmp(dir_entries[selected].type, "SEQ") == 0) {
                sprintf(load_name, "%s,S,R", dir_entries[selected].name);
            } else {
                // For PRG and other types, also open as data channel
                sprintf(load_name, "%s,P,R", dir_entries[selected].name);
            }

            cbm_k_setlfs(2, current_drive, 2);
            cbm_k_setnam(load_name);
            
            if (cbm_k_open() == 0) {
                int line = 0, pos = 0;
                int page = 0;
                int total = 0;
                unsigned char ch;
                unsigned char eof = 0;

                memset(lines, 0, sizeof(lines));
                cbm_k_chkin(2);

                while (!eof) {
                    ch = cbm_k_chrin();
                    if (cbm_k_readst() & 0x40) {
                        eof = 1;
                        // Save final partial line
                        if (pos > 0 || line == 0) {
                            lines[line][pos] = '\0';
                            line++;
                        }
                        break;
                    }

                    if (ch == '\r' || ch == '\n') {
                        lines[line][pos] = '\0';
                        line++;
                        total++;
                        pos = 0;

                        // Page full - save to REU/temp and start next page
                        if (line >= LINES_PER_PAGE) {
                            num_lines = line;
                            current_page = page;
                            page_modified = 1;
                            if (reu_is_available()) {
                                reu_save_page(page);
                            } else {
                                save_current_page_to_temp();
                            }
                            page++;
                            line = 0;
                            pos = 0;
                            memset(lines, 0, sizeof(lines));
                        }
                    } else if (pos < MAX_LINE_LENGTH - 1) {
                        lines[line][pos++] = ch;
                    }
                }

                cbm_k_clrch();
                cbm_k_close(2);

                // Final page stays in lines buffer
                num_lines = line > 0 ? line : 1;
                total += num_lines;
                num_pages = page + 1;
                current_page = 0;
                total_lines = total;
                cursor_x = 0;
                cursor_y = 0;
                scroll_offset = 0;
                page_modified = 0;

                // If multi-page, save the last page and load page 0
                if (num_pages > 1) {
                    // Save the last page we just read
                    current_page = page;
                    page_modified = 1;
                    if (reu_is_available()) {
                        reu_save_page(page);
                    } else {
                        save_current_page_to_temp();
                    }
                    // Load page 0 for display
                    memset(lines, 0, sizeof(lines));
                    current_page = 0;
                    if (reu_is_available()) {
                        int loaded = reu_load_page(0);
                        if (loaded > 0) num_lines = loaded;
                    }
                    page_modified = 0;
                }

                strcpy(current_filename, dir_entries[selected].name);

                update_cursor();
                char lmsg[40];
                sprintf(lmsg, "LOADED %d LINES %d PAGES", total_lines, num_pages);
                show_message(lmsg, COL_GREEN);
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
    char full_filename[30];
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
    
    // Check if file exists - try to open for read
    cbm_k_setlfs(2, current_drive, 2);
    sprintf(full_filename, "%s,S,R", filename);
    cbm_k_setnam(full_filename);
    if (cbm_k_open() == 0) {
        cbm_k_clrch();
        cbm_k_close(2);

        show_message("FILE EXISTS! OVERWRITE? (Y/N)", COL_YELLOW);
        char response = cgetc();

        if (response != 'Y' && response != 'y') {
            show_message("SAVE CANCELLED", COL_RED);
            return;
        }
        overwrite = 1;
    } else {
        // Clear any pending drive error from failed open
        cbm_k_close(2);
    }
    
    show_message("SAVING...", COL_YELLOW);

    sprintf(full_filename, "@0:%s,S,W", filename);

    cbm_k_setlfs(2, current_drive, 2);
    cbm_k_setnam(full_filename);

    if (cbm_k_open() != 0) {
        show_message("SAVE ERROR - OPEN FAIL", COL_RED);
        return;
    }

    cbm_k_chkout(2);

    // Write all pages to file
    if (num_pages > 1) {
        int saved_page = current_page;
        int first_line_written = 0;

        // Current page was saved to REU/temp by save_current_page_to_temp above
        for (int p = 0; p < num_pages; p++) {
            int page_lines;

            // Load each page from REU/temp into lines buffer
            memset(lines, 0, sizeof(lines));
            if (reu_is_available()) {
                page_lines = reu_load_page(p);
                if (page_lines <= 0) page_lines = 1;
            } else {
                char temp_name[20];
                sprintf(temp_name, "%s.P%d,S,R", TEMP_FILE, p);
                cbm_k_setlfs(3, current_drive, 2);
                cbm_k_setnam(temp_name);
                page_lines = 1;
                if (cbm_k_open() == 0) {
                    int li = 0, po = 0;
                    unsigned char ch;
                    cbm_k_chkin(3);
                    while (li < LINES_PER_PAGE) {
                        ch = cbm_k_chrin();
                        if (cbm_k_readst() & 0x40) break;
                        if (ch == '\r' || ch == '\n') {
                            lines[li][po] = '\0';
                            li++; po = 0;
                        } else if (po < MAX_LINE_LENGTH - 1) {
                            lines[li][po++] = ch;
                        }
                    }
                    if (po > 0) { lines[li][po] = '\0'; li++; }
                    cbm_k_clrch();
                    cbm_k_close(3);
                    if (li > 0) page_lines = li;
                }
            }

            // Write this page's lines to the output file
            cbm_k_chkout(2);
            for (i = 0; i < page_lines; i++) {
                if (first_line_written) {
                    cbm_k_chrout(13);
                }
                len = strlen(lines[i]);
                for (int j = 0; j < len; j++) {
                    cbm_k_chrout(lines[i][j]);
                }
                first_line_written = 1;
            }
        }

        // Restore the page the user was on
        memset(lines, 0, sizeof(lines));
        if (reu_is_available()) {
            int loaded = reu_load_page(saved_page);
            if (loaded > 0) num_lines = loaded;
        }
        current_page = saved_page;
    } else {
        // Single page - write directly from current lines buffer
        for (i = 0; i < num_lines; i++) {
            len = strlen(lines[i]);
            for (int j = 0; j < len; j++) {
                cbm_k_chrout(lines[i][j]);
            }
            if (i < num_lines - 1) {
                cbm_k_chrout(13);
            }
        }
    }

    cbm_k_clrch();
    cbm_k_close(2);
    
    // Check error channel
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
        
        // Check for success
        // 00 = OK
        // 01 = files scratched (OK for overwrite)  
        if (status[0] == '0' && (status[1] == '0' || status[1] == '1')) {
            strcpy(current_filename, filename);
            page_modified = 0;
            show_message("SAVED!", COL_GREEN);
        } else {
            show_message(status, COL_RED);
        }
    } else {
        strcpy(current_filename, filename);
        page_modified = 0;
        show_message("SAVED!", COL_GREEN);
    }
}

void new_file() {
    char msg[40];
    // Ask for confirmation if current buffer has unsaved changes
    if (page_modified || num_lines > 1 || strlen(lines[0]) > 0) {
        show_message("CLEAR BUFFER? (Y/N)", COL_YELLOW);
        char c = cgetc();
        if (c != 'Y' && c != 'y') {
            show_message("CANCELLED", COL_RED);
            update_cursor();
            return;
        }
    }
    
    // Clear the buffer
    memset(lines, 0, sizeof(lines));
    num_lines = 1;
    total_lines = 1;
    current_page = 0;
    num_pages = 1;
    cursor_x = 0;
    cursor_y = 0;
    scroll_offset = 0;
    page_modified = 0;
    current_filename[0] = '\0';
    
    // Clear search/replace
    search_term[0] = '\0';
    replace_term[0] = '\0';
    search_line = 0;
    search_pos = 0;
    
    // Clear clipboard and marks
    clipboard_lines = 0;
    mark_active = 0;
    
    // Delete any temp files from previous session
    for (int i = 0; i < 10; i++) {
        sprintf(msg, "@0:%s.P%d", TEMP_FILE, i);
        cbm_k_setnam(msg);
        cbm_k_setlfs(15, current_drive, 15);
        cbm_k_open();
        cbm_k_close(15);
    }
    
    show_message("NEW FILE READY", COL_GREEN);
    update_cursor();
}
