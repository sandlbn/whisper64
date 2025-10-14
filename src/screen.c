#include "screen.h"
#include "editor_state.h"

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
    
    if (current_filename[0] != '\0') {
        sprintf(title, "%.8s", current_filename);
        cputs_at(0, 0, title, COL_YELLOW);
    } else {
        cputs_at(0, 0, "WHISPER", COL_YELLOW);
    }
    
    char pos_info[15];
    sprintf(pos_info, " %d:%d", global_line, cursor_x + 1);
    cputs_at(8, 0, pos_info, COL_GREEN);
    
    int next_x = 17;
    if (basic_mode) {
        cputs_at(next_x, 0, "[BAS]", COL_PURPLE);
        next_x += 5;
    }
    
    if (mark_active) {
        cputs_at(next_x, 0, "[M]", COL_GREEN);
        next_x += 3;
    }
    
    char drive_info[10];
    sprintf(drive_info, " D:%d", current_drive);
    cputs_at(30, 0, drive_info, COL_CYAN);
    
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
            SCREEN_RAM[pos] = lines[cursor_y][cursor_x] + 128;
            COLOR_RAM[pos] = COL_WHITE;
        } else {
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
    
    if (basic_mode) {
        cputs_at(32, 24, "[BASIC]", COL_PURPLE);
    }
}