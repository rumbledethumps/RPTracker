#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool is_playing;
    uint8_t ticks_per_row; // Standard tracker speed (default 6)
    uint8_t tick_counter;  // Counter to track sub-row timing
    uint8_t bpm;           // Current BPM (logic uses 60Hz / ticks)
} SequencerState;

extern SequencerState seq;


// Initialize player state
void player_init(void);

// Process keyboard-to-OPL logic (call this once per frame in main loop)
void player_tick(void);

// Global settings
extern uint8_t current_octave;
extern uint8_t current_instrument;
extern uint8_t player_channel;

extern void handle_navigation(void);
extern void handle_transport_controls(void);
extern void sequencer_step(void);
extern void handle_editing(void);

#endif