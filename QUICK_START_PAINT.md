# Quick Start - HalcyonScript Paint API

## Быстрый старт за 5 минут! 🚀

### Шаг 1: Сборка с Paint API

```bash
# Windows
cd HalcyonScript-Native
build_with_paint.bat
build_all.bat
```

### Шаг 2: Запуск примера

```bash
# Простой Paint
halcyon.exe examples/simple_paint.hcs

# Полный Paint с фильтрами
halcyon.exe examples/paint_app.hcs
```

### Шаг 3: Создайте свой первый Paint!

Создайте файл `my_paint.hcs`:

```javascript
import "halforms"

function main() {
    halforms_init()
    
    // Окно
    halform_create("My Paint", 900, 700)
    halform_center()
    
    // Кнопки
    halctrl_button("mainForm", "Brush", 10, 10, 80, 30, "onBrush")
    halctrl_button("mainForm", "Eraser", 10, 45, 80, 30, "onEraser")
    halctrl_button("mainForm", "Clear", 10, 80, 80, 30, "onClear")
    
    // Холст
    paintcanvas_create("mainForm", "canvas", 100, 10, 750, 650)
    paintcanvas_clear("canvas", 255, 255, 255)
    paintcanvas_set_tool("canvas", "brush")
    paintcanvas_set_color("canvas", 0, 0, 0)
    paintcanvas_set_brush_size("canvas", 5)
    
    halform_show()
    halforms_run()
}

function onBrush() {
    paintcanvas_set_tool("canvas", "brush")
}

function onEraser() {
    paintcanvas_set_tool("canvas", "eraser")
}

function onClear() {
    paintcanvas_clear("canvas", 255, 255, 255)
}

main()
```

Запустите:
```bash
halcyon.exe my_paint.hcs
```

## Основные функции

### Создание холста
```javascript
paintcanvas_create("mainForm", "canvas", x, y, width, height)
```

### Выбор инструмента
```javascript
paintcanvas_set_tool("canvas", "brush")    // Кисть
paintcanvas_set_tool("canvas", "pencil")   // Карандаш
paintcanvas_set_tool("canvas", "eraser")   // Ластик
paintcanvas_set_tool("canvas", "fill")     // Заливка
```

### Настройка цвета
```javascript
paintcanvas_set_color("canvas", 255, 0, 0)    // Красный
paintcanvas_set_color("canvas", 0, 255, 0)    // Зеленый
paintcanvas_set_color("canvas", 0, 0, 255)    // Синий
```

### Размер кисти
```javascript
paintcanvas_set_brush_size("canvas", 3)   // Тонкая
paintcanvas_set_brush_size("canvas", 10)  // Средняя
paintcanvas_set_brush_size("canvas", 20)  // Толстая
```

### Сохранение/Загрузка
```javascript
paintcanvas_save("canvas", "myart.bmp")
paintcanvas_load("canvas", "photo.bmp")
```

### Отмена/Повтор
```javascript
paintcanvas_undo("canvas")
paintcanvas_redo("canvas")
```

### Фильтры
```javascript
paintcanvas_grayscale("canvas")           // Ч/Б
paintcanvas_invert("canvas")              // Негатив
paintcanvas_brightness("canvas", 50)      // Ярче
paintcanvas_blur("canvas", 2)             // Размытие
```

## Примеры использования

### Цветовая палитра

```javascript
// Создать кнопки цветов
function createPalette() {
    var colors = [
        {r: 0, g: 0, b: 0, name: "Black"},
        {r: 255, g: 0, b: 0, name: "Red"},
        {r: 0, g: 255, b: 0, name: "Green"},
        {r: 0, g: 0, b: 255, name: "Blue"}
    ]
    
    for (var i = 0; i < colors.length; i++) {
        var c = colors[i]
        var btn = halctrl_button("mainForm", c.name, 
                                 10, 100 + i * 35, 80, 30, "onColor")
        halctrl_set_backcolor(btn, c.r, c.g, c.b)
        halctrl_set_tag(btn, i)
    }
}

function onColor(sender, event) {
    var colors = [
        {r: 0, g: 0, b: 0},
        {r: 255, g: 0, b: 0},
        {r: 0, g: 255, b: 0},
        {r: 0, g: 0, b: 255}
    ]
    var i = halctrl_get_tag(sender)
    var c = colors[i]
    paintcanvas_set_color("canvas", c.r, c.g, c.b)
}
```

### Меню с действиями

```javascript
function createMenu() {
    var menu = halmenu_create()
    
    var fileMenu = halmenu_add_submenu(menu, "File")
    halmenuitem_add_item(fileMenu, "New", "onNew")
    halmenuitem_add_item(fileMenu, "Open", "onOpen")
    halmenuitem_add_item(fileMenu, "Save", "onSave")
    
    var editMenu = halmenu_add_submenu(menu, "Edit")
    halmenuitem_add_item(editMenu, "Undo", "onUndo")
    halmenuitem_add_item(editMenu, "Redo", "onRedo")
    
    halform_set_menu("mainForm", menu)
}

function onNew() {
    paintcanvas_clear("canvas", 255, 255, 255)
}

function onOpen() {
    var file = halforms_open_file("Open", "*.bmp")
    if (file) paintcanvas_load("canvas", file)
}

function onSave() {
    var file = halforms_save_file("Save", "*.bmp", "art.bmp")
    if (file) paintcanvas_save("canvas", file)
}

function onUndo() {
    paintcanvas_undo("canvas")
}

function onRedo() {
    paintcanvas_redo("canvas")
}
```

### Регулятор размера кисти

```javascript
// Создать слайдер
var slider = halctrl_trackbar("mainForm", 10, 200, 80, 30, 1, 50)
halctrl_set_value(slider, 5)
halctrl_on_change(slider, "onSizeChange")

function onSizeChange(sender, event) {
    var size = halctrl_get_value(sender)
    paintcanvas_set_brush_size("canvas", size)
}
```

## Полезные советы

### 1. Двойная буферизация
Canvas автоматически использует двойную буферизацию - не нужно беспокоиться о мерцании!

### 2. Производительность
Для больших изображений:
```javascript
// Уменьшите уровни отмены
// Используйте меньший размер кисти
// Применяйте фильтры к выделенным областям
```

### 3. Сохранение состояния
```javascript
// Автоматически сохраняется при начале рисования
// Можно вручную: paintcanvas_save_state("canvas")
```

### 4. Обработка ошибок
```javascript
if (!paintcanvas_save("canvas", "file.bmp")) {
    halforms_msgbox("Save failed!", "Error", 0, 16)
}
```

## Что дальше?

1. **Изучите примеры**:
   - `examples/simple_paint.hcs` - базовый редактор
   - `examples/paint_app.hcs` - полный функционал

2. **Читайте документацию**:
   - `docs/PAINT_API.md` - полный API reference

3. **Экспериментируйте**:
   - Добавьте новые инструменты
   - Создайте свои фильтры
   - Сделайте уникальный интерфейс

## Создайте что-то потрясающее! 🎨

С Paint API вы можете создать:
- Графический редактор
- Редактор скриншотов
- Пиксель-арт редактор
- Детское приложение для рисования
- Редактор диаграмм
- И многое другое!

**Удачи в творчестве! ✨**
