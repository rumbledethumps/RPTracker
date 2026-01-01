#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // For rand()
#include "effects.h"
#include "player.h"
#include "opl.h"
#include "instruments.h"
#include "screen.h"

// State memory for all 9 channels
ArpState ch_arp[9];

// Maps T (0-F) to target ticks
const uint8_t arp_tick_lut[16] = {
    1, 2, 3, 6, 9, 12, 18, 24, 30, 36, 42, 48, 60, 72, 84, 96
};

void process_arp_logic(uint8_t ch) {
    if (!ch_arp[ch].active) return;

    // 1. Advance the timer
    ch_arp[ch].phase_timer++;

    // 2. Check if we reached the target duration
    if (ch_arp[ch].phase_timer < ch_arp[ch].target_ticks) return;

    // 3. Reset timer and flip the note
    ch_arp[ch].phase_timer = 0;
    ch_arp[ch].step_toggle = !ch_arp[ch].step_toggle;

    uint8_t note_to_play = ch_arp[ch].base_note;
    if (ch_arp[ch].step_toggle) {
        note_to_play += ch_arp[ch].depth;
    }

    // 4. Retrigger OPL2
    OPL_NoteOff(ch);
    OPL_SetPatch(ch, &gm_bank[ch_arp[ch].inst]);
    OPL_SetVolume(ch, ch_arp[ch].vol << 1); 
    OPL_NoteOn(ch, note_to_play);
    ch_peaks[ch] = ch_arp[ch].vol; // Update peak meter
}
