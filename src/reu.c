#include "reu.h"
#include "editor_state.h"
#include "screen.h"
#include <string.h>

unsigned int reu_available = 0;
unsigned int reu_size_kb = 0;
unsigned int reu_max_pages = 0;

// Helper to set REU addresses
static void reu_set_addresses(unsigned int c64_addr, unsigned long reu_addr, unsigned int length) {
    REU_C64_ADDR_LO = c64_addr & 0xFF;
    REU_C64_ADDR_HI = (c64_addr >> 8) & 0xFF;
    REU_REU_ADDR_LO = reu_addr & 0xFF;
    REU_REU_ADDR_HI = (reu_addr >> 8) & 0xFF;
    REU_REU_BANK = (reu_addr >> 16) & 0xFF;
    REU_TRANS_LEN_LO = length & 0xFF;
    REU_TRANS_LEN_HI = (length >> 8) & 0xFF;
}

unsigned char reu_detect(void) {
    unsigned char i;
    
    // Proper REU detection: write values to $DF02-$DF05 and check if they stick
    // If no REU, these addresses don't hold values
    for (i = 2; i <= 5; i++) {
        *(volatile unsigned char*)(0xDF00 + i) = i;
    }
    
    // Now read them back
    for (i = 5; i >= 2; i--) {
        if (*(volatile unsigned char*)(0xDF00 + i) != i) {
            return 0;  // No REU
        }
    }
    
    return 1;  // REU detected
}

void reu_init(void) {
    char msg[40];
    
    reu_available = 0;
    reu_size_kb = 0;
    reu_max_pages = 0;
    
    // Show detection attempt
    show_message("DETECTING REU...", COL_YELLOW);
    
    if (!reu_detect()) {
        show_message("NO REU DETECTED", COL_RED);
        return;
    }
    
    reu_available = 1;
    
    // For safety, assume 256KB minimum and don't probe beyond bank 4
    // This prevents hangs on large REUs
    reu_size_kb = 256;  // Conservative default
    
    // Calculate max pages (use 80% for pages, 20% for undo)
    unsigned long total_bytes = (unsigned long)reu_size_kb * 1024UL;
    unsigned long page_bytes = (total_bytes * 80UL) / 100UL;
    reu_max_pages = page_bytes / REU_PAGE_SIZE;
    
    // Limit to reasonable max
    if (reu_max_pages > REU_MAX_PAGES) {
        reu_max_pages = REU_MAX_PAGES;
    }
    
    sprintf(msg, "REU: %dKB (%d PAGES)", reu_size_kb, reu_max_pages);
    show_message(msg, COL_GREEN);
}

void reu_transfer_to(unsigned int c64_addr, unsigned long reu_addr, unsigned int length) {
    if (!reu_available || length == 0) return;
    
    // Protect screen RAM ($0400-$07FF) and color RAM ($D800-$DBFF)
    if ((c64_addr >= 0x0400 && c64_addr < 0x0800) ||
        (c64_addr >= 0xD800 && c64_addr < 0xDC00)) {
        return;  // Don't transfer from screen/color RAM
    }
    
    reu_set_addresses(c64_addr, reu_addr, length);
    REU_ADDR_CTRL = 0x00;  // Increment both addresses
    REU_COMMAND = REU_CMD_EXECUTE | REU_CMD_C64_TO_REU;
}

void reu_transfer_from(unsigned int c64_addr, unsigned long reu_addr, unsigned int length) {
    if (!reu_available || length == 0) return;
    
    // Protect screen RAM ($0400-$07FF) and color RAM ($D800-$DBFF)
    if ((c64_addr >= 0x0400 && c64_addr < 0x0800) ||
        (c64_addr >= 0xD800 && c64_addr < 0xDC00)) {
        return;  // Don't transfer to screen/color RAM
    }
    
    reu_set_addresses(c64_addr, reu_addr, length);
    REU_ADDR_CTRL = 0x00;  // Increment both addresses
    REU_COMMAND = REU_CMD_EXECUTE | REU_CMD_REU_TO_C64;
}

void reu_save_page(int page_num, const char lines[][MAX_LINE_LENGTH], int num_lines) {
    if (!reu_available || page_num >= reu_max_pages || page_num < 0) return;
    
    // Calculate REU address for this page
    unsigned long reu_addr = (unsigned long)page_num * REU_PAGE_SIZE;
    
    // Make sure we don't overflow REU memory
    if (reu_addr > 200000UL) return;  // Safety check
    
    // Transfer entire page buffer
    reu_transfer_to((unsigned int)lines, reu_addr, LINES_PER_PAGE * MAX_LINE_LENGTH);
    
    // Store num_lines at end of page
    unsigned char nl = (unsigned char)num_lines;
    reu_transfer_to((unsigned int)&nl, reu_addr + (LINES_PER_PAGE * MAX_LINE_LENGTH), 1);
}

void reu_load_page(int page_num, char lines[][MAX_LINE_LENGTH], int *num_lines) {
    if (!reu_available || page_num >= reu_max_pages || page_num < 0) return;
    
    // Calculate REU address for this page
    unsigned long reu_addr = (unsigned long)page_num * REU_PAGE_SIZE;
    
    // Make sure we don't overflow REU memory
    if (reu_addr > 200000UL) return;  // Safety check
    
    // Transfer page buffer from REU
    reu_transfer_from((unsigned int)lines, reu_addr, LINES_PER_PAGE * MAX_LINE_LENGTH);
    
    // Load num_lines
    unsigned char nl;
    reu_transfer_from((unsigned int)&nl, reu_addr + (LINES_PER_PAGE * MAX_LINE_LENGTH), 1);
    *num_lines = nl;
}

void reu_clear_page(int page_num) {
    if (!reu_available || page_num >= reu_max_pages) return;
    
    // Clear a page by writing zeros
    static char clear_buffer[MAX_LINE_LENGTH];
    memset(clear_buffer, 0, MAX_LINE_LENGTH);
    
    unsigned long reu_addr = (unsigned long)page_num * REU_PAGE_SIZE;
    
    for (int i = 0; i < LINES_PER_PAGE; i++) {
        reu_transfer_to((unsigned int)clear_buffer, reu_addr + (i * MAX_LINE_LENGTH), MAX_LINE_LENGTH);
    }
}

void reu_save_undo_state(int undo_slot) {
    if (!reu_available || undo_slot >= REU_MAX_UNDO * 2) return;
    
    // Undo storage starts after page storage
    unsigned long undo_base = (unsigned long)reu_max_pages * REU_PAGE_SIZE;
    unsigned long undo_addr = undo_base + (undo_slot * REU_PAGE_SIZE);
    
    // Save current editor state
    reu_transfer_to((unsigned int)lines, undo_addr, LINES_PER_PAGE * MAX_LINE_LENGTH);
    
    // Save metadata (cursor position, num_lines, etc)
    unsigned char metadata[8];
    metadata[0] = num_lines;
    metadata[1] = cursor_x;
    metadata[2] = cursor_y;
    metadata[3] = scroll_offset;
    metadata[4] = current_page;
    metadata[5] = num_pages;
    
    reu_transfer_to((unsigned int)metadata, undo_addr + (LINES_PER_PAGE * MAX_LINE_LENGTH), 8);
}

void reu_load_undo_state(int undo_slot) {
    if (!reu_available || undo_slot >= REU_MAX_UNDO * 2) return;
    
    unsigned long undo_base = (unsigned long)reu_max_pages * REU_PAGE_SIZE;
    unsigned long undo_addr = undo_base + (undo_slot * REU_PAGE_SIZE);
    
    // Restore editor state
    reu_transfer_from((unsigned int)lines, undo_addr, LINES_PER_PAGE * MAX_LINE_LENGTH);
    
    // Restore metadata
    unsigned char metadata[8];
    reu_transfer_from((unsigned int)metadata, undo_addr + (LINES_PER_PAGE * MAX_LINE_LENGTH), 8);
    
    num_lines = metadata[0];
    cursor_x = metadata[1];
    cursor_y = metadata[2];
    scroll_offset = metadata[3];
    current_page = metadata[4];
    num_pages = metadata[5];
}