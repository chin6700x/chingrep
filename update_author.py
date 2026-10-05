#!/usr/bin/env python3
"""
Author: Wirot Chookeaw (chin6700x)
Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
License: MIT License

Script to update author name from 'Wirote Chukeaw' to 'Wirot Chookeaw' across chingrep.
"""

import os
import sys

OLD_NAME = "Wirote Chukeaw"
NEW_NAME = "Wirot Chookeaw"

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
IGNORE_DIRS = {".git", "bin"}
VALID_EXTS = {".c", ".h", ".s", ".sh", ".md", ".txt", ".json", "Makefile", "LICENSE"}

def process_file(filepath):
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()
    except Exception:
        return False

    if OLD_NAME in content:
        new_content = content.replace(OLD_NAME, NEW_NAME)
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"✅ Updated: {os.path.relpath(filepath, PROJECT_DIR)}")
        return True
    return False

def main():
    updated_count = 0
    total_scanned = 0

    for root, dirs, files in os.walk(PROJECT_DIR):
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
        for file in files:
            ext = os.path.splitext(file)[1]
            if ext in VALID_EXTS or file in VALID_EXTS:
                total_scanned += 1
                filepath = os.path.join(root, file)
                if process_file(filepath):
                    updated_count += 1

    print(f"\n🎉 Finished! Updated {updated_count} files (Scanned {total_scanned} files).")

if __name__ == "__main__":
    main()
