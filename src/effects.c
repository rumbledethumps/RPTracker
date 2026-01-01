#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // For rand()
#include "effects.h"
#include "player.h"
#include "opl.h"

// State memory for all 9 channels
ArpState ch_arp[9];

void process_arp_logic(uint8_t ch) {
    if (!ch_arp[ch].active) return;

    // Determine which step of the arpeggio we are on
    // If speed is 1, change every tick. If speed 2, every 2nd tick, etc.
    uint8_t step = seq.tick_counter / (ch_arp[ch].speed + 1);
    
    int16_t note_offset = 0;

    switch (ch_arp[ch].style) {
        case 0: // UP
            note_offset = step * ch_arp[ch].depth;
            break;
        case 1: // DOWN
            note_offset = 0 - (step * ch_arp[ch].depth);
            break;
        case 2: // RANDOM (Uses a simple 8-bit LFSR or rand)
            note_offset = (uint8_t)(rand() % 3) * ch_arp[ch].depth;
            break;
    }

    // Update the OPL2 frequency without restarting the ADSR envelope
    // This requires your OPL_SetPitch function (NoteOn without bit 5 toggle)
    OPL_SetPitch(ch, ch_arp[ch].base_note + note_offset);
}
