#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "whisper64.h"

// Copy/paste operations
void mark_toggle(void);
void copy_marked(void);
void paste_clipboard(void);

#endif // CLIPBOARD_H