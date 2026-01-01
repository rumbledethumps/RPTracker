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
        case 2: // MAJOR
            if (index % 3 == 1) return 4;
            if (index % 3 == 2) return 7;
            return 0;
        case 3: // MINOR
            if (index % 3 == 1) return 3;
            if (index % 3 == 2) return 7;
            return 0;
        case 4: return (index % 3) * depth;          // CLIMB
        case 5: return (index % 3) * 12;             // OCTAVE
    }
    return 0;
}

void process_arp_logic(uint8_t ch) {
    if (!ch_arp[ch].active) return;

    // --- TICK 0 GUARD ---
    // The sequencer handles the strike on Tick 0. 
    // We only process mid-row re-triggers here.
    if (seq.tick_counter == 0) return;

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