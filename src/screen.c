#include "screen.h"
#include "constants.h"
#include "input.h"
#include <rp6502.h>
#include "usb_hid_keys.h"
#include <stdio.h>

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

void draw_note(uint16_t xram_vga_addr, uint8_t midi_note) {
    RIA.addr0 = xram_vga_addr;
    RIA.step0 = 3; // In Mode 1, skip FG and BG bytes
    
    if (midi_note == 0) {
        RIA.rw0 = '.'; RIA.rw0 = '.'; RIA.rw0 = '.';
    } else if (midi_note == 255) {
        RIA.rw0 = '='; RIA.rw0 = '='; RIA.rw0 = '='; // Note Off
    } else {
        uint8_t note = midi_note % 12;
        uint8_t octave = (midi_note / 12) - 1;
        const char* name = note_names[note];
        RIA.rw0 = name[0];
        RIA.rw0 = name[1];
        RIA.rw0 = '0' + octave;
    }
}

void handle_navigation() {
    // Arrow keys for cursor
    if (key_pressed(KEY_DOWN))  { if (cur_row < 63) cur_row++; }
    if (key_pressed(KEY_UP))    { if (cur_row > 0)  cur_row--; }
    if (key_pressed(KEY_RIGHT)) { if (cur_channel < 8) cur_channel++; }
    if (key_pressed(KEY_LEFT))  { if (cur_channel > 0) cur_channel--; }

    // Toggle Edit Mode (Space bar)
    if (key_pressed(KEY_SPACE)) {
        edit_mode = !edit_mode;
        printf("Edit Mode: %s\n", edit_mode ? "ON" : "OFF");
    }
    
    // Delete Note
    if (key_pressed(KEY_BACKSPACE) || key_pressed(KEY_DELETE)) {
        PatternCell c = {0, 0, 0, 0};
        write_cell(cur_pattern, cur_row, cur_channel, &c);
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

void render_row(uint8_t row_idx) {
    // 1. Calculate VGA start address for this row (Line starts at row 2 of screen)
    // VGA_BASE + (ScreenRow * 80 * 3)
    uint16_t vga_row_ptr = text_message_addr + ((row_idx + 2) * 80 * 3);
    
    // 2. Draw Row Number (Columns 0-2)
    RIA.addr0 = vga_row_ptr;
    RIA.step0 = 3;
    RIA.rw0 = hex_chars[(row_idx >> 4) & 0x0F];
    RIA.rw0 = hex_chars[row_idx & 0x0F];
    RIA.rw0 = '|';

    // 3. Draw 9 Channels
    for (uint8_t ch = 0; ch < 9; ch++) {
        PatternCell cell;
        read_cell(cur_pattern, row_idx, ch, &cell);
        
        // Offset for this channel: RowNum(3) + ch * ChannelWidth(8)
        uint16_t vga_cell_ptr = vga_row_ptr + (3 + ch * 8) * 3;
        
        // Draw Note (3 chars: e.g., "C-4")
        draw_note(vga_cell_ptr, cell.note);
        
        // Draw Instrument (2 chars hex)
        draw_hex_byte(vga_cell_ptr + (4 * 3), cell.inst);
        
        // Draw Volume (2 chars hex)
        draw_hex_byte(vga_cell_ptr + (6 * 3), cell.vol);
    }
}

void render_grid(void) {
    // Draw all 60 rows visible on screen
    // If the pattern has 64, we just draw the first 60 for now.
    for (uint8_t i = 0; i < 60; i++) {
        render_row(i);
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


// Add this helper to your display/ui logic
void update_cursor_visuals(uint8_t old_row, uint8_t new_row) {
    // 1. Reset the old row background to Black (0)
    // Row + 2 because of our header offset
    uint16_t old_addr = text_message_addr + ((old_row + 2) * 80 * 3) + 2;
    RIA.addr0 = old_addr;
    RIA.step0 = 3; 
    for (uint8_t i = 0; i < 80; i++) RIA.rw0 = HUD_COL_BG;

    // 2. Set the new row background to Blue (4)
    uint16_t new_addr = text_message_addr + ((new_row + 2) * 80 * 3) + 2;
    RIA.addr0 = new_addr;
    RIA.step0 = 3;
    for (uint8_t i = 0; i < 80; i++) RIA.rw0 = HUD_COL_HIGHLIGHT;
}