#include "reu.h"
#include "editor_state.h"
#include "screen.h"
#include <string.h>

static uint8_t reu_available = 0;
static uint32_t reu_size = 0;
static int reu_max_pages = 0;

// Page header structure stored in REU
typedef struct {
    uint16_t magic;
    uint16_t num_lines_stored;
} REUPageHeader;

#define REU_PAGE_SIZE (sizeof(REUPageHeader) + LINES_PER_PAGE * MAX_LINE_LENGTH)

// First 256 bytes of REU reserved for future use
#define REU_DATA_OFFSET 256

// Compiler memory barrier - tells the compiler that memory may have changed
// due to DMA. Without this, the compiler can optimize away reads/writes
// that the REU DMA depends on.
#define DMA_BARRIER() __asm__ volatile("" ::: "memory")

static REUPtr reu_page_addr(int page_num) {
    return REU_DATA_OFFSET + ((REUPtr)page_num * REU_PAGE_SIZE);
}

uint8_t reu_detect(void) {
    volatile uint8_t orig, test;

    // Save original value of REU base low register
    orig = REU_REGS.reu_base_lo;

    // Write test pattern 0xAA
    REU_REGS.reu_base_lo = 0xAA;
    test = REU_REGS.reu_base_lo;
    if (test != 0xAA) {
        REU_REGS.reu_base_lo = orig;
        return 0;
    }

    // Write complementary pattern 0x55
    REU_REGS.reu_base_lo = 0x55;
    test = REU_REGS.reu_base_lo;

    // Restore original
    REU_REGS.reu_base_lo = orig;

    return (test == 0x55);
}

void reu_init(void) {
    reu_available = reu_detect();
    if (reu_available) {
        REU_REGS.control = 0;
        REU_REGS.reu_bank = 0;
        reu_size = reu_get_size();
        if (reu_size > 0) {
            reu_max_pages = (reu_size - REU_DATA_OFFSET) / REU_PAGE_SIZE;
        } else {
            reu_max_pages = 0;
        }
    }
}

uint8_t reu_is_available(void) {
    return reu_available;
}

void reu_read(REUPtr reu_addr, void* c64_addr, uint16_t size) {
    if (!reu_available || size == 0) return;

    REU_SET_C64_ADDR((uint16_t)c64_addr);
    REU_SET_REU_ADDR(reu_addr);
    REU_SET_LENGTH(size);
    REU_REGS.control = 0;
    REU_REGS.command = REU_CMD_FETCH;  // DMA happens here - CPU halted
    DMA_BARRIER();  // Tell compiler: memory at c64_addr was changed by DMA
}

void reu_write(REUPtr reu_addr, void* c64_addr, uint16_t size) {
    if (!reu_available || size == 0) return;

    DMA_BARRIER();  // Ensure all pending writes to c64_addr are flushed
    REU_SET_C64_ADDR((uint16_t)c64_addr);
    REU_SET_REU_ADDR(reu_addr);
    REU_SET_LENGTH(size);
    REU_REGS.control = 0;
    REU_REGS.command = REU_CMD_STASH;  // DMA happens here - CPU halted
}

int reu_max_page_count(void) {
    return reu_max_pages;
}

void reu_clear_pages(void) {
    static uint16_t zero = 0;
    int i;
    if (!reu_available) return;
    for (i = 0; i < reu_max_pages && i < 64; i++) {
        reu_write(reu_page_addr(i), &zero, sizeof(zero));
    }
}

void reu_save_page(int page_num) {
    REUPtr addr;
    REUPageHeader header;

    if (!reu_available) return;
    if (page_num >= reu_max_pages) return;

    addr = reu_page_addr(page_num);

    header.magic = REU_PAGE_MAGIC;
    header.num_lines_stored = num_lines;
    reu_write(addr, &header, sizeof(header));
    addr += sizeof(header);

    reu_write(addr, lines, LINES_PER_PAGE * MAX_LINE_LENGTH);
}

int reu_load_page(int page_num) {
    REUPtr addr;
    REUPageHeader header;

    if (!reu_available) return 0;
    if (page_num >= reu_max_pages) return 0;

    addr = reu_page_addr(page_num);

    reu_read(addr, &header, sizeof(header));

    if (header.magic != REU_PAGE_MAGIC) {
        return 0;
    }

    if (header.num_lines_stored == 0 || header.num_lines_stored > LINES_PER_PAGE) {
        return 0;
    }

    addr += sizeof(header);

    reu_read(addr, lines, LINES_PER_PAGE * MAX_LINE_LENGTH);

    return header.num_lines_stored;
}

uint32_t reu_get_size(void) {
    // Must be volatile - REU DMA reads/writes these behind the compiler's back
    static volatile uint8_t test_byte;
    static volatile uint8_t read_byte;
    uint32_t size = 0;
    uint8_t bank;

    if (!reu_available) return 0;

    for (bank = 0; bank < 255; bank++) {
        test_byte = 0xA5;

        // Stash test_byte to REU bank N, address 0
        REU_SET_C64_ADDR((uint16_t)&test_byte);
        REU_SET_REU_ADDR(0);
        REU_REGS.reu_bank = bank;
        REU_SET_LENGTH(1);
        REU_REGS.control = 0;
        REU_REGS.command = REU_CMD_STASH;

        // Change test byte so we know a fetch actually happened
        test_byte = 0x5A;
        read_byte = 0x00;

        // Fetch from REU bank N, address 0 into read_byte
        REU_SET_C64_ADDR((uint16_t)&read_byte);
        REU_SET_REU_ADDR(0);
        REU_REGS.reu_bank = bank;
        REU_SET_LENGTH(1);
        REU_REGS.control = 0;
        REU_REGS.command = REU_CMD_FETCH;

        // If we didn't read back what we wrote, this bank doesn't exist
        if (read_byte != 0xA5) {
            break;
        }

        size = ((uint32_t)(bank + 1)) << 16;
    }

    return size;
}
