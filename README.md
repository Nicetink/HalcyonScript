# HalcyonScript

A native programming language for building Windows desktop applications with a simple, readable syntax.

**Version:** 0.18.26  
**Runtime:** HalcyonRT

## Overview

HalcyonScript is a domain-specific language designed for rapid GUI application development on Windows. It compiles to native executables and includes two UI frameworks:

- **HalGUI** - Modern themed UI with custom rendering
- **HalForms** - Windows Forms-like native controls

## Features

- Simple, human-readable syntax
- Native Windows GUI support via HalGUI (modern themed UI)
- HalForms framework (Windows Forms-like, native controls)
- Built-in theming system (Dark, Light, Midnight, Ocean, Teal)
- Event-driven programming model
- File I/O operations
- Audio playback support
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
│   │   └── halforms_runtime.c
│   └── launcher/        # Standalone app builder
├── examples/            # Sample programs
├── docs/               # Documentation
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

## Examples

See the `examples/` directory for sample applications:

### HalGUI Examples
- `halgui_simple.hcs` - Basic GUI demo
- `calc.hcs` - Calculator
- `audioplayer.hcs` - Music player

### HalForms Examples
- `halforms_demo.hcs` - HalForms controls demo
- `halforms_editor.hcs` - Simple text editor

## License

MIT License

## Author

KAInaps
