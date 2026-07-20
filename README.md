# FastScanner

Fast multi-threaded file search tool for Windows (GUI + CLI), built with C++17 and Qt6.

![FastScanner](docs/screenshot.png)

## Benchmarks

| Tool        | Time (~1700 files) |
|-------------|--------------------|
| FastScanner | ~16 ms             |
| ripgrep     | ~40 ms             |
| grep        | ~1590 ms           |

*Measured with PowerShell Measure-Command on Windows 11, averaged over multiple runs.*
*FastScanner is optimized for scanning many small to medium files in parallel. For single large file, single-threaded tools with optimized search algorithms (e.g. grep) may perform better.*

## Features

- Multi-threaded directory scanning with thread pool
- Zero-copy file search using memory mapping
- Filename and content search modes
- Configurable extension filter with custom input
- Real-time result display via callback
- Right-click actions: Open File, Open in Explorer, Copy Path
- Unicode path support (Korean filenames tested)

## Download

See [Releases](https://github.com/hongsumin-cs/FastScanner/releases/tag/v1.0.0) for Windows builds (GUI & CLI).

## Troubleshooting Notes

- **Abort() on scan**: Exceptions due to junction points from worker thread caused terminate. Fixed by wrapping try-catch and ensuring the catch block itself doesn't throw. 
- **Unicode Exception**: Paths with Korean filenames crashed due to implicit string() conversion (CP949). Fixed by keeping paths as std::filesystem::path and using u8string()/CreateFileW.
- **Interleaved CLI output**: Multiple threads writing via operator<< produced mixed lines. Fixed by building the full string before a single cout call.

## Build from Source

Requirements: CMake 3.16+, Qt 6.x, MSVC 2022

​```
git clone https://github.com/hongsumin-cs/FastScanner.git
cmake -B build
cmake --build build --config Release
​```

## Architecture

- `FastScanner` — core scanning engine
- `main.cpp` — CLI frontend
- `gui_main.cpp` — Qt GUI frontend

Both frontends use the same engine.

## Roadmap

- [ ] Matched file count separation
- [ ] Cross-platform builds
- [ ] Replace content search with Boyer-Moore algorithm for large files
- [ ] Parallel chunk-based search within large files