import struct
import sys
import os

# Configuration
ROWS = 32
CHANS = 9
PATS_R1 = 16  # Old format had 16 patterns
PATS_R2 = 32  # New format has 32 patterns

def convert_rpt1_to_rpt2(input_file, output_file):
    if not os.path.exists(input_file):
        print(f"Error: {input_file} not found.")
        return

    with open(input_file, "rb") as f:
        # 1. Read Header
        header = f.read(4)
        if header != b"RPT1":
            print("Error: Not a valid RPT1 file.")
            return

        # 2. Read Metadata (Octave:1, Vol:1, SongLength:2)
        meta = f.read(4)
        octave, vol, song_len = struct.unpack("<BBH", meta)
        print(f"Converting: {input_file}")
        print(f"Metadata: Octave={octave}, Vol={vol}, Seq Length={song_len}")

        # 3. Read 16 Patterns (4-byte cells in RPT1)
        # RPT1: 16 patterns × 32 rows × 9 channels × 4 bytes = 18,432 bytes (0x4800)
        # RPT2: 32 patterns × 32 rows × 9 channels × 5 bytes = 46,080 bytes (0xB400)
        print(f"Reading {PATS_R1} patterns (4 bytes/cell)...")
        pats_data = []
        for p in range(PATS_R1):
            rows = []
            for r in range(ROWS):
                cells = []
                for c in range(CHANS):
                    cell_raw = f.read(4)
                    if len(cell_raw) < 4:
                        print(f"Warning: Incomplete cell at pattern {p}, row {r}, channel {c}")
                        cell_raw = cell_raw.ljust(4, b'\x00')
                    
                    # Unpack: Note, Inst, Vol, Old_Effect (1 byte)
                    n, i, v, e_old = struct.unpack("BBBB", cell_raw)
                    
                    # Convert to 5-byte cell: Note, Inst, Vol, Effect_Lo, Effect_Hi
                    # Map old 1-byte effect to low byte of new 16-bit effect
                    new_cell = bytearray([n, i, v, e_old, 0])
                    cells.append(new_cell)
                rows.append(cells)
            pats_data.append(rows)
        
        print(f"Read {len(pats_data)} patterns successfully")

        # 4. Read the 256-byte Order List (sequence data)
        # In RPT1 this was written after the patterns
        print("Reading sequence order list (256 bytes)...")
        order_list = f.read(256)
        if len(order_list) < 256:
            print(f"Warning: Order list only {len(order_list)} bytes, padding with zeros")
            order_list = order_list.ljust(256, b"\x00")
        print(f"Sequence data: {len(order_list)} bytes")

        print(f"Sequence data: {len(order_list)} bytes")

    # WRITE RPT2 FORMAT
    print(f"\nWriting RPT2 format to: {output_file}")
    with open(output_file, "wb") as f:
        # 1. Write Header (RPT2)
        f.write(b"RPT2")
        
        # 2. Write Metadata (same format)
        f.write(struct.pack("<BBH", octave, vol, song_len))
        print(f"Wrote metadata: Octave={octave}, Vol={vol}, Length={song_len}")

        # 3. Write the 16 original patterns (now 5 bytes per cell)
        # Each pattern: 32 rows × 9 channels × 5 bytes = 1,440 bytes
        pattern_bytes_written = 0
        for p in range(PATS_R1):
            for r in range(ROWS):
                for c in range(CHANS):
                    f.write(pats_data[p][r][c])
                    pattern_bytes_written += 5
        print(f"Wrote {PATS_R1} patterns: {pattern_bytes_written} bytes")

        # 4. Write 16 EMPTY patterns (to reach 32 total)
        # Each empty pattern: 1,440 bytes of zeros
        empty_pattern_size = ROWS * CHANS * 5  # 1,440 bytes
        empty_pattern = b"\x00" * empty_pattern_size
        empty_bytes_written = 0
        for _ in range(PATS_R2 - PATS_R1):
            f.write(empty_pattern)
            empty_bytes_written += empty_pattern_size
        print(f"Wrote {PATS_R2 - PATS_R1} empty patterns: {empty_bytes_written} bytes")
        
        total_pattern_bytes = pattern_bytes_written + empty_bytes_written
        print(f"Total pattern data: {total_pattern_bytes} bytes (0x{total_pattern_bytes:X})")
        
        # 5. Write Sequence Order List (always 256 bytes)
        # RPT2 expects this at offset 0xB400 in the XRAM dump
        # which is right after the 32 patterns
        f.write(order_list)
        print(f"Wrote sequence order list: 256 bytes")

    output_size = os.path.getsize(output_file)
    print(f"\n✓ Success! Converted to RPT2 format")
    print(f"  Input:  {input_file}")
    print(f"  Output: {output_file}")
    print(f"  Size:   {output_size} bytes (0x{output_size:X})")
    print(f"  Expected: 8 (header+meta) + 46,080 (patterns) + 256 (sequence) = 46,344 bytes")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert_rpt12.py input.rpt [output.rpt]")
        print("Converts RPT1 format (16 patterns, 4-byte cells) to RPT2 format (32 patterns, 5-byte cells)")
    else:
        infile = sys.argv[1]
        outfile = sys.argv[2] if len(sys.argv) > 2 else "NEW_" + infile
        convert_rpt1_to_rpt2(infile, outfile)