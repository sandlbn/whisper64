#ifndef BASIC_H
#define BASIC_H

#include "whisper64.h"

// BASIC operations
int extract_line_number(const char *line);
void replace_line_number(char *line, int old_num, int new_num);
void renumber_basic(void);

#endif // BASIC_H