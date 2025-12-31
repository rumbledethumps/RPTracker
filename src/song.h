#ifndef SONG_H
#define SONG_H

#define ORDER_LIST_XRAM 0xB000
#define MAX_ORDERS 256

extern uint8_t cur_order_idx;
extern uint16_t song_length;
extern bool is_song_mode;
extern bool is_dialog_active;
extern bool is_saving;
extern char dialog_buffer[21]; // 8.3 filename + null terminator
extern uint8_t dialog_pos;

extern void update_order_display();
extern uint8_t read_order_xram(uint8_t index);
extern void write_order_xram(uint8_t index, uint8_t pattern_id);
extern void handle_filename_input();
extern char scancode_to_ascii(uint8_t scancode);

// Picocomputer OS Call Registers (Device 0, Channel 0)
#define TRK_READ_XRAM  0x31
#define TRK_WRITE_XRAM 0x32

#endif // SONG_H