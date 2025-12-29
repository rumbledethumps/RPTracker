#include "player.h"
#include "opl.h"
#include "input.h"
#include "usb_hid_keys.h"
#include <stdio.h>
#include "screen.h"

// Current State
uint8_t current_instrument = 0; // Instrument index (0 = Piano)
uint8_t current_octave_offset = 0; // Adjusts in jumps of 12
uint8_t active_midi_note = 0;      // Tracks the currently playing note

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
    uint8_t channel = 0;
    bool note_pressed_this_frame = false;
    uint8_t target_note = 0;

    // 1. Scan the piano keys
    // We iterate through the bitmask to find the first pressed piano key
    for (int k = 0; k < 256; k++) {
        if (key(k)) { // Using the bitmask check from your input logic
            int8_t semitone = get_semitone(k);
            if (semitone != -1) {
                // MIDI 60 is C-4.
                target_note = 60 + current_octave_offset + semitone;
                note_pressed_this_frame = true;
                break; // Found a note, stop looking (Monophonic)
            }
        }
    }

    // 2. Logic: Note On
    if (note_pressed_this_frame) {
        // If it's a new note, or we weren't playing anything
        if (target_note != active_midi_note) {
            if (active_midi_note != 0) OPL_NoteOff(channel); 
            
            OPL_NoteOn(channel, target_note);
            active_midi_note = target_note;
            printf("Playing MIDI: %d\n", target_note);

            if (edit_mode && note_pressed_this_frame) {
                PatternCell c;
                c.note = target_note;
                c.inst = current_instrument;
                c.vol = 63; // Max volume default
                c.effect = 0;
                
                write_cell(cur_pattern, cur_row, cur_channel, &c);
                
                // Auto-advance
                if (cur_row < 63) cur_row++;
                // Trigger a redraw of the grid here
            }

        }
    } 
    // 3. Logic: Note Off
    else {
        if (active_midi_note != 0) {
            OPL_NoteOff(channel);
            active_midi_note = 0;
            printf("Key Released\n");
        }
    }

    // 4. Octave Switching (Using F1/F2)
    // Note: Use key_pressed (edge detection) so it only moves 1 octave per tap
    if (key_pressed(KEY_F1)) {
        if (current_octave_offset >= 12) current_octave_offset -= 12;
        printf("Octave Down. Base: %d\n", 60 + current_octave_offset);
    }
    if (key_pressed(KEY_F2)) {
        if (current_octave_offset <= 48) current_octave_offset += 12;
        printf("Octave Up. Base: %d\n", 60 + current_octave_offset);
    }
}