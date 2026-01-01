# RPTracker (v0.2)
### A Native 6502 Music Tracker for the RP6502 (Picocomputer)

**RPTracker** is a high-performance music composition tool designed for the [RP6502 Picocomputer](https://github.com/picocomputer). It features a 640x480 VGA interface and targets both the **Native RIA OPL2** emulation and the **FPGA OPL2** sound card. It provides a classic "tracker" workflow with modern high-resolution visual feedback.


![Screen Shot](images/Tracker_screenshot.png)

---

## 🎹 Keyboard Control Reference

### 1. Musical Keyboard (Piano Layout)
RPTracker uses a standard "FastTracker II" style layout mapping:
*   **Lower Octave (C-3 to B-3):** `Z S X D C V G B H N J M`
*   **Upper Octave (C-4 to B-4):** `Q 2 W 3 E R 5 T 6 Y 7 U`

### 2. Global "Brush" Controls
These keys adjust the settings used when recording **new** notes.
*   **F1 / F2**: Decrease / Increase global keyboard **Octave**.
*   **F3 / F4**: Previous / Next **Instrument** (Wraps 00-FF).
*   **[ / ]**: Decrease / Increase global **Volume** (Range 00-3F).
*   **F5**: **Instrument Pick.** Samples the Note, Instrument, and Volume from the grid into your current Brush.

### 3. Navigation & Transport
*   **Arrow Keys**: Navigate the 9-channel pattern grid.
*   **F6**: **Play / Pause.** In Song Mode, starts from the current Sequence slot. In Pattern Mode, loops the current pattern.
*   **F7 / ESC**: **Stop & Panic.** Stops playback, resets the cursor to Row 00, and silences all hardware voices immediately.
*   **F8**: **Toggle Playback Mode.** Switch between **PATTERN** (loops current patterns) and **SONG** (follows the Sequence Order List).

### 4. Editing & Grid Commands
*   **Spacebar**: Toggle **Edit Mode**.
    *   *Blue Cursor:* Navigation only. Piano keys play sounds for preview.
    *   *Red Cursor:* Record Mode. Piano keys enter data and auto-advance.
*   **Backspace / Delete**: Clear the current cell.
*   **Tilde ( ` )**: Insert **Note Off** (`===`).
*   **- / =**: **Transpose** the note in the current cell by **1 semitone**.
*   **SHIFT + - / =**: **Transpose** the current cell by **12 semitones (1 Octave)**.
*   **SHIFT + F3 / F4**: Change the **Instrument** of the current cell only.
*   **SHIFT + [ / ]**: Adjust the **Volume** of the current cell only.

### 5. Pattern & Sequence Management
*   **F9 / F10**: Jump to Previous / Next **Pattern ID**.
*   **F11 / F12**: Jump to Previous / Next **Sequence Slot** (Playlist position).
*   **SHIFT + F11 / F12**: Change the **Pattern ID** assigned to the current Sequence Slot.
*   **ALT + F11 / F12**: Decrease / Increase total **Song Length**.

### 6. Clipboard & Files
*   **Ctrl + C**: **Copy** the current 32-row pattern to the internal RAM clipboard.
*   **Ctrl + V**: **Paste** the clipboard into the current pattern (overwrites existing data).
*   **Ctrl + S**: **Save Song.** Opens a dialog to save the current song to the USB drive as an `.RPT` file.
*   **Ctrl + O**: **Load Song.** Opens a dialog to load an `.RPT` file from the USB drive.

### Effect Mode (Toggle with '/')
*   **SHIFT + [ / ]** : Change Command (Digit 1 - `X000`)
*   **[ / ]**       : Change Style   (Digit 2 - `0X00`)
*   **' (Apostrophe)**: Increase Params (Digits 3 & 4 - `00XX`)
*   **; (Semicolon)** : Decrease Params (Digits 3 & 4 - `00XX`)
*   *Note: Hold SHIFT with ; or ' to jump by 0x10 for faster parameter scrolling.*

### 🎹 Effect Command 1: Advanced Arpeggio (1SDT)
RPTracker uses a 16-bit effect system (4 hex digits) when in Effect View (`/`).
The Arpeggio engine retriggers the note on every cycle step to ensure a crisp, chiptune attack.

**Format: `1 S D T`**

*   **1**: Command ID (Arpeggio).
*   **S (Style)**: The movement pattern:
    *   `0`: **Up** (Root -> +Depth)
    *   `1`: **Down** (+Depth -> Root)
    *   `2`: **Major 3rd** (Root -> +4 -> +7) - *Ignores D*
    *   `3`: **Minor 3rd** (Root -> +3 -> +7) - *Ignores D*
    *   `4`: **Climb** (Root -> +Depth -> +Depth*2)
    *   `5`: **Octave** (Root -> +12 -> +24) - *Ignores D*
    *   `6`: **Random** (Random offset using Depth)
*   **D (Depth)**: The interval in semitones (0-F).
*   **T (Timing)**: How fast the notes cycle (mapped to a Musical LUT):
    *   `0-2`: High-speed "Buzz" (1-3 VSync frames)
    *   `3`: **1 Step** (6 ticks)
    *   `7`: **2 Steps** (12 ticks)
    *   `B`: **1 Beat** (24 ticks)
    *   `F`: **2 Bars** (16 steps / 96 ticks)

**Usage:**
- `10C3`: Standard octave flip (Up, 12 semitones, every 1 row step).
- `1471`: Fast 3-step climb (0 -> 7 -> 14 semitones) every 2 frames.
- `0000`: Stop all effects on the channel.

---

## 🖥 User Interface Guide

### The Dashboard (Top)
The top 27 rows provide a real-time view of the synthesizer and sequencer state:
*   **Status Bar:** Displays current Mode, Octave, Instrument name, Volume, and Sequencer status.
*   **Sequence Row:** A horizontal view of your song structure (e.g., `00 00 01 02`). The active slot is highlighted in **Yellow**.
*   **Operator Panels:** Shows the 11 raw OPL2 registers for the currently selected instrument (Modulator and Carrier).
*   **Channel Meters:** Visual bars that react to note volume and decay over time.
*   **System Panel:** Displays active hardware (Native OPL2 vs FPGA) and CPU speed.

### The Grid (Bottom)
The pattern grid starts at **Row 28**.
*   **Dark Grey Bars:** Highlights every 4th row (0, 4, 8, etc.) to indicate the musical beat.
*   **Syntax Highlighting:**
    *   **White:** Musical Notes.
    *   **Muted Purple:** Instrument IDs.
    *   **Sage Green:** Volume Levels.
    *   **Cyan:** Dividers and Row Numbers.

---
*Created by Jason Rowe. Developed for the RP6502 Picocomputer Project.*
