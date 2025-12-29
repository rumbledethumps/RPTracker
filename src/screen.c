#include "screen.h"
#include "constants.h"
#include "input.h"
#include <rp6502.h>
#include "usb_hid_keys.h"
#include <stdio.h>
#include "instruments.h"
#include "player.h"

char message[MESSAGE_LENGTH + 1]; // Text message buffer (+1 for null terminator)

// Tracker Cursor
uint8_t cur_pattern = 0;
uint8_t cur_row = 0;        // 0-63
uint8_t cur_channel = 0;    // 0-8
bool edit_mode = false;     // Are we recording?

void write_cell(uint8_t pat, uint8_t row, uint8_t chan, PatternCell *cell) {
    // 2304 bytes per pattern (64 rows * 9 channels * 4 bytes)
    uint16_t addr = PATTERN_XRAM_BASE + (pat * 2304) + (row * 36) + (chan * 4);
    RIA.addr0 = addr;
    RIA.step0 = 1;
    RIA.rw0 = cell->note;
    RIA.rw0 = cell->inst;
    RIA.rw0 = cell->vol;
    RIA.rw0 = cell->effect;
}

void read_cell(uint8_t pat, uint8_t row, uint8_t chan, PatternCell *cell) {
    uint16_t addr = PATTERN_XRAM_BASE + (pat * 2304) + (row * 36) + (chan * 4);
    RIA.addr0 = addr;
    RIA.step0 = 1;
    cell->note   = RIA.rw0;
    cell->inst   = RIA.rw0;
    cell->vol    = RIA.rw0;
    cell->effect = RIA.rw0;
}

const char* const note_names[] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
};

// Change colors of a range of characters on a specific screen row
void set_text_color(uint8_t x, uint8_t y, uint8_t len, uint8_t fg, uint8_t bg) {
    uint16_t addr = text_message_addr + (y * 80 + x) * 3;
    RIA.addr0 = addr;
    RIA.step0 = 1;
    for (uint8_t i = 0; i < len; i++) {
        RIA.addr0++;     // Skip Character byte
        RIA.rw0 = fg;    // Write Foreground
        RIA.rw0 = bg;    // Write Background
    }
}

void draw_note(uint16_t vga_addr, uint8_t midi_note) {
    const char* const names[] = {"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
    RIA.addr0 = vga_addr;
    RIA.step0 = 3;
    
    if (midi_note == 0) {
        // Transparent symmetry: 3 spaces
        RIA.rw0 = ' '; RIA.rw0 = ' '; RIA.rw0 = ' ';
    } else if (midi_note == 255) {
        // Note off
        RIA.rw0 = '='; RIA.rw0 = '='; RIA.rw0 = '=';
    } else {
        uint8_t note = midi_note % 12;
        uint8_t octave = (midi_note / 12) - 1;
        RIA.rw0 = names[note][0];
        RIA.rw0 = names[note][1];
        RIA.rw0 = '0' + octave;
    }
}

// Formatting helpers
const char hex_chars[] = "0123456789ABCDEF";

void draw_hex_byte(uint16_t vga_addr, uint8_t val) {
    RIA.addr0 = vga_addr;
    RIA.step0 = 3; 
    RIA.rw0 = hex_chars[val >> 4];
    RIA.rw0 = hex_chars[val & 0x0F];
}

// pattern_row_idx: The row index in the pattern data (0-31)
void render_row(uint8_t pattern_row_idx) {
    uint8_t screen_y = pattern_row_idx + GRID_SCREEN_OFFSET;
    uint16_t vga_row_ptr = text_message_addr + (screen_y * 80 * 3);
    
    // 1. Draw Row Number (Indices 0-3): "00 |"
    RIA.addr0 = vga_row_ptr;
    RIA.step0 = 3;
    RIA.rw0 = hex_chars[(pattern_row_idx >> 4) & 0x0F];
    RIA.rw0 = hex_chars[pattern_row_idx & 0x0F];
    RIA.rw0 = ' ';
    RIA.rw0 = '|';

    // 2. Draw 9 Channels
    for (uint8_t ch = 0; ch < 9; ch++) {
        PatternCell cell;
        read_cell(cur_pattern, pattern_row_idx, ch, &cell);
        
        // Data block starts at index 4, 12, 20...
        uint16_t ch_ptr = vga_row_ptr + (4 + ch * 8) * 3;
        
        // Note: index x, x+1, x+2
        draw_note(ch_ptr, cell.note);
        
        // Instrument: index x+3, x+4
        draw_hex_byte(ch_ptr + (3 * 3), cell.inst);
        
        // Volume: index x+5, x+6
        draw_hex_byte(ch_ptr + (5 * 3), cell.vol);

        // Channel Divider: index x+7
        RIA.addr0 = ch_ptr + (7 * 3);
        RIA.rw0 = '|';
    }
}

void render_grid(void) {
    // We are showing 32 rows (0x00 to 0x1F)
    for (uint8_t i = 0; i < 32; i++) {
        render_row(i); // 'i' becomes 'pattern_row_idx' inside the function
    }
}

void set_row_color(uint8_t row_idx, uint8_t bg_color) {
    // Point to the BG byte (3rd byte) of the first character in the row
    uint16_t addr = text_message_addr + ((row_idx + 2) * 80 * 3) + 2;
    RIA.addr0 = addr;
    RIA.step0 = 3; // Jump to next BG byte
    
    for (uint8_t i = 0; i < 80; i++) {
        RIA.rw0 = bg_color;
    }
}

void update_cursor_visuals(uint8_t old_row, uint8_t new_row, uint8_t old_ch, uint8_t new_ch) {
    uint8_t old_y = old_row + GRID_SCREEN_OFFSET;
    uint8_t new_y = new_row + GRID_SCREEN_OFFSET;

    // --- 1. CLEAN UP OLD STATE ---
    set_text_color(0, old_y, 80, HUD_COL_WHITE, HUD_COL_BG);
    
    // Reset Header: "CH x" is at index 6 within the 8-char block (starting at 4).
    // So CH text is at index 6, 14, 22...
    set_text_color(6 + (old_ch * 8), 27, 4, HUD_COL_CYAN, HUD_COL_BG);

    // --- 2. APPLY NEW ROW HIGHLIGHT ---
    set_text_color(0, new_y, 80, HUD_COL_WHITE, HUD_COL_HIGHLIGHT);

    // --- 3. APPLY YELLOW CELL HIGHLIGHT ---
    
    // Highlight Row Number in Yellow (Index 0-1)
    set_text_color(0, new_y, 2, HUD_COL_YELLOW, HUD_COL_HIGHLIGHT);

    // Highlight only the ACTIVE DATA (7 chars: Note+Inst+Vol)
    // Starts at index 4, 12, 20...
    uint8_t cell_x = 4 + (new_ch * 8);
    set_text_color(cell_x, new_y, 7, HUD_COL_YELLOW, HUD_COL_HIGHLIGHT);

    // Highlight "CH x" in Header in Yellow (Index 6, 14, 22...)
    set_text_color(6 + (new_ch * 8), 27, 4, HUD_COL_YELLOW, HUD_COL_BG);
}

void draw_string(uint8_t x, uint8_t y, const char* s, uint8_t fg, uint8_t bg) {
    uint16_t addr = text_message_addr + (y * 80 + x) * 3;
    RIA.addr0 = addr;
    RIA.step0 = 1;
    while (*s) {
        RIA.rw0 = *s++;
        RIA.rw0 = fg;
        RIA.rw0 = bg;
    }
}

void draw_headers() {
    // Indices: 0123 45678901 23456789...
    // Header:  RN |  CH 0 |  CH 1 |  CH 2 |  CH 3 |  CH 4 |  CH 5 |  CH 6 |  CH 7 |  CH 8 |
    draw_string(0, 27, "RN |  CH 0 |  CH 1 |  CH 2 |  CH 3 |  CH 4 |  CH 5 |  CH 6 |  CH 7 |  CH 8 |", 
                HUD_COL_CYAN, HUD_COL_BG);
}

void draw_ui_dashboard() {
    // Row 1: Status
    // OCTAVE: 4 | INS: 00 (Piano) | CHAN: 0 | MODE: EDIT
    
    // Row 3-15: Instrument Parameters (Operator 1 & 2)
    // OP1: ATK: F  DEC: 1  SUS: 5  REL: 0  MULT: 1  WAVE: 0
    // OP2: ATK: D  DEC: 2  SUS: 7  REL: 6  MULT: 1  WAVE: 0
    // FEEDBACK: 6  CONNECTION: 0 (FM)
}

void clear_top_ui() {
    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;
    // Clear only the top 27 rows
    for (uint16_t i = 0; i < (GRID_SCREEN_OFFSET * 80); i++) {
        RIA.rw0 = ' ';
        RIA.rw0 = HUD_COL_WHITE;
        RIA.rw0 = HUD_COL_BG;
    }
}

void update_dashboard(void) {
    const OPL_Patch* p = &gm_bank[current_instrument];

    // 1. Header Line (Row 1)
    draw_string(2, 1, "INSTRUMENT:", HUD_COL_CYAN, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (1 * 80 + 14) * 3, current_instrument);
    
    // You can add a name lookup here if you have one, or just a placeholder
    draw_string(18, 1, "OCTAVE:", HUD_COL_CYAN, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (1 * 80 + 26) * 3, current_octave);
    
    draw_string(32, 1, "MODE:", HUD_COL_CYAN, HUD_COL_BG);
    draw_string(38, 1, edit_mode ? "RECORDING" : "PLAYING  ", 
                edit_mode ? HUD_COL_RED : HUD_COL_GREEN, HUD_COL_BG);

    // 2. Operator Panels (Rows 3-10)
    // --- MODULATOR (OP1) ---
    draw_string(2, 4, "[ MODULATOR ]", HUD_COL_YELLOW, HUD_COL_BG);
    draw_string(2, 6, "MULT/VIB: ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (6 * 80 + 12) * 3, p->m_ave);
    
    draw_string(2, 7, "KSL/LEV:  ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (7 * 80 + 12) * 3, p->m_ksl);
    
    draw_string(2, 8, "ATK/DEC:  ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (8 * 80 + 12) * 3, p->m_atdec);
    
    draw_string(2, 9, "SUS/REL:  ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (9 * 80 + 12) * 3, p->m_susrel);

    // --- CARRIER (OP2) ---
    draw_string(25, 4, "[ CARRIER ]", HUD_COL_YELLOW, HUD_COL_BG);
    draw_string(25, 6, "MULT/VIB: ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (6 * 80 + 35) * 3, p->c_ave);
    
    draw_string(25, 7, "KSL/LEV:  ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (7 * 80 + 35) * 3, p->c_ksl);
    
    draw_string(25, 8, "ATK/DEC:  ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (8 * 80 + 35) * 3, p->c_atdec);
    
    draw_string(25, 9, "SUS/REL:  ", HUD_COL_WHITE, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (9 * 80 + 35) * 3, p->c_susrel);

    // 3. Global Settings (Row 12)
    draw_string(2, 12, "FEEDBACK/CONN:", HUD_COL_CYAN, HUD_COL_BG);
    draw_hex_byte(text_message_addr + (12 * 80 + 17) * 3, p->feedback);
    
    // Connection type display (Additive vs FM)
    bool additive = (p->feedback & 0x01);
    draw_string(22, 12, additive ? "(ADDITIVE)" : "(FM SYNTH)", 
                HUD_COL_MAGENTA, HUD_COL_BG);

    // Sequencer Status (Row 1, right side)
    draw_string(55, 1, "SEQ:", HUD_COL_CYAN, HUD_COL_BG);
    if (seq.is_playing) {
        draw_string(60, 1, "PLAYING", HUD_COL_GREEN, HUD_COL_BG);
    } else {
        draw_string(60, 1, "STOPPED", HUD_COL_RED, HUD_COL_BG);
    }

}