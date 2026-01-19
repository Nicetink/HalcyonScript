HalcyonScript Console API Reference

Overview

The Console API provides full control over terminal/console applications, including colors, cursor positioning, and user input.

Initialization

Console.init()

Initializes the console. Call this at the start of your console application.

Output Functions

Console.write(text, ...)
Write text without newline. Accepts multiple arguments.

Console.write("Hello, ")
Console.write("World!")

Console.writeln(text, ...)
Write text with newline.

Console.writeln("Hello, World!")
Console.writeln("Line 1", "Line 2")

 Input Functions

Console.read()
Read a line of text from user input.
var name = Console.read()
Console.writeln("Hello, " + name)

Console.readKey()
Read a single keypress without waiting for Enter.

Console.write("Press any key...")
var key = Console.readKey()

Color Functions

Console.setColor(color)
Set text color. Available colors:
- `"black"`, `"blue"`, `"green"`, `"cyan"`
- `"red"`, `"magenta"`, `"yellow"`, `"white"`
- `"gray"`, `"bright_blue"`, `"bright_green"`, `"bright_cyan"`
- `"bright_red"`, `"bright_magenta"`, `"bright_yellow"`, `"bright_white"`

Console.setColor("bright_green")
Console.writeln("Success!")
Console.resetColor()


Console.resetColor()
Reset to default console colors.

Screen Control

 Console.clear()
Clear the console screen.

Console.clear()

Console.setTitle(title)
Set console window title.

Console.setTitle("My Application")

Console.getSize()
Get console dimensions.

var size = Console.getSize()
Console.writeln("Width: " + size.width)
Console.writeln("Height: " + size.height)

Cursor Control

Console.setCursor(x, y)
Move cursor to position (0-based).

Console.setCursor(10, 5)
Console.write("Text at position 10,5")


Console.hideCursor()
Hide the cursor.

Console.hideCursor()

Console.showCursor()
Show the cursor.

Console.showCursor()


Sound

Console.beep(frequency, duration)
Play a beep sound.

Console.beep(800, 200)

Utility

Sys.sleep(milliseconds)
Pause execution.

Sys.sleep(1000)


Complete Examples

Simple Menu

Console.init()
Console.setTitle("Menu Demo")

function showMenu() {
    Console.clear()
    Console.setColor("bright_cyan")
    Console.writeln("MAIN MENU")
    Console.resetColor()
    Console.writeln()
    Console.writeln("1. Option 1")
    Console.writeln("2. Option 2")
    Console.writeln("3. Exit")
    Console.writeln()
    Console.write("Select: ")
}

var running = true
while (running) {
    showMenu()
    var choice = Console.read()
    
    if (choice == "1") {
        Console.writeln("Option 1 selected")
        Sys.sleep(1000)
    } else if (choice == "2") {
        Console.writeln("Option 2 selected")
        Sys.sleep(1000)
    } else if (choice == "3") {
        running = false
    }
}
```

Progress Bar

Console.init()
Console.clear()

function drawProgress(percent) {
    Console.setCursor(0, 5)
    Console.write("[")
    
    var width = 50
    var filled = Math.floor(width * percent / 100)
    
    Console.setColor("bright_green")
    for (var i = 0; i < filled; i++) {
        Console.write("█")
    }
    Console.resetColor()
    
    for (var i = filled; i < width; i++) {
        Console.write("░")
    }
    
    Console.write("] " + percent + "%")
}

for (var i = 0; i <= 100; i += 5) {
    drawProgress(i)
    Sys.sleep(100)
}

Console.writeln()
Console.writeln()
Console.setColor("bright_green")
Console.writeln("Complete!")
Console.resetColor()


Colored Output

Console.init()
Console.clear()

Console.setColor("bright_red")
Console.writeln("ERROR: Something went wrong")

Console.setColor("bright_yellow")
Console.writeln("WARNING: Check your input")

Console.setColor("bright_green")
Console.writeln("SUCCESS: Operation completed")

Console.setColor("bright_cyan")
Console.writeln("INFO: Processing data...")

Console.resetColor()


Best Practices

1. Always call `Console.init()` at the start
2. Use `Console.resetColor()` after colored output
3. Clear screen before drawing complex UIs
4. Hide cursor during animations
5. Use `Sys.sleep()` for delays and animations
6. Handle user input validation
7. Provide clear exit options

Platform Support

- Windows: Full support