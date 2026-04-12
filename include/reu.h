#ifndef REU_H
#define REU_H

#include "whisper64.h"
#include <stdint.h>

typedef uint32_t REUPtr;

// REU hardware registers at $DF00-$DF0A
// Must be packed to match exact hardware layout (no alignment padding)
struct __attribute__((packed)) REU {
    volatile uint8_t status;        // $DF00 - Status register
    volatile uint8_t command;       // $DF01 - Command register
    volatile uint8_t c64_base_lo;   // $DF02 - C64 base address low
    volatile uint8_t c64_base_hi;   // $DF03 - C64 base address high
    volatile uint8_t reu_base_lo;   // $DF04 - REU base address low
    volatile uint8_t reu_base_hi;   // $DF05 - REU base address high
    volatile uint8_t reu_bank;      // $DF06 - REU bank number
    volatile uint8_t length_lo;     // $DF07 - Transfer length low
    volatile uint8_t length_hi;     // $DF08 - Transfer length high
    volatile uint8_t irq;           // $DF09 - Interrupt mask register
    volatile uint8_t control;       // $DF0A - Address control register
};

#define REU_REGS (*((struct REU *)0xDF00))

// REU commands - bit 7 = execute immediately, bit 4 = no autoload
#define REU_CMD_STASH    0x90   // C64 -> REU (with immediate execute)
#define REU_CMD_FETCH    0x91   // REU -> C64 (with immediate execute)
#define REU_CMD_SWAP     0x92   // Swap C64 <-> REU

// Status register bits
#define REU_STATUS_IRQ      0x80  // Interrupt pending
#define REU_STATUS_EOB      0x40  // End of block
#define REU_STATUS_FAULT    0x20  // Verify error
#define REU_STATUS_SIZE     0x10  // 256K chips if set

#define REU_PAGE_MAGIC 0xC64E

// Helper macros for setting 16-bit address registers
#define REU_SET_C64_ADDR(addr) do { \
    REU_REGS.c64_base_lo = (uint8_t)((uint16_t)(addr) & 0xFF); \
    REU_REGS.c64_base_hi = (uint8_t)((uint16_t)(addr) >> 8); \
} while(0)

#define REU_SET_REU_ADDR(addr) do { \
    REU_REGS.reu_base_lo = (uint8_t)((uint16_t)(addr) & 0xFF); \
    REU_REGS.reu_base_hi = (uint8_t)((uint16_t)(addr) >> 8); \
    REU_REGS.reu_bank = (uint8_t)(((uint32_t)(addr)) >> 16); \
} while(0)

#define REU_SET_LENGTH(len) do { \
    REU_REGS.length_lo = (uint8_t)((uint16_t)(len) & 0xFF); \
    REU_REGS.length_hi = (uint8_t)((uint16_t)(len) >> 8); \
} while(0)

// Functions
uint8_t reu_detect(void);
void reu_init(void);
uint8_t reu_is_available(void);
uint32_t reu_get_size(void);

void reu_read(REUPtr reu_addr, void* c64_addr, uint16_t size);
void reu_write(REUPtr reu_addr, void* c64_addr, uint16_t size);

void reu_save_page(int page_num);
int reu_load_page(int page_num);

#endif