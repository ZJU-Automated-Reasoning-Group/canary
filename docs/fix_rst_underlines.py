#!/usr/bin/env python3
"""
Script to fix RST underlines in documentation files.
This ensures the underline has exactly the same length as the title text.
"""

import os
import re
import sys
from pathlib import Path

def fix_underlines(file_path):
    """Fix underlines in a single RST file."""
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Pattern to find title and underline combinations
    # Title is any text, followed by a newline, followed by one or more chars of a single type
    title_pattern = re.compile(r'([^\n]+)\n([=\-~^\'"]+)(\n|$)', re.MULTILINE)
    
    def fix_match(match):
        title = match.group(1)
        underline_char = match.group(2)[0]  # First char of underline
        # Create new underline of correct length
        new_underline = underline_char * len(title)
        return f"{title}\n{new_underline}{match.group(3)}"
    
    # Replace all title/underline combinations
    new_content = title_pattern.sub(fix_match, content)
    
    # Only write back if changes were made
    if new_content != content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python fix_rst_underlines.py <directory>")
        sys.exit(1)
    
    directory = sys.argv[1]
    dir_path = Path(directory)
    
    if not dir_path.exists() or not dir_path.is_dir():
        print(f"Error: {directory} is not a valid directory")
        sys.exit(1)
    
    print(f"Scanning for RST files in: {directory}")
    
    # Find all .rst files
    rst_files = list(dir_path.glob('**/*.rst'))
    print(f"Found {len(rst_files)} RST files")
    
    # Fix underlines in each file
    fixed_count = 0
    for rst_file in rst_files:
        if fix_underlines(rst_file):
            print(f"Fixed: {rst_file}")
            fixed_count += 1
    
    print(f"Total files fixed: {fixed_count}")

if __name__ == "__main__":
    main() 