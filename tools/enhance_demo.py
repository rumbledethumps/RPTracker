#!/usr/bin/env python3
"""
RPTracker Demo Enhancement Script
Takes an existing RPT2 file and adds effects, melodies, and expands the sequence
to showcase all tracker features.
"""

import struct
import sys
import os

# Configuration
ROWS = 32
CHANS = 9
PATTERNS = 32

class PatternCell:
    def __init__(self, note=0, inst=0, vol=0, effect=0x0000):
        self.note = note    # 0=empty, 255=note off, 12-119=MIDI notes
        self.inst = inst    # 0-255 instrument
        self.vol = vol      # 0-63 volume
        self.effect = effect  # 16-bit effect (0xXXXX)
    
    def to_bytes(self):
        """Convert to 5-byte format"""
        lo = self.effect & 0xFF
        hi = (self.effect >> 8) & 0xFF
        return bytes([self.note, self.inst, self.vol, lo, hi])
    
    @staticmethod
    def from_bytes(data):
        """Create from 5-byte format"""
        if len(data) < 5:
            # Pad with zeros if incomplete
            data = data + b'\x00' * (5 - len(data))
        n, i, v, lo, hi = data[:5]
        effect = (hi << 8) | lo
        return PatternCell(n, i, v, effect)

class RPT2File:
    def __init__(self):
        self.octave = 3
        self.volume = 63
        self.song_length = 1
        self.patterns = [[[[PatternCell() for _ in range(CHANS)] 
                          for _ in range(ROWS)] 
                         for _ in range(PATTERNS)]]
        self.sequence = [0] * 256
    
    def load(self, filename):
        """Load RPT2 file"""
        with open(filename, 'rb') as f:
            # Read header
            header = f.read(4)
            if header != b'RPT2':
                raise ValueError("Not a valid RPT2 file")
            
            # Read metadata
            meta = f.read(4)
            self.octave, self.volume, self.song_length = struct.unpack('<BBH', meta)
            
            print(f"Loaded: {filename}")
            print(f"  Octave: {self.octave}, Volume: {self.volume}, Length: {self.song_length}")
            
            # Read all 32 patterns
            self.patterns = []
            for p in range(PATTERNS):
                pattern = []
                for r in range(ROWS):
                    row = []
                    for c in range(CHANS):
                        cell_data = f.read(5)
                        if len(cell_data) < 5:
                            print(f"Warning: Incomplete cell at pattern {p}, row {r}, channel {c}")
                            cell_data = cell_data + b'\x00' * (5 - len(cell_data))
                        cell = PatternCell.from_bytes(cell_data)
                        row.append(cell)
                    pattern.append(row)
                self.patterns.append(pattern)
            
            # Read sequence
            seq_data = f.read(256)
            if len(seq_data) < 256:
                print(f"Warning: Sequence data only {len(seq_data)} bytes, padding...")
                seq_data = seq_data + b'\x00' * (256 - len(seq_data))
            self.sequence = list(seq_data)
            
            # Safety check for song_length
            if self.song_length > 256:
                print(f"Warning: Song length {self.song_length} exceeds max, capping at 64")
                self.song_length = 64
            
            seq_preview = [self.sequence[i] for i in range(min(self.song_length, len(self.sequence)))]
            print(f"  Patterns: {PATTERNS}, Sequence: {seq_preview}")
    
    def save(self, filename):
        """Save RPT2 file"""
        with open(filename, 'wb') as f:
            # Write header
            f.write(b'RPT2')
            
            # Write metadata
            f.write(struct.pack('<BBH', self.octave, self.volume, self.song_length))
            
            # Write all 32 patterns
            for p in range(PATTERNS):
                for r in range(ROWS):
                    for c in range(CHANS):
                        f.write(self.patterns[p][r][c].to_bytes())
            
            # Write sequence
            f.write(bytes(self.sequence))
        
        print(f"\nSaved: {filename}")
        print(f"  Song Length: {self.song_length}")
        print(f"  Sequence: {[self.sequence[i] for i in range(min(16, self.song_length))]}")

# Note helper functions
def note(name, octave=4):
    """Convert note name to MIDI number (C-4 = 60)"""
    notes = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 
             'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}
    return (octave + 1) * 12 + notes[name.upper()]

def enhance_demo(input_file, output_file):
    """Enhance demo with effects and new musical content"""
    
    rpt = RPT2File()
    rpt.load(input_file)
    
    print("\n=== ENHANCING DEMO ===\n")
    
    # Pattern 1: Add arpeggios and volume effects to existing content
    print("Pattern 1: Adding arpeggios and volume slides...")
    pat = rpt.patterns[1]
    for r in range(0, 32, 8):
        if pat[r][0].note > 0 and pat[r][0].note < 255:
            pat[r][0].effect = 0x12C3  # Major chord arpeggio
        if pat[r][1].note > 0 and pat[r][1].note < 255:
            pat[r+2][1].effect = 0x3130  # Volume slide down
    
    # Pattern 2: Create bass line with portamento on CH6
    print("Pattern 2: Creating bass line with portamento on CH6...")
    pat = rpt.patterns[2]
    bass_notes = [note('C', 2), note('G', 2), note('A#', 2), note('F', 2)]
    for i, n in enumerate(bass_notes):
        r = i * 8
        pat[r][6] = PatternCell(n, 32, 63, 0x0000)  # Bass instrument
        if i < len(bass_notes) - 1:
            pat[r+4][6].effect = 0x2046  # Portamento up

    # Pattern 3: Add lead melody with vibrato on CH7
    print("Pattern 3: Creating lead melody with vibrato on CH7...")
    pat = rpt.patterns[3]
    melody = [
        (note('C', 5), 0), (note('E', 5), 4), (note('G', 5), 8), 
        (note('E', 5), 12), (note('D', 5), 16), (note('F', 5), 20),
        (note('A', 5), 24), (note('G', 5), 28)
    ]
    for n, r in melody:
        pat[r][7] = PatternCell(n, 72, 50, 0x4420)  # Synth lead with vibrato
    
    # Pattern 4: Rhythmic stabs with note cut on CH8
    print("Pattern 4: Creating rhythmic stabs with note cuts on CH8...")
    pat = rpt.patterns[4]
    stab_rows = [0, 4, 8, 10, 16, 20, 24, 28]
    for r in stab_rows:
        pat[r][8] = PatternCell(note('C', 4), 80, 63, 0x5003)  # Pad with note cut
    
    # Pattern 5: Arpeggio showcase - different styles
    print("Pattern 5: Arpeggio showcase...")
    pat = rpt.patterns[5]
    arp_styles = [
        (0x10C3, 0),   # UP
        (0x11C3, 4),   # DOWN
        (0x12C3, 8),   # MAJOR
        (0x13C3, 12),  # MINOR
        (0x14C3, 16),  # MAJ7
        (0x15C3, 20),  # MIN7
        (0x1AC3, 24),  # POWER
        (0x1DC3, 28),  # OCTAVE
    ]
    for effect, r in arp_styles:
        pat[r][6] = PatternCell(note('C', 3), 0, 63, effect)
    
    # Pattern 6: Tremolo and vibrato combo
    print("Pattern 6: Tremolo and vibrato effects...")
    pat = rpt.patterns[6]
    pat[0][7] = PatternCell(note('C', 5), 9, 63, 0x4420)  # Vibrato
    pat[2][7].effect = 0x8440  # Add tremolo
    pat[8][7] = PatternCell(note('G', 5), 9, 63, 0x4420)
    pat[10][7].effect = 0x8440
    pat[16][7] = PatternCell(note('E', 5), 9, 63, 0x4420)
    pat[18][7].effect = 0x8440
    pat[24][7] = PatternCell(note('C', 5), 9, 63, 0x4420)
    pat[26][7].effect = 0x8440
    
    # Pattern 7: Retrigger showcase
    print("Pattern 7: Retrigger effects...")
    pat = rpt.patterns[7]
    pat[0][8] = PatternCell(note('C', 4), 96, 63, 0x7003)  # Standard retrigger
    pat[8][8] = PatternCell(note('E', 4), 96, 63, 0x7001)  # Fast retrigger
    pat[16][8] = PatternCell(note('G', 4), 96, 63, 0x7006)  # Slow retrigger
    pat[24][8] = PatternCell(note('C', 5), 96, 63, 0x7002)  # Medium retrigger
    
    # Pattern 8: Chord progression with multiple effects
    print("Pattern 8: Complex chord progression...")
    pat = rpt.patterns[8]
    # Channel 6: Bass with portamento
    bass_prog = [note('C', 2), note('F', 2), note('G', 2), note('A#', 2)]
    for i, n in enumerate(bass_prog):
        r = i * 8
        pat[r][6] = PatternCell(n, 32, 63, 0x2046 if i < 3 else 0x0000)
    
    # Channel 7: Lead with vibrato
    for r in [0, 8, 16, 24]:
        pat[r][7] = PatternCell(note('C', 5) + (r // 8) * 2, 72, 50, 0x4420)
    
    # Channel 8: Pad with tremolo
    for r in [0, 16]:
        pat[r][8] = PatternCell(note('C', 4), 80, 40, 0x8440)
    
    # Pattern 9: Breakdown with note delays
    print("Pattern 9: Breakdown with note delays...")
    pat = rpt.patterns[9]
    # Create a swing feel with note delays
    for i in range(4):
        r = i * 8
        pat[r][6] = PatternCell(note('C', 2), 32, 63, 0x0000)
        pat[r][7].effect = 0x6030  # Delayed note
        pat[r+4][6] = PatternCell(note('G', 2), 32, 50, 0x0000)
    
    # Pattern 10: Build-up with volume slides
    print("Pattern 10: Build-up with volume slides...")
    pat = rpt.patterns[10]
    for c in range(6, 9):
        pat[0][c] = PatternCell(note('C', 3 + c - 6), 80, 10, 0x3020)  # Fade in
        pat[16][c] = PatternCell(note('G', 3 + c - 6), 80, 10, 0x3020)
    
    # Expand sequence to showcase all patterns
    print("\nExpanding sequence...")
    new_sequence = [
        0,   # Original pattern (intro)
        1,   # Arpeggios added
        2,   # Bass line
        3,   # Lead melody
        2,   # Bass line repeat
        3,   # Lead melody repeat
        4,   # Rhythmic stabs
        5,   # Arpeggio showcase
        6,   # Tremolo/vibrato
        7,   # Retrigger showcase
        8,   # Complex progression
        9,   # Breakdown
        10,  # Build-up
        1,   # Return to arp section
        8,   # Final chorus
        0,   # Outro
    ]
    
    rpt.song_length = len(new_sequence)
    for i, pat_id in enumerate(new_sequence):
        rpt.sequence[i] = pat_id
    
    # Save enhanced version
    rpt.save(output_file)
    
    print("\n=== ENHANCEMENT COMPLETE ===")
    print("\nNew patterns created:")
    print("  Pattern 1: Arpeggios and volume effects")
    print("  Pattern 2: Bass line with portamento (CH6)")
    print("  Pattern 3: Lead melody with vibrato (CH7)")
    print("  Pattern 4: Rhythmic stabs with note cuts (CH8)")
    print("  Pattern 5: Arpeggio style showcase")
    print("  Pattern 6: Tremolo and vibrato combo")
    print("  Pattern 7: Retrigger effects")
    print("  Pattern 8: Complex chord progression")
    print("  Pattern 9: Breakdown with note delays")
    print("  Pattern 10: Build-up with volume slides")
    print(f"\nSequence expanded to {rpt.song_length} steps")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python enhance_demo.py DEMOTEST.RPT [output.rpt]")
        print("\nEnhances an RPT2 file with effects and new musical content")
        print("to showcase all tracker features.")
        sys.exit(1)
    
    infile = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) > 2 else "DEMO_ENHANCED.RPT"
    
    if not os.path.exists(infile):
        print(f"Error: {infile} not found")
        sys.exit(1)
    
    enhance_demo(infile, outfile)
