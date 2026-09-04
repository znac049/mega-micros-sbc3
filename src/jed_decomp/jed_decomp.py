# 22V10 JEDEC File Disassembler / Equation Extractor
# This script parses a standard 22V10 .JED file and extracts its product terms.

import sys
import re

def parse_jed(filename):
    try:
        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading file: {e}")
        return

    # Extract Fuse Count (QF)
    qf_match = re.search(r'QF(\d+)', content)
    num_fuses = int(qf_match.group(1)) if qf_match else 5828
    print(r"/*")
    print(f" * JEDEC File: {filename}")
    print(f" * Target Device: PALCE22V10 / GAL22V10")
    print(f" * Total Fuses: {num_fuses}")
    print(r" */")

    # Extract Fuse Data Matrix
    # Standard JEDEC fuse maps start with Lxxxx where xxxx is the fuse index
    fuse_map = {}
    lines = content.split('\n')
    
    # Simple parser for L lines
    for line in lines:
        line = line.strip().rstrip('*')
        if line.startswith('L'):
            match = re.match(r'L(\d+)\s+([01 ]+)', line)
            if match:
                start_addr = int(match.group(1))
                bits = match.group(2).replace(' ', '')
                for idx, bit in enumerate(bits):
                    fuse_map[start_addr + idx] = int(bit)

    # 22V10 Architecture Configuration
    # Input mapping to fuse columns (Total 44 columns: 22 inputs * 2 polarities)
    # 12 Dedicated Inputs + 10 Feedback paths from Macrocells
    # Product terms per macrocell vary: 8, 10, 12, 14, 16, 16, 14, 12, 10, 8
    p_terms_per_macrocell = [8, 10, 12, 14, 16, 16, 14, 12, 10, 8]
    macrocell_pins = [23, 22, 21, 20, 19, 18, 17, 16, 15, 14]
    
    # 22V10 Column Mapping Guide (Simplified)
    # Pins 1-11, 13 (Inputs/Clock) and Feedbacks map to columns 0-43
    input_names = {
        0: "Pin1",  1: "/Pin1",
        2: "Pin2",  3: "/Pin2",
        4: "Pin3",  5: "/Pin3",
        6: "Pin4",  7: "/Pin4",
        8: "Pin5",  9: "/Pin5",
        10: "Pin6", 11: "/Pin6",
        12: "Pin7", 13: "/Pin7",
        14: "Pin8", 15: "/Pin8",
        16: "Pin9", 17: "/Pin9",
        18: "Pin10", 19: "/Pin10",
        20: "Pin11", 21: "/Pin11",
        22: "Pin13", 23: "/Pin13",
        # Feedback columns continue from 24 to 43 (Pins 14-23 macrocells)
    }
    
    # Map remaining feedback paths dynamically for display
    for i in range(10):
        input_names[24 + i*2] = f"Pin{macrocell_pins[i]}"
        input_names[25 + i*2] = f"/Pin{macrocell_pins[i]}"

    print("// --- RECONSTRUCTED BOOLEAN LOGIC EQUATIONS ---")
    
    current_fuse = 0
    # Process each of the 10 macrocells
    for mc_idx, pins in enumerate(macrocell_pins):
        terms = p_terms_per_macrocell[mc_idx]
        valid_equations = []
        
        # Every macrocell has an Output Enable (OE) product term first
        oe_term = []
        for col in range(44):
            bit = fuse_map.get(current_fuse + col, 1)
            if bit == 0:
                oe_term.append(input_names.get(col, f"C{col}"))
        current_fuse += 44
        
        # Process the logic product terms
        for t in range(terms):
            p_term_literals = []
            for col in range(44):
                bit = fuse_map.get(current_fuse + col, 1)
                if bit == 0:  # 0 means programmed/connected in JEDEC logic array
                    p_term_literals.append(input_names.get(col, f"C{col}"))
            
            current_fuse += 44
            
            # If all fuses in a row are 1, it means disconnected (ignored)
            # If all are 0, it forces the product term to 0
            if len(p_term_literals) == 44:
                continue # Disconnected row
            elif len(p_term_literals) > 0:
                valid_equations.append(" & ".join(p_term_literals))
        
        # Print output equation if it contains active connections
        if valid_equations:
            eqn_str = " #\n".join(valid_equations)
            print(f"\nPin {pins} Equations:")
            if oe_term:
                print(f"  Pin{pins}.OE = " + " & ".join(oe_term) + ";")
            print(f"  Pin{pins} = {eqn_str};")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python decomp_22v10.py <your_file.jed>")
    else:
        parse_jed(sys.argv[1])
