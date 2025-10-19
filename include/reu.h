#ifndef REU_H
#define REU_H

#include "whisper64.h"

// REU Hardware Registers at $DF00-$DF0A
#define REU_STATUS    (*(volatile unsigned char*)0xDF00)
#define REU_COMMAND   (*(volatile unsigned char*)0xDF01)
#define REU_C64_ADDR_LO (*(volatile unsigned char*)0xDF02)
#define REU_C64_ADDR_HI (*(volatile unsigned char*)0xDF03)
#define REU_REU_ADDR_LO (*(volatile unsigned char*)0xDF04)
#define REU_REU_ADDR_HI (*(volatile unsigned char*)0xDF05)
#define REU_REU_BANK    (*(volatile unsigned char*)0xDF06)
#define REU_TRANS_LEN_LO (*(volatile unsigned char*)0xDF07)
#define REU_TRANS_LEN_HI (*(volatile unsigned char*)0xDF08)
#define REU_INT_MASK    (*(volatile unsigned char*)0xDF09)
#define REU_ADDR_CTRL   (*(volatile unsigned char*)0xDF0A)

// Command register bits
#define REU_CMD_EXECUTE    0x80
#define REU_CMD_C64_TO_REU 0x00
#define REU_CMD_REU_TO_C64 0x01
#define REU_CMD_SWAP       0x02
#define REU_CMD_VERIFY     0x03
#define REU_CMD_AUTOLOAD   0x20
#define REU_CMD_FF00       0x10

// Address control bits
#define REU_ADDR_FIX_C64   0x80
#define REU_ADDR_FIX_REU   0x40

// REU Memory Layout for Whisper64
#define REU_PAGE_SIZE      (LINES_PER_PAGE * MAX_LINE_LENGTH)  // ~14KB per page
#define REU_MAX_PAGES      20    // Support up to 20 pages (1280 lines) with 256KB REU
#define REU_UNDO_BANK      1     // Start undo storage at bank 1
#define REU_MAX_UNDO       10    // Support 10-level undo with REU

// REU status
extern unsigned int reu_available;
extern unsigned int reu_size_kb;
extern unsigned int reu_max_pages;

// REU Functions
unsigned char reu_detect(void);
void reu_init(void);
void reu_transfer_to(unsigned int c64_addr, unsigned long reu_addr, unsigned int length);
void reu_transfer_from(unsigned int c64_addr, unsigned long reu_addr, unsigned int length);

// Page management with REU
void reu_save_page(int page_num, const char lines[][MAX_LINE_LENGTH], int num_lines);
void reu_load_page(int page_num, char lines[][MAX_LINE_LENGTH], int *num_lines);
void reu_clear_page(int page_num);

// Undo management with REU
void reu_save_undo_state(int undo_slot);
void reu_load_undo_state(int undo_slot);

#endif // REU_H