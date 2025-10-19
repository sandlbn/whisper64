#include "undo.h"
#include "editor_state.h"
#include "screen.h"
#include "reu.h"

// Undo state storage - 0 lines to save RAM (REU handles storage)
#define UNDO_LINES 0

// Non-REU mode disabled to save RAM
// All undo/redo now requires REU
static int undo_available = 0;
static int redo_available = 0;

// REU-based multi-level undo
static int reu_undo_head = 0;
static int reu_undo_count = 0;
static int reu_redo_head = 0;
static int reu_redo_count = 0;

void save_undo_state(void) {
    if (!reu_available) {
        // No undo without REU to save RAM
        return;
    }
    
    // REU mode: Save to circular buffer
    reu_save_undo_state(reu_undo_head);
    
    // Move head forward
    reu_undo_head = (reu_undo_head + 1) % REU_MAX_UNDO;
    
    // Increase count up to max
    if (reu_undo_count < REU_MAX_UNDO) {
        reu_undo_count++;
    }
    
    // Clear redo when new action happens
    reu_redo_count = 0;
}

void undo_last_action(void) {
    if (!reu_available) {
        show_message("UNDO REQUIRES REU", COL_RED);
        return;
    }
    
    // REU mode: Multi-level undo
    if (reu_undo_count == 0) {
        show_message("NOTHING TO UNDO", COL_RED);
        return;
    }
    
    // Save current state to redo before undo
    reu_save_undo_state(REU_MAX_UNDO + reu_redo_head);
    reu_redo_head = (reu_redo_head + 1) % REU_MAX_UNDO;
    if (reu_redo_count < REU_MAX_UNDO) {
        reu_redo_count++;
    }
    
    // Move back in undo history
    reu_undo_head = (reu_undo_head - 1 + REU_MAX_UNDO) % REU_MAX_UNDO;
    reu_undo_count--;
    
    // Restore from undo buffer
    reu_load_undo_state(reu_undo_head);
    
    page_modified = 1;
    update_cursor();
    
    char msg[40];
    sprintf(msg, "UNDO (%d LEFT)", reu_undo_count);
    show_message(msg, COL_GREEN);
}

void redo_last_action(void) {
    if (!reu_available) {
        show_message("REDO REQUIRES REU", COL_RED);
        return;
    }
    
    // REU mode: Multi-level redo
    if (reu_redo_count == 0) {
        show_message("NOTHING TO REDO", COL_RED);
        return;
    }
    
    reu_redo_head = (reu_redo_head - 1 + REU_MAX_UNDO) % REU_MAX_UNDO;
    reu_redo_count--;
    
    reu_load_undo_state(REU_MAX_UNDO + reu_redo_head);
    
    reu_undo_head = (reu_undo_head + 1) % REU_MAX_UNDO;
    if (reu_undo_count < REU_MAX_UNDO) {
        reu_undo_count++;
    }
    
    page_modified = 1;
    update_cursor();
    
    char msg[40];
    sprintf(msg, "REDO (%d LEFT)", reu_redo_count);
    show_message(msg, COL_GREEN);
}

int can_undo(void) {
    return reu_available && (reu_undo_count > 0);
}

int can_redo(void) {
    return reu_available && (reu_redo_count > 0);
}