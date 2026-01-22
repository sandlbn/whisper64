#ifndef REU_H
#define REU_H

#include "whisper64.h"
#include <stdint.h>

// REU pointer type (24-bit address space)
typedef uint32_t REUPtr;

// REU register structure at $DF00
struct REU {
    volatile uint8_t status;        // $DF00
    volatile uint8_t command;       // $DF01
    volatile uint16_t c64_base;     // $DF02-$DF03
    volatile uint16_t reu_base;     // $DF04-$DF05
    volatile uint8_t reu_base_bank; // $DF06
    volatile uint16_t length;       // $DF07-$DF08
    volatile uint8_t irq;           // $DF09
    volatile uint8_t control;       // $DF0A
};

#define reu (*((struct REU *)0xDF00))

// REU commands
#define REU_CMD_STASH    0x90  // C64 -> REU, execute immediately
#define REU_CMD_FETCH    0x91  // REU -> C64, execute immediately
#define REU_CMD_SWAP     0x92  // Swap C64 <-> REU
#define REU_CMD_VERIFY   0x93  // Compare C64 vs REU

// REU memory layout for Whisper64
// Page 0: 0x000000 - 0x00DFFF (56KB)
// Page 1: 0x00E000 - 0x01BFFF (56KB)
// etc.
#define REU_PAGE_SIZE    (LINES_PER_PAGE * MAX_LINE_LENGTH)  // ~14KB per page
#define REU_MAX_PAGES    16  // Support up to 16 pages in REU
#define REU_SIGNATURE    0x57363421  // "W64!" magic number

// Functions
uint8_t reu_detect(void);
void reu_init(void);
uint8_t reu_is_available(void);
uint32_t reu_get_size(void);

// Low-level transfer functions
void reu_read(REUPtr reu_addr, void* c64_addr, uint16_t size);
void reu_write(REUPtr reu_addr, void* c64_addr, uint16_t size);

// Page management for editor
void reu_save_page(int page_num, char lines[][MAX_LINE_LENGTH], int num_lines);
int reu_load_page(int page_num, char lines[][MAX_LINE_LENGTH]);
void reu_clear_pages(void);

#endif // REU_H