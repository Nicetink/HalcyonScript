# HalcyonScript

A native programming language for building Windows desktop applications with a simple, readable syntax.

**Version:** 0.26.26  
**Runtime:** HalcyonRT

## Overview

HalcyonScript is a domain-specific language designed for rapid GUI application development on Windows. It compiles to native executables and includes two UI frameworks:

- **HalGUI** - Modern themed UI with custom rendering
- **HalForms** - Windows Forms-like native controls

## Features

- Simple, human-readable syntax
- Native Windows GUI support via HalGUI (modern themed UI)
- **NEW: DVD/CD API** - Complete optical drive control: read discs, manage trays, copy files
- **NEW: System Information API** - Get OS, CPU, memory, disk, and screen info
- **NEW: Clipboard API** - Copy/paste text to/from system clipboard
- **NEW: Drag & Drop API** - Enable drag and drop functionality (preview)
- **NEW: Automatic Layout System** - No more manual coordinate calculations!
- **NEW: Paint API** - Complete graphics editor functionality with brushes, filters, and file support
- **NEW: Console API** - Full terminal control with colors, cursor positioning, and input
- **NEW: Enhanced String & Math Functions** - String.split(), Math.random(), Tooltip.show()
- HalForms framework (Windows Forms-like, native controls)
- Built-in theming system (Dark, Light, Midnight, Ocean, Teal)
- Event-driven programming model
- File I/O operations
- Audio playbook support
- Project-based builds with .halproj files
- Single executable distribution
- Windows XP/7/10/11 compatibility

## Quick Start

### HalGUI (Modern UI)
```
# Hello World with HalGUI
HalGUI.init()
HalGUI.setTheme("dark")

create window app "My Application" 800 600
create button btn "Click Me" x:20 y:20 width:150 height:40
create label lbl "Status: Ready" x:20 y:80 width:300 height:30

when btn clicked {
    set lbl.text = "Button clicked!"
}

HalGUI.run()
```

### HalForms (Windows Forms-like)
```
# Hello World with HalForms
HalForms.init()

HalForms.createForm("mainForm", "My Application", 800, 600, 0)
HalForms.createControl("button", "btn", "Click Me", 20, 20, 150, 40)
HalForms.createControl("label", "lbl", "Status: Ready", 20, 80, 300, 30)

on btn.clicked {
    HalForms.setProperty("lbl", "text", "Button clicked!")
}

HalForms.run()
```

### NEW: Automatic Layout System
No more manual coordinate calculations! Use automatic layouts:

```
HalGUI.init()
HalGUI.setTheme("dark")

window main "Layout Demo" 400 300

# Create a panel with vertical layout
panel container {
    x: 20
    y: 20
    width: 360
    height: 260
}

# Set vertical layout - widgets will auto-arrange!
HalGUI.setLayout("container", "vertical")
HalGUI.setGap("container", 10)

# Just add widgets - they position automatically!
label lblTitle "Welcome!" { width: 340, height: 30 }
input txtName "Enter name" { width: 340, height: 36 }
button btnSubmit "Submit" { width: 340, height: 40 }
label lblResult "" { width: 340, height: 30 }

on btnSubmit clicked {
    lblResult.text = "Hello, " + txtName.text + "!"
}

HalGUI.run()
```

**Layout Types:**
- `vertical` - Stack widgets vertically
- `horizontal` - Arrange widgets in a row
- `grid` - Auto-wrap grid layout
- `flex` - Proportional sizing with flex weights

See `docs/LAYOUT_SYSTEM.md` for full documentation.

## Installation

### Requirements
- Windows 10/11
- MinGW-w64 (for building from source)

### Build from Source
```batch
cd HalcyonScript
build_all.bat
```

This builds HalcyonScript with both HalGUI and HalForms support.

### Run a Script
```batch
Halcyon script.hcs
```

### Build a Project
```batch
Halcyon build project.halproj
```

## Language Syntax

### Variables
```
var name = "John"
const PI = 3.14159
global counter = 0
```

### Control Flow
```
if condition {
    # code
} else {
    # code
}

for i from 1 to 10 {
    print i
}

while running {
    # code
}
```

### Functions
```
func add(a, b) {
    return a + b
}

var result = add(5, 3)
```

### GUI Elements
```
create window win "Title" 800 600
create button btn "Text" x:10 y:10 width:100 height:30
create label lbl "Text" x:10 y:50 width:200 height:25
create input inp "" x:10 y:90 width:200 height:30
create checkbox chk "Option" x:10 y:130 width:150 height:25
create slider sld x:10 y:170 width:200 height:30 min:0 max:100 value:50
create progress prg x:10 y:210 width:200 height:20
create textarea txt "" x:10 y:250 width:300 height:150
```

### Events
```
when btn clicked {
    # handle click
}

when inp changed {
    get inp.text -> value
    print value
}
```

### Properties
```
set lbl.text = "New text"
set btn.visible = true
set chk.checked = false

get inp.text -> userInput
get sld.value -> sliderValue
```

### Dialogs
```
HalGUI.dialog("Message", "Title", 0, 0)

var file = HalGUI.openFile("Select File", "Text Files (*.txt)|*.txt")
var save = HalGUI.saveFile("Save As", "All Files (*.*)|*.*", "default.txt")
```

### File Operations
```
var content = File.read("data.txt")
File.write("output.txt", content)
var exists = File.exists("config.ini")
```

### Audio
```
Audio.create("player")
Audio.load("player", "music.mp3")
Audio.play("player")
Audio.setVolume("player", 80)
Audio.pause("player")
Audio.stop("player")
```

### NEW: Console API
Full terminal control for console applications:

```
# Initialize console
Console.init()
Console.setTitle("My Console App")

# Colored output
Console.setColor("bright_green")
Console.writeln("SUCCESS: Operation completed!")
Console.resetColor()

# User input
Console.write("Enter your name: ")
var name = Console.read()

# Screen control
Console.clear()
Console.setCursor(10, 5)
Console.write("Text at position 10,5")

# Progress bar example
for i from 0 to 100 step 5 {
    Console.setCursor(0, 10)
    Console.write("Progress: [")
    var filled = i / 2
    for j from 0 to filled {
        Console.write("█")
    }
    Console.write("] " + i + "%")
    Sys.sleep(100)
}
```

### NEW: Paint API
Complete graphics editor functionality:

```
# Create paint canvas
paintcanvas_create("mainForm", "canvas", 10, 10, 800, 600)
paintcanvas_clear("canvas", 255, 255, 255)

# Set drawing tools
paintcanvas_set_tool("canvas", "brush")
paintcanvas_set_brush_size("canvas", 10)
paintcanvas_set_brush_type("canvas", "soft")
paintcanvas_set_color("canvas", 255, 0, 0)

# Image filters
paintcanvas_grayscale("canvas")
paintcanvas_blur("canvas", 3)
paintcanvas_brightness("canvas", 50)
paintcanvas_invert("canvas")

# File operations
paintcanvas_save("canvas", "artwork.bmp")
paintcanvas_load("canvas", "photo.bmp")

# Undo/Redo
paintcanvas_undo("canvas")
paintcanvas_redo("canvas")
```

### NEW: Enhanced Functions
String manipulation and utilities:

```
# String splitting
var csv = "apple,banana,orange"
var fruits = String.split(csv, ",")
# fruits = ["apple", "banana", "orange"]

# Random numbers
var dice = Math.random(1, 6)
var percentage = Math.random(0, 100)

# Tooltips
Tooltip.show("Hello World!", 100, 100)
on btn clicked {
    Tooltip.show("Button clicked!", 200, 150)
}
```

### NEW: System Information API
Get detailed system information:

```
# Get OS information
var os = SysInfo.getOS()
Console.writeln("Operating System: " + os)

# Get computer and user info
var computerName = SysInfo.getComputerName()
var userName = SysInfo.getUserName()
Console.writeln("Computer: " + computerName)
Console.writeln("User: " + userName)

# Get CPU information
var cpuCount = SysInfo.getCPUCount()
Console.writeln("CPU Cores: " + cpuCount)

# Get memory information
var memTotal = SysInfo.getMemoryTotal()
var memAvail = SysInfo.getMemoryAvailable()
Console.writeln("Memory: " + memAvail + " / " + memTotal + " MB")

# Get disk space
var diskInfo = SysInfo.getDiskSpace("C:\\")
Console.writeln("Disk C: " + diskInfo.free + " / " + diskInfo.total + " GB")

# Get screen resolution
var screen = SysInfo.getScreenResolution()
Console.writeln("Screen: " + screen.width + " x " + screen.height)
```

### NEW: Clipboard API
Work with system clipboard:

```
# Copy text to clipboard
Clipboard.setText("Hello from HalcyonScript!")

# Check if clipboard has text
if (Clipboard.hasText()) {
    # Get text from clipboard
    var text = Clipboard.getText()
    Console.writeln("Clipboard: " + text)
}

# Example: Copy system info to clipboard
var report = "=== System Report ===\n"
report = report + "OS: " + SysInfo.getOS() + "\n"
report = report + "User: " + SysInfo.getUserName() + "\n"
Clipboard.setText(report)
```

### NEW: Drag & Drop API (Coming Soon)
Enable drag and drop functionality:

```
# Enable drag for a widget
Widget.enableDrag("myButton", true)
Widget.setDragData("myButton", "Button data")

# Enable drop for a widget
Widget.enableDrop("myPanel", true)

# Get drag data
on myPanel dropped {
    var data = Widget.getDragData("myButton")
    Console.writeln("Dropped: " + data)
}
```

## Project Structure

```
MyProject/
├── MyProject.halproj
├── src/
│   ├── main.hcs
│   └── utils.hcs
└── assets/
    └── icon.ico
```

### Project File Format
```json
{
    "name": "MyProject",
    "version": "1.0.0",
    "entry": "src/main.hcs",
    "files": ["src/main.hcs", "src/utils.hcs"],
    "build": {
        "output": "dist",
        "icon": "assets/icon.ico"
    }
}
```

## Themes

Available themes: `dark`, `light`, `midnight`, `ocean`, `teal`

```
HalGUI.setTheme("midnight")
```

## Directory Structure

```
HalcyonScript-Native/
├── src/
│   ├── main.c           # Entry point
│   ├── lexer.c          # Tokenizer
│   ├── parser.c         # AST builder
│   ├── runtime.c        # Interpreter
│   ├── builtin_api.c    # Built-in functions (String.split, Math.random, etc.)
│   ├── console_api.c    # Console API implementation
│   ├── halgui/          # HalGUI framework (modern themed UI)
│   │   ├── halgui_core.c
│   │   ├── halgui_widgets.c
│   │   ├── halgui_render.c
│   │   ├── halgui_themes.c
│   │   ├── halgui_dialogs.c
│   │   └── halgui_audio.c
│   ├── halforms/        # HalForms framework (Windows Forms-like)
│   │   ├── halforms_core.c
│   │   ├── halforms_controls.c
│   │   ├── halforms_menu.c
│   │   ├── halforms_advanced.c
│   │   ├── halforms_dialogs.c
│   │   ├── halforms_paint.c      # Paint API implementation
│   │   └── halforms_runtime.c
│   └── launcher/        # Standalone app builder
├── examples/            # Sample programs
├── docs/               # Documentation
│   ├── LAYOUT_SYSTEM.md    # Layout system guide
│   ├── CONSOLE_API.md      # Console API reference
│   ├── PAINT_API.md        # Paint API reference
│   ├── SYSINFO_API.md      # System Information API reference
│   ├── CLIPBOARD_API.md    # Clipboard API reference
│   ├── DVD_API.md          # DVD/CD Optical Drive API reference
│   └── NEW_FUNCTIONS.md    # New built-in functions
└── logo/               # Branding assets
```

## UI Frameworks Comparison

| Feature | HalGUI | HalForms |
|---------|--------|----------|
| Rendering | Custom GDI+/D3D11 | Native Win32 |
| Themes | dark, light, midnight, ocean, teal | System |
| Compatibility | Windows 7+ | Windows XP+ |
| Best for | Modern apps | IDE, DAW, editors |
| Controls | Styled widgets | Native controls |
| Layout System | ✅ Automatic layouts | ❌ Manual positioning |
| Paint API | ❌ Not supported | ✅ Full paint functionality |
| Console API | ✅ Supported | ✅ Supported |

## Examples

See the `examples/` directory for sample applications:

### HalGUI Examples
- `halgui_simple.hcs` - Basic GUI demo
- `calc.hcs` - Calculator
- `system_info_viewer.hcs` - System information viewer with clipboard support
- `clipboard_dragdrop_demo.hcs` - Clipboard and drag & drop demo
- `audioplayer.hcs` - Music player

### HalForms Examples
- `halforms_demo.hcs` - HalForms controls demo
- `halforms_editor.hcs` - Simple text editor

### NEW: Console Examples
- `console_calculator.hcs` - Console-based calculator with colors
- `test_console.hcs` - Console API demonstration

### NEW: Paint Examples
- `simple_paint_demo.hcs` - Basic paint application
- `test_paint_draw.hcs` - Drawing tools demonstration
- `test_paint_console.hcs` - Paint with console output

### NEW: Layout Examples
- `simple_layout.hcs` - Automatic layout demonstration
- `advanced_layout_app.hcs` - Complex layout with multiple panels
- `responsive_layout_demo.hcs` - Responsive design patterns

### NEW: DVD/CD Examples
- `test_dvd_api.hcs` - Complete DVD API demonstration
- `dvd_backup_tool.hcs` - Professional DVD backup utility

## License

MIT License

## Author

KAInaps
