#include "basic.h"
#include "editor_state.h"
#include "screen.h"

int extract_line_number(const char *line) {
    int num = 0;
    int i = 0;
    
    while (line[i] == ' ') i++;
    
    if (!isdigit(line[i])) return -1;
    
    while (isdigit(line[i])) {
        num = num * 10 + (line[i] - '0');
        i++;
    }
    
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
    
    const char *keywords[] = {"GOTO ", "GOSUB ", "GO TO ", "GO SUB ", "THEN ", "THEN", "ELSE ", "ELSE", NULL};
    
    for (int k = 0; keywords[k] != NULL; k++) {
        pos = line;
        while ((pos = strstr(pos, keywords[k])) != NULL) {
            pos += strlen(keywords[k]);
            
            while (*pos == ' ') pos++;
            
            if (strncmp(pos, search, search_len) == 0) {
                char next_char = pos[search_len];
                if (next_char == ' ' || next_char == ':' || next_char == '\0' || next_char == ',') {
                    line_len = strlen(line);
                    int pos_offset = pos - line;
                    
                    if (line_len - search_len + replace_len < MAX_LINE_LENGTH) {
                        strcpy(temp, pos + search_len);
                        strcpy(pos, replace);
                        strcpy(pos + replace_len, temp);
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
        show_message("NOT IN BASIC MODE - USE F4", COL_RED);
        return;
    }
    
    show_message("RENUMBERING...", COL_YELLOW);
    
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
    
    for (i = 0; i < num_lines; i++) {
        old_line_num = extract_line_number(lines[i]);
        if (old_line_num >= 0) {
            for (j = 0; j < num_mappings; j++) {
                if (line_mappings[j].old_num == old_line_num) {
                    int pos = 0;
                    while (isdigit(lines[i][pos])) pos++;
                    
                    sprintf(temp_line, "%d%s", line_mappings[j].new_num, &lines[i][pos]);
                    strcpy(lines[i], temp_line);
                    break;
                }
            }
        }
    }
    
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