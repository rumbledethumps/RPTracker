#ifndef EFFECTS_C
#define EFFECTS_C

typedef struct {
    uint8_t base_note;  // The original note from the grid
    uint8_t style;      // 0:Up, 1:Down, 2:Random
    uint8_t depth;      // Semitones between steps
    uint8_t speed;      // How many ticks per pitch change
    bool active;        // Is an arp currently running on this channel?
} ArpState;

extern ArpState ch_arp[9];

extern void process_arp_logic(uint8_t ch);

#endif // EFFECTS_C