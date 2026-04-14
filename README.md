# wxhearts

A cross-platform port of Microsoft Hearts built with [wxWidgets](https://www.wxwidgets.org/). Runs on Windows, macOS, and Linux.

## Screenshots

### Windows
![Windows screenshot](/screenshots/wxhearts_win.png)

### macOS
![macOS screenshot](/screenshots/wxhearts_macOS.png)

### Linux
![Linux screenshot](/screenshots/wxhearts_linux.png)

## Building

### Prerequisites

- CMake 3.14+
- C++17 compiler
- wxWidgets (with `core`, `base`, `xrc`, `html`, `xml` components)
- `wxrc` tool (ships with wxWidgets)

### Build steps

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

On macOS this produces a `wxhearts.app` bundle. On Linux the binary can be installed system-wide:

```bash
cmake --install .
```

## License

GPL-2.0
