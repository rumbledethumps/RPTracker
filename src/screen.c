#include "screen.h"
#include "constants.h"
#include "input.h"
#include <rp6502.h>
#include "usb_hid_keys.h"
#include <stdio.h>

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

