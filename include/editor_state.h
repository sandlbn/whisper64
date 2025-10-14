#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include "whisper64.h"

// Text buffer
extern char lines[LINES_PER_PAGE][MAX_LINE_LENGTH];
extern int num_lines;
extern int total_lines;
extern int current_page;
extern int num_pages;
extern int cursor_x;
extern int cursor_y;
extern int scroll_offset;
extern int current_drive;
extern char current_filename[17];
extern char page_modified;

// Search/Replace state
extern char search_term[40];
extern char replace_term[40];
extern int search_line;
extern int search_pos;

// Copy/Paste state
extern char clipboard[8][MAX_LINE_LENGTH];
extern int clipboard_lines;
extern int mark_active;
extern int mark_start_x, mark_start_y;
extern int mark_end_x, mark_end_y;

// BASIC mode
extern int basic_mode;

// Line number mapping for BASIC renumbering
typedef struct {
    int old_num;
    int new_num;
} LineMapping;

extern LineMapping line_mappings[MAX_LINES];
extern int num_mappings;

// Directory browser
typedef struct {
    char name[17];
    int blocks;
    char type[5];
} DirEntry;

extern DirEntry dir_entries[MAX_DIR_ENTRIES];
extern int num_dir_entries;
extern char disk_name[17];

// BASIC keywords
extern const char *basic_keywords[];

#endif // EDITOR_STATE_H