# HalcyonScript

A native programming language for building Windows desktop applications with a simple, readable syntax.

## Overview

HalcyonScript is a domain-specific language designed for rapid GUI application development on Windows. It compiles to native executables and includes HalGUI, a built-in graphics framework with hardware-accelerated rendering.

## Features

- Simple, human-readable syntax
- Native Windows GUI support via HalGUI
- Built-in theming system (Dark, Light, Midnight, Ocean, Teal)
- Event-driven programming model
- File I/O operations
- Audio playback support
- Project-based builds with .halproj files
- Single executable distribution

## Quick Start

```
# Hello World
print "Hello, World!"

# GUI Application
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

## Installation

### Requirements
- Windows 10/11
- MinGW-w64 (for building from source)

### Build from Source
```batch
cd HalcyonScript-Native
build_and_install.bat
```

### Run a Script
```batch
halcyon.exe script.hcs
```

### Build a Project
```batch
halcyon.exe build project.halproj
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
│   ├── halgui/          # GUI framework
│   │   ├── halgui_core.c
│   │   ├── halgui_widgets.c
│   │   ├── halgui_render.c
│   │   ├── halgui_themes.c
│   │   ├── halgui_dialogs.c
│   │   └── halgui_audio.c
│   └── launcher/        # Standalone app builder
├── examples/            # Sample programs
├── docs/               # Documentation
└── logo/               # Branding assets
```

## Examples

See the `examples/` directory for sample applications:
- `halgui_simple.hcs` - Basic GUI demo
- `calc.hcs` - Calculator
- `audioplayer.hcs` - Music player

## License

MIT License

## Author

KAInaps
