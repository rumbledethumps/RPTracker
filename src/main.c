#include <rp6502.h>
#include <stdio.h>
#include "constants.h"
#include "display.h"
#include "input.h"
#include "instruments.h"
#include "opl.h"


unsigned text_message_addr;         // Text message address

static void init_graphics(void)
{
    // Initialize graphics here
    xregn(1, 0, 0, 1, 3); // 320x240 (4:3)

    text_message_addr = TEXT_CONFIG + sizeof(vga_mode1_config_t);
    unsigned text_storage_end = text_message_addr + MESSAGE_LENGTH * BYTES_PER_CHAR;


    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, x_wrap, 0);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, y_wrap, 0);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, x_pos_px, 0); //Bug: first char duplicated if not set to zero
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, y_pos_px, 0);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, width_chars, MESSAGE_WIDTH);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, height_chars, MESSAGE_HEIGHT);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, xram_data_ptr, text_message_addr);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, xram_palette_ptr, 0xFFFF);
    xram0_struct_set(TEXT_CONFIG, vga_mode1_config_t, xram_font_ptr, 0xFFFF);

    // 4 parameters: text mode, 8-bit, config, plane
    xregn(1, 0, 1, 4, 1, 3, TEXT_CONFIG, 2);

    // Clear message buffer to spaces
    for (int i = 0; i < MESSAGE_LENGTH; ++i) message[i] = ' ';

    // Now write the MESSAGE_LENGTH characters into text RAM (3 bytes per char)
    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;
    for (uint16_t i = 0; i < MESSAGE_LENGTH; i++) {
        RIA.rw0 = ' ';
        RIA.rw0 = HUD_COL_WHITE;
        RIA.rw0 = HUD_COL_BG;
    }


}

int main(void)
{

    // Enable OPL2 sound
    OPL_Config(1, OPL_ADDR);
    opl_init();

    // Enable keyboard input
    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    // Enable gamepad input
    xregn(0, 0, 2, 1, GAMEPAD_INPUT);

    init_graphics();

    uint8_t vsync_last = RIA.vsync;

    while (1) {
        // Wait for vertical sync
        while (RIA.vsync == vsync_last);
        vsync_last = RIA.vsync;

        // Main loop code here

    }
}
