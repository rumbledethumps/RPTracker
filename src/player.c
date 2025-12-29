#include "player.h"
#include "opl.h"
#include "input.h"
#include "usb_hid_keys.h"
#include <stdio.h>
#include "screen.h"
#include "instruments.h"

// Current State
uint8_t current_instrument = 0; // Instrument index (0 = Piano)
uint8_t current_octave = 4; // Adjusts in jumps of 12
uint8_t active_midi_note = 0;      // Tracks the currently playing note

SequencerState seq = {false, 6, 0, 125};

#define KEY_REPEAT_DELAY 20 // Frames before repeat starts
#define KEY_REPEAT_RATE  4  // Frames between repeats
uint8_t repeat_timer = 0;
uint8_t last_scancode = 0;

// Piano Mapping: Scancode -> MIDI Offset from C
// (0 = C, 1 = C#, 2 = D, etc.)
static int8_t get_semitone(uint8_t scancode) {
    switch (scancode) {
        // Lower Row (C-4 to B-4)
        case KEY_Z: return 0;  case KEY_S: return 1;
        case KEY_X: return 2;  case KEY_D: return 3;
        case KEY_C: return 4;  case KEY_V: return 5;
        case KEY_G: return 6;  case KEY_B: return 7;
        case KEY_H: return 8;  case KEY_N: return 9;
        case KEY_J: return 10; case KEY_M: return 11;
        case KEY_COMMA: return 12;

        // Upper Row (C-5 to B-5)
        case KEY_Q: return 12; case KEY_2: return 13;
        case KEY_W: return 14; case KEY_3: return 15;
        case KEY_E: return 16; case KEY_R: return 17;
        case KEY_5: return 18; case KEY_T: return 19;
        case KEY_6: return 20; case KEY_Y: return 21;
        case KEY_7: return 22; case KEY_U: return 23;
        case KEY_I: return 24;

        default: return -1;
    }
}

void player_tick(void) {
    uint8_t channel = cur_channel; // Map piano to the active grid channel
    bool note_pressed_this_frame = false;
    uint8_t target_note = 0;
    uint8_t semitone = 0;

    // 1. Scan for piano keys
    for (int k = 0; k < 256; k++) {
        if (key(k)) {
            int8_t s = get_semitone(k);
            if (s != -1) {
                semitone = s;
                target_note = (current_octave + 1) * 12 + semitone;
                note_pressed_this_frame = true;
                break; 
            }
        }
    }

    // 2. Logic: Note On & Recording
    if (note_pressed_this_frame) {
        if (target_note != active_midi_note) {
            // Stop previous note if still playing
            if (active_midi_note != 0) OPL_NoteOff(channel); 
            
            // Play new note
            OPL_NoteOn(channel, target_note);
            active_midi_note = target_note;

            // --- RECORDING LOGIC ---
            if (edit_mode) {
                PatternCell c;
                c.note = target_note;
                c.inst = current_instrument; // Ensure this is the global 'brush'
                c.vol = 63; 
                c.effect = 0;
                
                write_cell(cur_pattern, cur_row, cur_channel, &c);
                render_row(cur_row); // Draw what we just recorded
                
                if (cur_row < 31) cur_row++; // Advance
            }
        }
    } 
    // 3. Logic: Note Off
    else {
        if (active_midi_note != 0) {
            OPL_NoteOff(channel);
            active_midi_note = 0;
        }
    }

    // 4. Function Keys (Octave & Instrument)
    if (key_pressed(KEY_F1)) { if (current_octave > 0) current_octave--; update_dashboard(); }
    if (key_pressed(KEY_F2)) { if (current_octave < 8) current_octave++; update_dashboard(); }

    if (key_pressed(KEY_F3)) { 
        current_instrument--;
        OPL_SetPatch(cur_channel, &gm_bank[current_instrument]);
        update_dashboard();
    }
    if (key_pressed(KEY_F4)) { 
        current_instrument++; // int8 wraps naturally
        OPL_SetPatch(cur_channel, &gm_bank[current_instrument]);
        update_dashboard();
    }
    if (key_pressed(KEY_F5)) { // Use F5 to "Pick" the instrument under the cursor
        PatternCell cell;
        read_cell(cur_pattern, cur_row, cur_channel, &cell);
        if (cell.note != 0) {
            current_instrument = cell.inst;
            OPL_SetPatch(cur_channel, &gm_bank[current_instrument]);
            update_dashboard();
        }
    }
}

void handle_navigation() {
    uint8_t move_row = 0;
    int8_t move_chan = 0;
    uint8_t active_scancode = 0;

    // Detect which key is being held
    if (key(KEY_DOWN))       active_scancode = KEY_DOWN;
    else if (key(KEY_UP))    active_scancode = KEY_UP;
    else if (key(KEY_LEFT))  active_scancode = KEY_LEFT;
    else if (key(KEY_RIGHT)) active_scancode = KEY_RIGHT;

    if (active_scancode != 0) {
        if (active_scancode != last_scancode) {
            // First press: move immediately
            repeat_timer = 0;
            if (active_scancode == KEY_DOWN)  move_row = 1;
            if (active_scancode == KEY_UP)    move_row = 2; // Signal for 'Up'
            if (active_scancode == KEY_LEFT)  move_chan = -1;
            if (active_scancode == KEY_RIGHT) move_chan = 1;
        } else {
            // Holding: wait for repeat delay
            repeat_timer++;
            if (repeat_timer >= KEY_REPEAT_DELAY) {
                if ((repeat_timer - KEY_REPEAT_DELAY) % KEY_REPEAT_RATE == 0) {
                    if (active_scancode == KEY_DOWN)  move_row = 1;
                    if (active_scancode == KEY_UP)    move_row = 2;
                    if (active_scancode == KEY_LEFT)  move_chan = -1;
                    if (active_scancode == KEY_RIGHT) move_chan = 1;
                }
            }
        }
    }
    last_scancode = active_scancode;

    // Track state to determine if we need a UI update
    uint8_t old_row = cur_row;
    uint8_t old_chan = cur_channel;

    // Apply Row Movement (Capped at 0-31 for our 1F bottom alignment)
    if (move_row == 1 && cur_row < 31) cur_row++; 
    if (move_row == 2 && cur_row > 0)  cur_row--;

    // Apply Channel Movement (Capped at 0-8)
    if (move_chan == -1 && cur_channel > 0) cur_channel--;
    if (move_chan == 1  && cur_channel < 8) cur_channel++;

    // Optional: Toggle Edit mode with Space inside navigation
    if (key_pressed(KEY_SPACE)) {
        edit_mode = !edit_mode;
        // Force a visual refresh even if row/column didn't move
        update_cursor_visuals(cur_row, cur_row, cur_channel, cur_channel);
        update_dashboard();
    }

}

void sequencer_step(void) {
    if (!seq.is_playing) return;

    seq.tick_counter++;

    if (seq.tick_counter >= seq.ticks_per_row) {
        seq.tick_counter = 0;

        uint8_t old_row = cur_row;
        // Advance row logic...
        if (cur_row < 31) cur_row++; else cur_row = 0;

        for (uint8_t ch = 0; ch < 9; ch++) {
            PatternCell cell;
            read_cell(cur_pattern, cur_row, ch, &cell);
            
            if (cell.note != 0) {
                // 1. Always stop the previous sound on this channel first
                OPL_NoteOff(ch); 
                
                if (cell.note != 255) {
                    // 2. Load the new patch
                    OPL_SetPatch(ch, &gm_bank[cell.inst]);
                    
                    // 3. Trigger the new note
                    // The FPGA FIFO handles these back-to-back writes perfectly
                    OPL_NoteOn(ch, cell.note);
                }
            }
        }

        update_cursor_visuals(old_row, cur_row, cur_channel, cur_channel);
    }
}

void handle_transport_controls() {
    // F5: Play / Pause
    if (key_pressed(KEY_F6)) {
        seq.is_playing = !seq.is_playing;
        seq.tick_counter = seq.ticks_per_row; // Trigger first row immediately
        update_dashboard(); // Show Play/Pause status
    }

    // F6: Stop & Reset
    if (key_pressed(KEY_F7)) {
        seq.is_playing = false;
        
        // Silence all channels
        for (uint8_t i = 0; i < 9; i++) OPL_NoteOff(i);
        
        // Reset to beginning
        uint8_t old = cur_row;
        cur_row = 0;
        seq.tick_counter = 0;
        
        update_cursor_visuals(old, 0, cur_channel, cur_channel);
        update_dashboard();
    }
}

void handle_editing(void) {
    if (!edit_mode) return;

    // Check for Backspace or Delete key
    if (key_pressed(KEY_BACKSPACE) || key_pressed(KEY_DELETE)) {
        
        // 1. Create an empty cell (all zeros)
        PatternCell empty = {0, 0, 0, 0};
        
        // 2. Write to XRAM at current cursor position
        write_cell(cur_pattern, cur_row, cur_channel, &empty);
        
        // 3. Redraw the row to reflect the empty cell
        render_row(cur_row);
        
        // 4. (Standard Tracker Behavior) Auto-advance to the next row
        if (cur_row < 31) {
            uint8_t old_row = cur_row;
            cur_row++;
            update_cursor_visuals(old_row, cur_row, cur_channel, cur_channel);
        }
        
        printf("Cell Cleared at Row %d\n", cur_row - 1);
    }

    if (key_pressed(KEY_GRAVE)) {
        PatternCell off = {255, current_instrument, 0, 0};
        write_cell(cur_pattern, cur_row, cur_channel, &off);
        render_row(cur_row);
        if (cur_row < 31) cur_row++; 
    }

}