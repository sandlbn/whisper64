#ifndef UNDO_H
#define UNDO_H

#include "whisper64.h"

// Undo/Redo operations
void save_undo_state(void);
void undo_last_action(void);
void redo_last_action(void);
int can_undo(void);
int can_redo(void);

#endif // UNDO_H