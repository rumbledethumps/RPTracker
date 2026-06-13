#include "midi.h"
#include <rp6502.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

// MIDI note entry from a USB MIDI keyboard on MIDI0:.
// The device delivers SMF-style events: a variable length delta time
// (ignored here) before each raw MIDI message.

#define MIDI_DEBUG 0

#define MIDI_DEVICE "MIDI0:"
#define MIDI_RETRY_FRAMES 60 // retry open once per second at 60 fps

uint8_t midi_note = 0;
uint8_t midi_vel = 0;
bool midi_fresh = false;
static bool midi_off_pending = false;

static int midi_fd = -1;
static uint8_t midi_retry = MIDI_RETRY_FRAMES;

// Stream parser state
enum {
    ST_DELTA,
    ST_STATUS,
    ST_DATA,
    ST_SYSEX,
};
static uint8_t midi_state = ST_DELTA;
static uint8_t midi_msg[3];
static uint8_t midi_msg_len;
static uint8_t midi_msg_need;

/**
 * Data byte count for a status byte
 */
static uint8_t midi_data_len(uint8_t status)
{
    switch (status & 0xF0) {
        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
            return 2;
        case 0xC0:
        case 0xD0:
            return 1;
        case 0xF0:
            if (status == 0xF2) return 2;
            if (status == 0xF1 || status == 0xF3) return 1;
            return 0;
    }
    return 0;
}

/**
 * Track the held note from note on/off messages
 */
static void midi_handle_message(void)
{
    uint8_t kind = midi_msg[0] & 0xF0;
    if (kind == 0x90 && midi_msg[2]) {
        if (midi_msg[1]) {
#if MIDI_DEBUG
            printf("[on %02X %02X]", midi_msg[1], midi_msg[2]);
#endif
            midi_note = midi_msg[1];
            midi_vel = midi_msg[2];
            midi_fresh = true;
            midi_off_pending = false;
        }
    } else if (kind == 0x80 || kind == 0x90) {
#if MIDI_DEBUG
        printf("[off %02X]", midi_msg[1]);
#endif
        if (midi_msg[1] == midi_note) {
            // A note struck and released within one frame still
            // sounds and records, the release applies next frame
            if (midi_fresh)
                midi_off_pending = true;
            else
                midi_note = 0;
        }
    }
}

/**
 * Parse one byte of the delta-timed event stream
 */
static void midi_parse_byte(uint8_t b)
{
    switch (midi_state) {
        case ST_DELTA:
            // Delta time ends at the first byte with the high bit clear
            if (!(b & 0x80))
                midi_state = ST_STATUS;
            break;
        case ST_STATUS:
            if (b >= 0xF8)
                midi_state = ST_DELTA; // real-time, one byte
            else if (b == 0xF0)
                midi_state = ST_SYSEX;
            else if (b >= 0x80) {
                midi_msg[0] = b;
                midi_msg_len = 1;
                midi_msg_need = midi_data_len(b);
                midi_state = midi_msg_need ? ST_DATA : ST_DELTA;
            }
            break;
        case ST_DATA:
            midi_msg[midi_msg_len++] = b;
            if (!--midi_msg_need) {
                midi_handle_message();
                midi_state = ST_DELTA;
            }
            break;
        case ST_SYSEX:
            if (b == 0xF7)
                midi_state = ST_DELTA;
            break;
    }
}

/**
 * Poll MIDI input: retry open every second, close on input fail
 */
void midi_task(void)
{
    uint8_t buf[32];
    int n;

    midi_fresh = false;
    if (midi_off_pending) {
        midi_off_pending = false;
        midi_note = 0;
    }

    if (midi_fd < 0) {
        if (++midi_retry < MIDI_RETRY_FRAMES)
            return;
        midi_retry = 0;
        midi_fd = open(MIDI_DEVICE, O_RDONLY);
        if (midi_fd < 0)
            return;
        midi_state = ST_DELTA;
#if MIDI_DEBUG
        printf("[midi open]\n");
#endif
    }

    n = read_xstack(buf, sizeof(buf), midi_fd);
    if (n < 0) {
#if MIDI_DEBUG
        printf("[midi err]\n");
#endif
        close(midi_fd);
        midi_fd = -1;
        midi_retry = 0;
        midi_note = 0;
        midi_off_pending = false;
        return;
    }
#if MIDI_DEBUG
    for (int i = 0; i < n; i++)
        printf(" %02X", buf[i]);
#endif
    for (int i = 0; i < n; i++)
        midi_parse_byte(buf[i]);
}
