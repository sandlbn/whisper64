#include "help.h"
#include "screen.h"

void show_help() {
    clrscr();
    
    cputs_at(0, 0, "WHISPER64 - HELP", COL_YELLOW);
    cputs_at(0, 2, "FUNCTION KEYS:", COL_CYAN);
    cputs_at(2, 3, "F1 - LOAD FILE (DIRECTORY)", COL_WHITE);
    cputs_at(2, 4, "F2 - SAVE FILE", COL_WHITE);
    cputs_at(2, 5, "F3 - SELECT DRIVE (8-15)", COL_WHITE);
    cputs_at(2, 6, "F4 - BASIC MODE & RENUMBER", COL_WHITE);
    cputs_at(2, 7, "F5 - FIND TEXT", COL_WHITE);
    cputs_at(2, 8, "F6 - FIND & REPLACE", COL_WHITE);
    cputs_at(2, 9, "F7 - FIND NEXT", COL_WHITE);
    cputs_at(2, 10, "F8 - THIS HELP SCREEN", COL_WHITE);
    
    cputs_at(0, 11, "EDITING:", COL_CYAN);
    cputs_at(2, 12, "CTRL+K - TOGGLE MARK MODE", COL_WHITE);
    cputs_at(2, 13, "CTRL+C - COPY MARKED TEXT", COL_WHITE);
    cputs_at(2, 14, "CTRL+V - PASTE TEXT", COL_WHITE);
    cputs_at(2, 15, "CTRL+Z - UNDO LAST CHANGE", COL_WHITE);
    cputs_at(2, 16, "CTRL+Y - REDO LAST CHANGE", COL_WHITE);
    cputs_at(2, 17, "CTRL+G - GOTO LINE NUMBER", COL_WHITE);
    cputs_at(2, 18, "HOME - GO TO TOP", COL_WHITE);
    cputs_at(2, 19, "ARROWS - MOVE CURSOR", COL_WHITE);
    
    cputs_at(0, 20, "BASIC MODE (F4):", COL_CYAN);
    cputs_at(2, 21, "PRESS F4 TO TOGGLE BASIC MODE", COL_WHITE);
    cputs_at(2, 22, "PRESS F4 AGAIN TO RENUMBER", COL_WHITE);
    
    cputs_at(0, 23, "PRESS ANY KEY TO CONTINUE", COL_GREEN);
    
    cgetc();
    update_cursor();
}