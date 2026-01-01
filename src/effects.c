#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "effects.h"
#include "player.h"
#include "opl.h"
#include "instruments.h"
#include "screen.h"

// State memory for all 9 channels
ArpState ch_arp[9];

const uint8_t arp_tick_lut[16] = {
    1, 2, 3, 6, 9, 12, 18, 24, 30, 36, 42, 48, 60, 72, 84, 96
};

// Add this to effects.c
int16_t get_arp_offset(uint8_t style, uint8_t depth, uint8_t index) {
    switch (style) {
        case 0: return (index % 2 == 1) ? depth : 0; // UP
        case 1: return (index % 2 == 0) ? depth : 0; // DOWN
        case 2: // MAJOR (4-note: root, 3rd, 5th, octave)
            if (index % 4 == 1) return 4;
            if (index % 4 == 2) return 7;
            if (index % 4 == 3) return 12;
            return 0;
        case 3: // MINOR (4-note: root, minor 3rd, 5th, octave)
            if (index % 4 == 1) return 3;
            if (index % 4 == 2) return 7;
            if (index % 4 == 3) return 12;
            return 0;
        case 4: // MAJ7 (root, 3rd, 5th, maj7th)
            if (index % 4 == 1) return 4;
            if (index % 4 == 2) return 7;
            if (index % 4 == 3) return 11;
            return 0;
        case 5: // MIN7 (root, minor 3rd, 5th, minor 7th)
            if (index % 4 == 1) return 3;
            if (index % 4 == 2) return 7;
            if (index % 4 == 3) return 10;
            return 0;
        case 6: // SUS4 (root, 4th, 5th, octave)
            if (index % 4 == 1) return 5;
            if (index % 4 == 2) return 7;
            if (index % 4 == 3) return 12;
            return 0;
        case 7: // SUS2 (root, 2nd, 5th, octave)
            if (index % 4 == 1) return 2;
            if (index % 4 == 2) return 7;
            if (index % 4 == 3) return 12;
            return 0;
        case 8: // DIM (root, minor 3rd, dim 5th, dim 7th)
            if (index % 4 == 1) return 3;
            if (index % 4 == 2) return 6;
            if (index % 4 == 3) return 9;
            return 0;
        case 9: // AUG (root, maj 3rd, aug 5th, octave)
            if (index % 4 == 1) return 4;
            if (index % 4 == 2) return 8;
            if (index % 4 == 3) return 12;
            return 0;
        case 10: // POWER (root, 5th, octave, octave)
            if (index % 4 == 1) return 7;
            if (index % 4 == 2) return 12;
            if (index % 4 == 3) return 12;
            return 0;
        case 11: // UPDOWN (root, up, up, root) - bounce pattern
            if (index % 4 == 1) return depth;
            if (index % 4 == 2) return depth;
            if (index % 4 == 3) return 0;
            return 0;
        case 12: // UP3 (0, depth, depth*2, depth*3) - climbing thirds
            return (index % 4) * depth;
        case 13: // OCTAVE (root, octave, root, octave)
            return (index % 2 == 1) ? 12 : 0;
        case 14: // FIFTH (root, 5th, root, 5th)
            return (index % 2 == 1) ? 7 : 0;
        case 15: // DOUBLE (depth value unused, repeats each note)
            return (index % 4 < 2) ? 0 : depth;
    }
    return 0;
}

void process_arp_logic(uint8_t ch) {
    if (!ch_arp[ch].active) return;

    // --- JUST TRIGGERED GUARD ---
    // If this arpeggio was just activated or a note was just struck,
    // skip processing this frame to avoid double-hit.
    if (ch_arp[ch].just_triggered) {
        ch_arp[ch].just_triggered = false;
        return;
    }

    ch_arp[ch].phase_timer++;

    if (ch_arp[ch].phase_timer < ch_arp[ch].target_ticks) return;

    ch_arp[ch].phase_timer = 0;
    ch_arp[ch].step_index++; 

    int16_t offset = get_arp_offset(ch_arp[ch].style, ch_arp[ch].depth, ch_arp[ch].step_index);

    OPL_NoteOff(ch);
    OPL_SetPatch(ch, &gm_bank[ch_arp[ch].inst]);
    OPL_SetVolume(ch, ch_arp[ch].vol << 1); 
    OPL_NoteOn(ch, ch_arp[ch].base_note + offset);
    ch_peaks[ch] = ch_arp[ch].vol; 
}