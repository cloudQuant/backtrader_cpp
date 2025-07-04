#!/usr/bin/env python3

import os
import re
import glob

def fix_lines_setup_in_file(filepath):
    """Fix Lines setup patterns in a single file"""
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Pattern to match: lines = std::make_shared<Lines>(N);
        pattern = r'lines\s*=\s*std::make_shared<Lines>\s*\(\s*(\d+)\s*\)\s*;'
        
        def replacement(match):
            num_lines = int(match.group(1))
            lines_code = []
            lines_code.append("if (lines->size() == 0) {")
            for i in range(num_lines):
                lines_code.append(f"        lines->add_line(std::make_shared<LineBuffer>());")
            lines_code.append("    }")
            return '\n    '.join(lines_code)
        
        # Apply the replacement
        new_content = re.sub(pattern, replacement, content)
        
        if new_content != content:
            with open(filepath, 'w') as f:
                f.write(new_content)
            print(f"Fixed Lines setup in {filepath}")
            return True
        return False
        
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return False

def main():
    # Find all indicator cpp files
    indicator_files = glob.glob("../../src/indicators/*.cpp")
    
    fixed_count = 0
    for filepath in indicator_files:
        if fix_lines_setup_in_file(filepath):
            fixed_count += 1
    
    print(f"Fixed Lines setup in {fixed_count} files")

if __name__ == "__main__":
    main()