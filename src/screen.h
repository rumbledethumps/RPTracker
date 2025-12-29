#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>

// XRAM Pattern Base (from our map)
#define PATTERN_XRAM_BASE 0x0000

typedef struct {
    uint8_t note;       // MIDI Note (0=None, 255=Off)
    uint8_t inst;       // Instrument index
    uint8_t vol;        // Volume (0-63)
    uint8_t effect;     // Combined Effect/Param (for now)
} PatternCell;

extern bool edit_mode;
extern uint8_t cur_row;
extern uint8_t cur_pattern;
extern uint8_t cur_channel;

extern void write_cell(uint8_t pat, uint8_t row, uint8_t chan, PatternCell *cell);


#endif // SCREEN_H