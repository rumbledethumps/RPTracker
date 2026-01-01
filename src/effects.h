#ifndef EFFECTS_C
#define EFFECTS_C

typedef struct {
    uint8_t base_note;
    uint8_t inst;
    uint8_t vol;
    uint8_t style;
    uint8_t depth;
    uint8_t speed_idx;   // The T nibble (0-F)
    uint8_t target_ticks; // Value from LUT
    uint8_t phase_timer;  // Accumulator
    uint8_t step_index;  
    bool    active;
} ArpState;

extern ArpState ch_arp[9];

extern void process_arp_logic(uint8_t ch);
extern int16_t get_arp_offset(uint8_t style, uint8_t depth, uint8_t index);
extern const uint8_t arp_tick_lut[16];

#endif // EFFECTS_C