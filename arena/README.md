# 🏟️ `chingrep` vs `ripgrep` Speed Arena

This folder contains the official automated Multi-Parameter Speed Arena comparing **`chingrep` (Pure ARM64 Assembly + NEON SIMD)** against **`ripgrep` (Rust)** across 22 diverse real-world search parameter combinations on 17,000+ files.

---

## 🚀 How to Run the Arena

```bash
cd "/Volumes/Mac Backup/htdocs/treesource/chingrep"

# Run via Makefile
make arena

# Or run script directly with optional target path
bash arena/run-arena.sh [TARGET_DIR]
```

---

## 🧪 22 Parameter Permutations Tested

### 📌 Section 1: Core Search & Flag Permutations
1. **`-n`**: Exact literal substring search with 1-based line numbers
2. **`-i`**: Case-insensitive substring search (ASCII lowercasing)
3. **`-w`**: Whole-word boundary search
4. **`-rniw`**: Clustered multi-flag: Case-insensitive + Whole-word + Line numbers
5. **`-c`**: Match counting mode
6. **`-ci`**: Match counting + Case-insensitivity
7. **`-cw`**: Match counting + Whole-word boundaries
8. **`-l`**: Files with matches (short-circuiting after first match)
9. **`-li`**: Files with matches + Case-insensitivity
10. **`-lw`**: Files with matches + Whole-word boundaries

### 📌 Section 2: Limits, Globs & Filters
11. **`-m 1`**: Max count limit = 1 per file
12. **`-m 5`**: Max count limit = 5 per file
13. **`-m 10`**: Max count limit = 10 per file
14. **`--include='*.js'`**: File extension inclusion filter
15. **`--include='*.c'`**: File extension inclusion filter
16. **`--exclude-dir='test'`**: Recursive directory skipping

### 📌 Section 3: Data Hit Density & Frequency
17. **High Frequency Keyword (`return`)**: Over 5,000 line matches across the codebase
18. **Medium Frequency Keyword (`interface`)**: Hundreds of line matches
19. **Sparse Keyword (`VirtualBuffer`)**: Tens of line matches
20. **Ultra-Rare Keyword (`DecompressionStream`)**: Under 10 matches
21. **Nonexistent Needle (`XYZ_NEVER_FOUND_99999`)**: 0 matches (pure traversal & binary probing speed)

### 📌 Section 4: Streaming Pipelines
22. **STDIN Pipeline**: Piped stream through Unix pipe (`cat ... | cg`)

---

## 📄 Official Benchmark Whitepaper
Detailed timings and speedup multiples are exported to:
👉 [ARENA_REPORT.md](file:///Volumes/Mac%20Backup/htdocs/treesource/chingrep/arena/ARENA_REPORT.md)
