#include "reu.h"
#include "editor_state.h"
#include "screen.h"
#include <string.h>

// REU state
static uint8_t reu_available = 0;
static uint32_t reu_size = 0;

// Page metadata stored at start of each REU page
typedef struct {
    uint16_t num_lines;
    uint16_t checksum;
} REUPageHeader;

// Calculate base address for a page in REU
static REUPtr reu_page_addr(int page_num) {
    // Skip first 256 bytes for signature/metadata
    return 256 + ((REUPtr)page_num * (REU_PAGE_SIZE + sizeof(REUPageHeader)));
}

uint8_t reu_detect(void) {
    volatile uint8_t test_val;
    volatile uint8_t orig_val;
    
    // Save original value
    orig_val = reu.reu_base;
    
    // Try to write and read back a test pattern
    reu.reu_base = 0xAA;
    test_val = reu.reu_base;
    
    if (test_val != 0xAA) {
        reu.reu_base = orig_val;
        return 0;
    }
    
    reu.reu_base = 0x55;
    test_val = reu.reu_base;
    
    if (test_val != 0x55) {
        reu.reu_base = orig_val;
        return 0;
    }
    
    // Restore and confirm REU is present
    reu.reu_base = orig_val;
    return 1;
}

uint32_t reu_get_size(void) {
    // Test REU size by writing/reading at different bank boundaries
    // Common sizes: 128KB, 256KB, 512KB, 1MB, 2MB, 16MB
    static uint8_t test_byte;
    static uint8_t read_byte;
    uint32_t size = 0;
    uint8_t bank;
    
    if (!reu_available) return 0;
    
    test_byte = 0xA5;
    
    // Test each 64KB bank
    for (bank = 0; bank < 255; bank++) {
        // Write test pattern to start of bank
        reu.c64_base = (uint16_t)&test_byte;
        reu.reu_base = 0;
        reu.reu_base_bank = bank;
        reu.length = 1;
        reu.control = 0;
        reu.command = REU_CMD_STASH;
        
        // Change test byte
        test_byte = 0x5A;
        
        // Read it back
        reu.c64_base = (uint16_t)&read_byte;
        reu.reu_base = 0;
        reu.reu_base_bank = bank;
        reu.length = 1;
        reu.command = REU_CMD_FETCH;
        
        // Restore test byte
        test_byte = 0xA5;
        
        // Check if we read back what we wrote
        if (read_byte != 0xA5) {
            break;
        }
        
        size = ((uint32_t)(bank + 1)) << 16;  // 64KB per bank
    }
    
    return size;
}

void reu_init(void) {
    reu_available = reu_detect();
    
    if (reu_available) {
        reu.control = 0;  // Increment both addresses during transfer
        reu_size = reu_get_size();
        
        // Write signature to REU
        uint32_t sig = REU_SIGNATURE;
        reu_write(0, &sig, sizeof(sig));
    }
}

uint8_t reu_is_available(void) {
    return reu_available;
}

void reu_read(REUPtr reu_addr, void* c64_addr, uint16_t size) {
    if (!reu_available || size == 0) return;
    
    reu.c64_base = (uint16_t)c64_addr;
    reu.reu_base = (uint16_t)(reu_addr & 0xFFFF);
    reu.reu_base_bank = (uint8_t)((reu_addr >> 16) & 0xFF);
    reu.length = size;
    reu.control = 0;
    reu.command = REU_CMD_FETCH;  // REU -> C64
}

void reu_write(REUPtr reu_addr, void* c64_addr, uint16_t size) {
    if (!reu_available || size == 0) return;
    
    reu.c64_base = (uint16_t)c64_addr;
    reu.reu_base = (uint16_t)(reu_addr & 0xFFFF);
    reu.reu_base_bank = (uint8_t)((reu_addr >> 16) & 0xFF);
    reu.length = size;
    reu.control = 0;
    reu.command = REU_CMD_STASH;  // C64 -> REU
}

void reu_save_page(int page_num, char lines_data[][MAX_LINE_LENGTH], int line_count) {
    REUPtr addr;
    REUPageHeader header;
    int i;
    uint16_t checksum = 0;
    
    if (!reu_available || page_num >= REU_MAX_PAGES) return;
    
    addr = reu_page_addr(page_num);
    
    // Calculate simple checksum
    for (i = 0; i < line_count; i++) {
        int j;
        for (j = 0; lines_data[i][j] && j < MAX_LINE_LENGTH; j++) {
            checksum += (uint8_t)lines_data[i][j];
        }
    }
    
    // Write header
    header.num_lines = line_count;
    header.checksum = checksum;
    reu_write(addr, &header, sizeof(header));
    addr += sizeof(header);
    
    // Write lines - transfer in chunks to handle 16-bit length limit
    for (i = 0; i < line_count; i++) {
        reu_write(addr, lines_data[i], MAX_LINE_LENGTH);
        addr += MAX_LINE_LENGTH;
    }
}

int reu_load_page(int page_num, char lines_data[][MAX_LINE_LENGTH]) {
    REUPtr addr;
    REUPageHeader header;
    int i;
    uint16_t checksum = 0;
    
    if (!reu_available || page_num >= REU_MAX_PAGES) return 0;
    
    addr = reu_page_addr(page_num);
    
    // Read header
    reu_read(addr, &header, sizeof(header));
    addr += sizeof(header);
    
    if (header.num_lines == 0 || header.num_lines > LINES_PER_PAGE) {
        return 0;
    }
    
    // Read lines
    for (i = 0; i < header.num_lines; i++) {
        reu_read(addr, lines_data[i], MAX_LINE_LENGTH);
        addr += MAX_LINE_LENGTH;
    }
    
    // Clear remaining lines
    for (; i < LINES_PER_PAGE; i++) {
        lines_data[i][0] = '\0';
    }
    
    // Verify checksum
    for (i = 0; i < header.num_lines; i++) {
        int j;
        for (j = 0; lines_data[i][j] && j < MAX_LINE_LENGTH; j++) {
            checksum += (uint8_t)lines_data[i][j];
        }
    }
    
    if (checksum != header.checksum) {
        return 0;  // Checksum mismatch
    }
    
    return header.num_lines;
}

void reu_clear_pages(void) {
    REUPageHeader header;
    int i;
    
    if (!reu_available) return;
    
    header.num_lines = 0;
    header.checksum = 0;
    
    for (i = 0; i < REU_MAX_PAGES; i++) {
        reu_write(reu_page_addr(i), &header, sizeof(header));
    }
}