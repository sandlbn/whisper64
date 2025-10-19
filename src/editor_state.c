#include "editor_state.h"

// Text buffer
char lines[LINES_PER_PAGE][MAX_LINE_LENGTH];
int num_lines = 1;
int total_lines = 1;
int current_page = 0;
int num_pages = 1;
int cursor_x = 0;
int cursor_y = 0;
int scroll_offset = 0;
int current_drive = 8;
char current_filename[17] = "";
char page_modified = 0;

// Search/Replace state
char search_term[40];
char replace_term[40];
int search_line = 0;
int search_pos = 0;

// Copy/Paste state - REDUCED to 3 lines
char clipboard[3][MAX_LINE_LENGTH];
int clipboard_lines = 0;
int mark_active = 0;
int mark_start_x = 0, mark_start_y = 0;
int mark_end_x = 0, mark_end_y = 0;

// BASIC mode
int basic_mode = 0;

// Directory browser
DirEntry dir_entries[MAX_DIR_ENTRIES];
int num_dir_entries = 0;
char disk_name[17];

// Line number mapping
LineMapping line_mappings[MAX_LINES];
int num_mappings = 0;

// BASIC keywords for syntax highlighting
const char *basic_keywords[] = {
    "PRINT", "FOR", "NEXT", "IF", "THEN", "GOTO", "GOSUB", "RETURN",
    "REM", "LET", "DIM", "READ", "DATA", "RESTORE", "STOP", "END",
    "INPUT", "GET", "POKE", "PEEK", "SYS", "LOAD", "SAVE", "RUN",
    "LIST", "NEW", "CLR", "AND", "OR", "NOT", "TO", "STEP",
    NULL
};