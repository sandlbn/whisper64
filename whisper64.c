/*
 * Whisper64 for Commodore 64
 * 
 * KEYS: F1=LOAD F2=SAVE F3=DRIVE F5=FIND F6=REPLACE F7=NEXT F8=HELP
 *       CBM+M=MARK CBM+C=COPY CBM+V=PASTE
 * 
 * Compile with LLVM-MOS: cmake --build build
 */

#include <stdio.h>
#include <string.h>
#include <cbm.h>
#include <ctype.h>

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 25
#define EDIT_WIDTH 37
#define EDIT_HEIGHT 23
#define MAX_LINES 256
#define MAX_LINE_LENGTH 220
#define MAX_DIR_ENTRIES 100
#define LINES_PER_PAGE 64

#define SCREEN_RAM ((unsigned char *)0x0400)

// Key codes
#define KEY_RETURN 13
#define KEY_DELETE 20
#define KEY_F1 133
#define KEY_F2 137
#define KEY_F3 134
#define KEY_F4 138  // Shift+F3
#define KEY_F5 135
#define KEY_F6 139
#define KEY_F7 136
#define KEY_F8 140  // F8 for help
#define KEY_HOME 19
#define KEY_UP 145
#define KEY_DOWN 17
#define KEY_LEFT 157
#define KEY_RIGHT 29

#define POKE(addr,val) (*(unsigned char*)(addr) = (val))
#define PEEK(addr) (*(unsigned char*)(addr))

// Text buffer
char lines[LINES_PER_PAGE][MAX_LINE_LENGTH];
int num_lines = 1;
int total_lines = 1;
int current_page = 0;
int num_pages = 1;
int cursor_x = 0;
int cursor_y = 0;
int scroll_offset = 0;
int current_drive = 8;  // Current drive number
char current_filename[17] = "";  // Remember loaded filename
char page_modified = 0;

// Search/Replace state
char search_term[40];
char replace_term[40];
int search_line = 0;
int search_pos = 0;

// Copy/Paste state
char clipboard[8][MAX_LINE_LENGTH];
int clipboard_lines = 0;
int mark_active = 0;
int mark_start_x = 0, mark_start_y = 0;
int mark_end_x = 0, mark_end_y = 0;

// BASIC mode
int basic_mode = 0;

// Directory browser
typedef struct {
    char name[17];
    int blocks;
    char type[5];
} DirEntry;

DirEntry dir_entries[MAX_DIR_ENTRIES];
int num_dir_entries = 0;
char disk_name[17];

// Temporary file for paging
#define TEMP_FILE "$WHISPER$"

// Line number mapping for BASIC renumbering
typedef struct {
    int old_num;
    int new_num;
} LineMapping;

LineMapping line_mappings[MAX_LINES];
int num_mappings = 0;

// Colors
#define COL_WHITE 1
#define COL_CYAN 3
#define COL_YELLOW 7
#define COL_RED 2
#define COL_GREEN 5
#define COL_BLUE 6
#define COL_PURPLE 4
#define COL_ORANGE 8

// Cursor character (reverse space)
#define CURSOR_CHAR 160

// BASIC keywords for syntax highlighting
const char *basic_keywords[] = {
    "PRINT", "FOR", "NEXT", "IF", "THEN", "GOTO", "GOSUB", "RETURN",
    "REM", "LET", "DIM", "READ", "DATA", "RESTORE", "STOP", "END",
    "INPUT", "GET", "POKE", "PEEK", "SYS", "LOAD", "SAVE", "RUN",
    "LIST", "NEW", "CLR", "AND", "OR", "NOT", "TO", "STEP",
    NULL
};

// Screen functions
void clrscr() {
    int i;
    for (i = 0; i < 1000; i++) {
        SCREEN_RAM[i] = ' ';
        COLOR_RAM[i] = COL_WHITE;
    }
}

char cgetc() {
    char c;
    while (!(c = cbm_k_getin()));
    return c;
}

void cputc_at(int x, int y, char c, unsigned char color) {
    int pos = y * SCREEN_WIDTH + x;
    SCREEN_RAM[pos] = c;
    COLOR_RAM[pos] = color;
}

void cputs_at(int x, int y, const char *s, unsigned char color) {
    int pos = y * SCREEN_WIDTH + x;
    while (*s) {
        SCREEN_RAM[pos] = *s++;
        COLOR_RAM[pos] = color;
        pos++;
    }
}

int is_basic_keyword(const char *word) {
    int i;
    for (i = 0; basic_keywords[i] != NULL; i++) {
        if (strcmp(word, basic_keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Paging functions
void save_current_page_to_temp() {
    char temp_name[20];
    int i, len;
    
    if (!page_modified || total_lines <= LINES_PER_PAGE) {
        return;
    }
    
    sprintf(temp_name, "%s.P%d", TEMP_FILE, current_page);
    
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
    
    // Save current page if modified
    save_current_page_to_temp();
    
    current_page = page_num;
    memset(lines, 0, sizeof(lines));
    
    if (total_lines <= LINES_PER_PAGE) {
        // All in memory, no need to load
        num_lines = total_lines;
        return;
    }
    
    sprintf(temp_name, "%s.P%d", TEMP_FILE, page_num);
    
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
        
        if (pos > 0 || i == 0) {
            lines[i][pos] = '\0';
            i++;
        }
        
        cbm_k_clrch();
        cbm_k_close(2);
        
        num_lines = i;
    } else {
        num_lines = 1;
    }
}

void check_page_boundary() {
    int global_line = current_page * LINES_PER_PAGE + cursor_y;
    
    // Check if we need to move to next page
    if (cursor_y >= LINES_PER_PAGE && current_page < num_pages - 1) {
        load_page(current_page + 1);
        cursor_y = 0;
        scroll_offset = 0;
    }
    // Check if we need to move to previous page
    else if (cursor_y < 0 && current_page > 0) {
        load_page(current_page - 1);
        cursor_y = LINES_PER_PAGE - 1;
        scroll_offset = cursor_y - EDIT_HEIGHT + 1;
        if (scroll_offset < 0) scroll_offset = 0;
    }
}

void init_editor() {
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

void draw_line_number(int screen_row, int line_num) {
    int pos = screen_row * SCREEN_WIDTH;
    
    if (line_num < num_lines) {
        SCREEN_RAM[pos] = (line_num / 10) + '0';
        SCREEN_RAM[pos + 1] = (line_num % 10) + '0';
        SCREEN_RAM[pos + 2] = ':';
        
        COLOR_RAM[pos] = COL_CYAN;
        COLOR_RAM[pos + 1] = COL_CYAN;
        COLOR_RAM[pos + 2] = COL_CYAN;
    } else {
        SCREEN_RAM[pos] = ' ';
        SCREEN_RAM[pos + 1] = ' ';
        SCREEN_RAM[pos + 2] = ' ';
    }
}

void draw_text_line(int screen_row, int line_num) {
    int pos = screen_row * SCREEN_WIDTH + 3;
    int i, j;
    char word[20];
    int word_len = 0;
    unsigned char color;
    
    if (line_num < num_lines) {
        int len = strlen(lines[line_num]);
        int is_marked;
        
        for (i = 0; i < EDIT_WIDTH; i++) {
            color = COL_WHITE;
            is_marked = 0;
            
            // Check if this position is marked
            if (mark_active) {
                int start_y = mark_start_y < mark_end_y ? mark_start_y : mark_end_y;
                int end_y = mark_start_y < mark_end_y ? mark_end_y : mark_start_y;
                
                if (line_num >= start_y && line_num <= end_y) {
                    if (start_y == end_y) {
                        int start_x = mark_start_x < mark_end_x ? mark_start_x : mark_end_x;
                        int end_x = mark_start_x < mark_end_x ? mark_end_x : mark_start_x;
                        is_marked = (i >= start_x && i < end_x);
                    } else if (line_num == start_y) {
                        is_marked = (i >= (line_num == mark_start_y ? mark_start_x : 0));
                    } else if (line_num == end_y) {
                        is_marked = (i < (line_num == mark_end_y ? mark_end_x : len));
                    } else {
                        is_marked = 1;
                    }
                }
            }
            
            if (i < len) {
                char c = lines[line_num][i];
                
                // BASIC keyword highlighting
                if (isupper(c) || c == '$' || c == '%') {
                    if (word_len < 19) {
                        word[word_len++] = c;
                    }
                } else {
                    if (word_len > 0) {
                        word[word_len] = '\0';
                        if (is_basic_keyword(word)) {
                            for (j = 0; j < word_len && i - word_len + j >= 0; j++) {
                                COLOR_RAM[pos + i - word_len + j] = COL_PURPLE;
                            }
                        }
                        word_len = 0;
                    }
                }
                
                SCREEN_RAM[pos + i] = c;
                if (is_marked) {
                    COLOR_RAM[pos + i] = COL_YELLOW;
                } else if (COLOR_RAM[pos + i] != COL_PURPLE) {
                    COLOR_RAM[pos + i] = color;
                }
            } else {
                SCREEN_RAM[pos + i] = ' ';
                COLOR_RAM[pos + i] = COL_WHITE;
            }
        }
        
        if (word_len > 0) {
            word[word_len] = '\0';
            if (is_basic_keyword(word)) {
                for (j = 0; j < word_len; j++) {
                    COLOR_RAM[pos + i - word_len + j] = COL_PURPLE;
                }
            }
        }
    } else {
        for (i = 0; i < EDIT_WIDTH; i++) {
            SCREEN_RAM[pos + i] = ' ';
        }
    }
}

void redraw_screen() {
    int i;
    char title[40];
    int global_line = current_page * LINES_PER_PAGE + cursor_y + 1;
    
    // Title with filename
    if (current_filename[0] != '\0') {
        sprintf(title, "%.8s", current_filename);
        cputs_at(0, 0, title, COL_YELLOW);
    } else {
        cputs_at(0, 0, "WHISPER", COL_YELLOW);
    }
    
    // Cursor position (Line:Column)
    char pos_info[15];
    sprintf(pos_info, " %d:%d", global_line, cursor_x + 1);
    cputs_at(8, 0, pos_info, COL_GREEN);
    
    // BASIC mode indicator
    int next_x = 17;
    if (basic_mode) {
        cputs_at(next_x, 0, "[BAS]", COL_PURPLE);
        next_x += 5;
    }
    
    if (mark_active) {
        cputs_at(next_x, 0, "[M]", COL_GREEN);
        next_x += 3;
    }
    
    // Drive info
    char drive_info[10];
    sprintf(drive_info, " D:%d", current_drive);
    cputs_at(30, 0, drive_info, COL_CYAN);
    
    // Page info if multiple pages
    if (num_pages > 1) {
        char page_info[10];
        sprintf(page_info, " P%d/%d", current_page + 1, num_pages);
        cputs_at(22, 0, page_info, COL_CYAN);
    }
    
    for (i = next_x; i < (num_pages > 1 ? 22 : 30); i++) {
        cputc_at(i, 0, ' ', COL_YELLOW);
    }
    
    for (i = 0; i < EDIT_HEIGHT; i++) {
        draw_line_number(i + 1, scroll_offset + i);
        draw_text_line(i + 1, scroll_offset + i);
    }
    
    cputs_at(0, 24, "                                        ", COL_CYAN);
}

void draw_cursor() {
    int screen_y = cursor_y - scroll_offset + 1;
    int screen_x = cursor_x + 3;
    
    if (screen_y >= 1 && screen_y <= EDIT_HEIGHT) {
        int pos = screen_y * SCREEN_WIDTH + screen_x;
        int len = strlen(lines[cursor_y]);
        
        if (cursor_x < len) {
            // Show cursor as reverse of current character
            SCREEN_RAM[pos] = lines[cursor_y][cursor_x] + 128;
            COLOR_RAM[pos] = COL_WHITE;
        } else {
            // Show cursor as reverse space at end of line
            SCREEN_RAM[pos] = CURSOR_CHAR;
            COLOR_RAM[pos] = COL_WHITE;
        }
    }
}

void update_cursor() {
    redraw_screen();
    draw_cursor();
}

void show_message(const char *msg, unsigned char col) {
    int i;
    for (i = 0; i < SCREEN_WIDTH; i++) {
        cputc_at(i, 24, ' ', col);
    }
    cputs_at(0, 24, msg, col);
    
    // Add mode indicator at the end if in BASIC mode
    if (basic_mode) {
        cputs_at(32, 24, "[BASIC]", COL_PURPLE);
    }
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
        
        if (cursor_x >= EDIT_WIDTH) {
            if (num_lines < LINES_PER_PAGE - 1) {
                memmove(&lines[cursor_y + 2], &lines[cursor_y + 1], 
                        (num_lines - cursor_y - 1) * sizeof(lines[0]));
                
                strcpy(lines[cursor_y + 1], &lines[cursor_y][EDIT_WIDTH]);
                lines[cursor_y][EDIT_WIDTH] = '\0';
                
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

void delete_char() {
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

void new_line() {
    if (num_lines < LINES_PER_PAGE - 1) {
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
        
        // Update page count if needed
        num_pages = (total_lines + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    }
}

// BASIC renumbering functions
int extract_line_number(const char *line) {
    int num = 0;
    int i = 0;
    
    // Skip leading spaces
    while (line[i] == ' ') i++;
    
    // Check if line starts with a number
    if (!isdigit(line[i])) return -1;
    
    // Extract the number
    while (isdigit(line[i])) {
        num = num * 10 + (line[i] - '0');
        i++;
    }
    
    // Must be followed by space or end
    if (line[i] != ' ' && line[i] != '\0') return -1;
    
    return num;
}

void replace_line_number(char *line, int old_num, int new_num) {
    char temp[MAX_LINE_LENGTH];
    char *pos;
    char search[10];
    char replace[10];
    int line_len, search_len, replace_len;
    
    sprintf(search, "%d", old_num);
    sprintf(replace, "%d", new_num);
    search_len = strlen(search);
    replace_len = strlen(replace);
    
    // Look for GOTO, GOSUB, THEN, ELSE followed by the line number
    const char *keywords[] = {"GOTO ", "GOSUB ", "GO TO ", "GO SUB ", "THEN ", "THEN", "ELSE ", "ELSE", NULL};
    
    for (int k = 0; keywords[k] != NULL; k++) {
        pos = line;
        while ((pos = strstr(pos, keywords[k])) != NULL) {
            // Move past the keyword
            pos += strlen(keywords[k]);
            
            // Skip spaces after keyword
            while (*pos == ' ') pos++;
            
            // Check if this is our number
            if (strncmp(pos, search, search_len) == 0) {
                // Make sure it's followed by a delimiter
                char next_char = pos[search_len];
                if (next_char == ' ' || next_char == ':' || next_char == '\0' || next_char == ',') {
                    // Build replacement string
                    line_len = strlen(line);
                    int pos_offset = pos - line;
                    
                    if (line_len - search_len + replace_len < MAX_LINE_LENGTH) {
                        // Save the part after the number
                        strcpy(temp, pos + search_len);
                        
                        // Insert new number
                        strcpy(pos, replace);
                        
                        // Append the rest
                        strcpy(pos + replace_len, temp);
                        
                        // Continue searching from after the replacement
                        pos += replace_len;
                        page_modified = 1;
                    }
                }
            }
        }
    }
}

void renumber_basic() {
    int i, j;
    int old_line_num, new_line_num;
    char temp_line[MAX_LINE_LENGTH];
    
    if (!basic_mode) {
        show_message("NOT IN BASIC MODE - USE CBM+B", COL_RED);
        return;
    }
    
    show_message("RENUMBERING...", COL_YELLOW);
    
    // Step 1: Build mapping of old line numbers to new ones
    num_mappings = 0;
    new_line_num = 10;
    
    for (i = 0; i < num_lines; i++) {
        old_line_num = extract_line_number(lines[i]);
        if (old_line_num >= 0) {
            line_mappings[num_mappings].old_num = old_line_num;
            line_mappings[num_mappings].new_num = new_line_num;
            num_mappings++;
            new_line_num += 10;
        }
    }
    
    if (num_mappings == 0) {
        show_message("NO BASIC LINE NUMBERS FOUND", COL_RED);
        return;
    }
    
    // Step 2: Renumber the lines themselves
    for (i = 0; i < num_lines; i++) {
        old_line_num = extract_line_number(lines[i]);
        if (old_line_num >= 0) {
            // Find the new number for this line
            for (j = 0; j < num_mappings; j++) {
                if (line_mappings[j].old_num == old_line_num) {
                    // Find where the line number ends
                    int pos = 0;
                    while (isdigit(lines[i][pos])) pos++;
                    
                    // Build new line with new number
                    sprintf(temp_line, "%d%s", line_mappings[j].new_num, &lines[i][pos]);
                    strcpy(lines[i], temp_line);
                    break;
                }
            }
        }
    }
    
    // Step 3: Update all GOTO/GOSUB/THEN/ELSE references
    for (i = 0; i < num_lines; i++) {
        for (j = 0; j < num_mappings; j++) {
            replace_line_number(lines[i], line_mappings[j].old_num, line_mappings[j].new_num);
        }
    }
    
    page_modified = 1;
    update_cursor();
    
    char msg[40];
    sprintf(msg, "RENUMBERED %d LINES", num_mappings);
    show_message(msg, COL_GREEN);
}

// Copy/Paste functions
void mark_toggle() {
    if (!mark_active) {
        mark_active = 1;
        mark_start_x = cursor_x;
        mark_start_y = cursor_y;
        mark_end_x = cursor_x;
        mark_end_y = cursor_y;
        show_message("MARK ON - ARROWS, CBM+C=COPY", COL_GREEN);
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
        show_message("NO MARK - PRESS CBM+M", COL_RED);
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
    
    show_message("COPIED - CBM+V TO PASTE", COL_GREEN);
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

// Drive selection
void show_help() {
    clrscr();
    
    cputs_at(0, 0, "WHISPER64 - HELP", COL_YELLOW);
    cputs_at(0, 2, "FUNCTION KEYS:", COL_CYAN);
    cputs_at(2, 3, "F1 - LOAD FILE (DIRECTORY)", COL_WHITE);
    cputs_at(2, 4, "F2 - SAVE FILE", COL_WHITE);
    cputs_at(2, 5, "F3 - SELECT DRIVE (8-15)", COL_WHITE);
    cputs_at(2, 6, "F4 - BASIC MODE & RENUMBER", COL_WHITE);
    cputs_at(2, 7, "F5 - FIND TEXT", COL_WHITE);
    cputs_at(2, 8, "F6 - FIND & REPLACE", COL_WHITE);
    cputs_at(2, 9, "F7 - FIND NEXT", COL_WHITE);
    cputs_at(2, 10, "F8 - THIS HELP SCREEN", COL_WHITE);
    
    cputs_at(0, 11, "EDITING:", COL_CYAN);
    cputs_at(2, 12, "CBM+M - TOGGLE MARK MODE", COL_WHITE);
    cputs_at(2, 13, "CBM+C - COPY MARKED TEXT", COL_WHITE);
    cputs_at(2, 14, "CBM+V - PASTE TEXT", COL_WHITE);
    cputs_at(2, 15, "HOME - GO TO TOP", COL_WHITE);
    cputs_at(2, 16, "ARROWS - MOVE CURSOR", COL_WHITE);
    
    cputs_at(0, 18, "BASIC MODE (F4):", COL_CYAN);
    cputs_at(2, 19, "PRESS F4 TO TOGGLE BASIC MODE", COL_WHITE);
    cputs_at(2, 20, "PRESS F4 AGAIN TO RENUMBER", COL_WHITE);
    cputs_at(2, 21, "UPDATES GOTO/GOSUB/THEN/ELSE", COL_WHITE);
    
    cputs_at(0, 23, "PRESS ANY KEY TO CONTINUE", COL_GREEN);
    
    cgetc();
    update_cursor();
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

// Directory browser
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
    
    // Skip load address (2 bytes)
    cbm_k_chrin();
    cbm_k_chrin();
    
    // Read directory entries
    while (num_dir_entries < MAX_DIR_ENTRIES - 1) {
        // Read link address (2 bytes)
        int link_lo = cbm_k_chrin();
        int link_hi = cbm_k_chrin();
        
        if (cbm_k_readst() & 0x40) break;  // EOF
        if (link_lo == 0 && link_hi == 0) break;  // End of directory
        
        // Read blocks (2 bytes)
        int blocks_lo = cbm_k_chrin();
        int blocks_hi = cbm_k_chrin();
        int blocks = blocks_lo + (blocks_hi << 8);
        
        // Read the entire line into a buffer
        char full_line[40];
        int full_line_pos = 0;
        memset(full_line, 0, 40);
        
        while (full_line_pos < 39) {
            c = cbm_k_chrin();
            if (c == 0) break;  // End of line
            full_line[full_line_pos++] = c;
        }
        full_line[full_line_pos] = '\0';
        
        // Parse the line to extract filename
        line_pos = 0;
        in_name = 0;
        memset(line_buf, 0, 40);
        
        for (int i = 0; i < full_line_pos && line_pos < 16; i++) {
            if (full_line[i] == '"') {
                if (!in_name) {
                    in_name = 1;  // Start of filename
                } else {
                    break;  // End of filename
                }
            } else if (in_name) {
                line_buf[line_pos++] = full_line[i];
            }
        }
        line_buf[line_pos] = '\0';
        
        // First line is disk name
        if (num_dir_entries == 0 && blocks == 0) {
            strncpy(disk_name, line_buf, 16);
            disk_name[16] = '\0';
            continue;
        }
        
        // Check if this is "BLOCKS FREE" line - empty filename or special blocks value
        if (line_buf[0] == '\0') {
            blocks_free = blocks;
            break;
        }
        
        // Extract file type from the line (last 3-4 chars usually)
        file_type = 2;  // Default to PRG
        file_type_byte = 0x82;  // PRG, closed
        
        // Look for file type keywords in the full line
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
        
        // Check for locked files (asterisk after type)
        if (strchr(full_line, '*') != NULL) {
            file_type_byte |= 0x40;
        }
        
        // Store entry
        strncpy(dir_entries[num_dir_entries].name, line_buf, 16);
        dir_entries[num_dir_entries].name[16] = '\0';
        dir_entries[num_dir_entries].blocks = blocks;
        
        // Set file type string based on type byte
        switch(file_type) {
            case 0: strcpy(dir_entries[num_dir_entries].type, "DEL"); break;
            case 1: strcpy(dir_entries[num_dir_entries].type, "SEQ"); break;
            case 2: strcpy(dir_entries[num_dir_entries].type, "PRG"); break;
            case 3: strcpy(dir_entries[num_dir_entries].type, "USR"); break;
            case 4: strcpy(dir_entries[num_dir_entries].type, "REL"); break;
            default: strcpy(dir_entries[num_dir_entries].type, "???"); break;
        }
        
        // Add asterisk for locked files
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
        
        // Title with drive number
        char title[40];
        sprintf(title, "DIRECTORY - DRIVE %d", current_drive);
        cputs_at(2, 0, title, COL_YELLOW);
        
        // Disk name centered
        if (disk_name[0]) {
            int name_len = strlen(disk_name);
            int start_x = (40 - name_len) / 2;
            cputs_at(start_x, 1, disk_name, COL_CYAN);
        }
        
        // Draw a line separator
        for (i = 0; i < 40; i++) {
            cputc_at(i, 2, '-', COL_CYAN);
        }
        
        // Column headers
        cputs_at(1, 3, "BLK  FILENAME         TYPE", COL_CYAN);
        
        // File list - starting at row 4
        for (i = scroll_pos; i < scroll_pos + 18 && i < num_dir_entries; i++) {
            unsigned char color;
            char line[40];
            
            // Different color for selected item
            if (i == selected) {
                color = COL_GREEN;
                // Draw selection bar
                for (int x = 0; x < 40; x++) {
                    cputc_at(x, i - scroll_pos + 4, ' ', COL_GREEN);
                }
            } else {
                color = COL_WHITE;
            }
            
            // Format: blocks (right-aligned), 2 spaces, filename (left), 2 spaces, type
            sprintf(line, "%4d  %-16s  %s", 
                    dir_entries[i].blocks, 
                    dir_entries[i].name,
                    dir_entries[i].type);
            cputs_at(1, i - scroll_pos + 4, line, color);
        }
        
        // Bottom info line
        cputs_at(0, 22, "                                        ", COL_BLUE);
        char info[40];
        sprintf(info, "%d FILES", num_dir_entries);
        cputs_at(1, 22, info, COL_BLUE);
        
        // Help text
        cputs_at(0, 23, " \x91=UP \x11=DOWN  RETURN=LOAD  STOP=EXIT", COL_CYAN);
        
        c = cgetc();
        
        if (c == KEY_UP && selected > 0) {
            selected--;
            if (selected < scroll_pos) scroll_pos = selected;
        } else if (c == KEY_DOWN && selected < num_dir_entries - 1) {
            selected++;
            if (selected >= scroll_pos + 18) scroll_pos = selected - 17;
        } else if (c == KEY_RETURN) {
            // Load selected file
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
                
                // Remember the filename
                strcpy(current_filename, dir_entries[selected].name);
                
                update_cursor();
                show_message("LOADED!", COL_GREEN);
                return;
            } else {
                show_message("ERROR LOADING", COL_RED);
            }
        } else if (c == 3) {  // RUN/STOP
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
    
    // Save current page first
    save_current_page_to_temp();
    
    // Pre-fill with current filename if exists
    if (current_filename[0] != '\0') {
        sprintf(msg, "SAVE AS [%s]: ", current_filename);
        show_message(msg, COL_YELLOW);
        strcpy(filename, current_filename);
        i = strlen(filename);
        
        // Display the default filename
        for (int k = 0; k < i; k++) {
            cputc_at(9 + strlen(current_filename) + 3 + k, 24, filename[k], COL_YELLOW);
        }
    } else {
        show_message("SAVE AS: ", COL_YELLOW);
        filename[0] = '\0';
        i = 0;
    }
    
    // Get filename input
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
    
    // If empty and no default, cancel
    if (i == 0 && current_filename[0] == '\0') {
        show_message("CANCELLED", COL_RED);
        return;
    }
    
    // Use default if user just pressed RETURN
    if (i == 0 && current_filename[0] != '\0') {
        strcpy(filename, current_filename);
    }
    
    // Check if file exists by trying to open for read
    cbm_k_setlfs(15, current_drive, 0);
    cbm_k_setnam(filename);
    if (cbm_k_open() == 0) {
        cbm_k_close(15);
        
        // File exists - ask for confirmation
        show_message("FILE EXISTS! OVERWRITE? (Y/N)", COL_YELLOW);
        char response = cgetc();
        
        if (response != 'Y' && response != 'y') {
            show_message("SAVE CANCELLED", COL_RED);
            return;
        }
        overwrite = 1;
    }
    
    show_message("SAVING...", COL_YELLOW);
    
    // If file exists, we need to scratch it first, then save
    if (overwrite) {
        // Send scratch command to command channel
        cbm_k_setlfs(15, current_drive, 15);
        sprintf(full_filename, "S0:%s", filename);
        cbm_k_setnam(full_filename);
        if (cbm_k_open() == 0) {
            cbm_k_close(15);
        }
    }
    
    // Now save the file
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
            cbm_k_chrout(13);  // Carriage return between lines
        }
    }
    
    cbm_k_clrch();
    cbm_k_close(2);
    
    // Check drive status
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
        
        // Check if error (status doesn't start with "00")
        if (status[0] != '0' || status[1] != '0') {
            show_message(status, COL_RED);
            return;
        }
    }
    
    // Remember this filename for next save
    strcpy(current_filename, filename);
    page_modified = 0;
    
    show_message("SAVED!", COL_GREEN);
}

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

int main(void) {
    char c;
    unsigned char commodore_key;
    
    init_editor();
    update_cursor();
    
    show_message("F1=LOAD F2=SAVE F4=BASIC F8=HELP", COL_CYAN);
    
    while (1) {
        c = cgetc();
        
        // Check if Commodore key is pressed (bit 7 of location 653)
        commodore_key = PEEK(653) & 0x80;
        
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
        // Commodore key combinations
        else if (commodore_key && (c == 'M' || c == 'm') && cursor_y < num_lines) {
            mark_toggle();
        } else if (commodore_key && (c == 'C' || c == 'c') && cursor_y < num_lines) {
            copy_marked();
        } else if (commodore_key && (c == 'V' || c == 'v') && cursor_y < num_lines) {
            paste_clipboard();
        }
        // Navigation and editing
        else if (c == KEY_RETURN) {
            new_line();
            update_cursor();
        } else if (c == KEY_DELETE) {
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
            insert_char(c);
            update_cursor();
        }
    }
    
    return 0;
}