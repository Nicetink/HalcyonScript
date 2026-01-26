# HalGUI Layout System

Система автоматического расположения виджетов в HalGUI. Больше не нужно вручную вычислять координаты для каждого элемента!

## Типы Layout

### 1. Вертикальный Layout (vertical)
Виджеты располагаются вертикально, один под другим.

```hcs
panel container {
    x: 10
    y: 10
    width: 300
    height: 400
}

HalGUI.setLayout("container", "vertical")
HalGUI.setGap("container", 10)  // Отступ между элементами

// Виджеты автоматически расположатся вертикально
label lbl1 "Первый" { width: 280, height: 30 }
label lbl2 "Второй" { width: 280, height: 30 }
label lbl3 "Третий" { width: 280, height: 30 }
```

### 2. Горизонтальный Layout (horizontal)
Виджеты располагаются горизонтально, в ряд.

```hcs
panel toolbar {
    x: 10
    y: 10
    width: 500
    height: 50
}

HalGUI.setLayout("toolbar", "horizontal")
HalGUI.setGap("toolbar", 8)

// Кнопки выстроятся в ряд
button btn1 "Кнопка 1" { width: 100, height: 40 }
button btn2 "Кнопка 2" { width: 100, height: 40 }
button btn3 "Кнопка 3" { width: 100, height: 40 }
```

### 3. Grid Layout (grid)
Виджеты располагаются в виде сетки с автоматическим переносом.

```hcs
panel grid {
    x: 10
    y: 10
    width: 400
    height: 300
}

HalGUI.setLayout("grid", "grid")
HalGUI.setGap("grid", 10)

// Виджеты автоматически расположатся в сетку
button btn1 "1" { width: 80, height: 80 }
button btn2 "2" { width: 80, height: 80 }
button btn3 "3" { width: 80, height: 80 }
button btn4 "4" { width: 80, height: 80 }
```

### 4. Flex Layout (flex)
Виджеты располагаются с учётом flex-весов (пропорциональное распределение).

```hcs
panel flexPanel {
    x: 10
    y: 10
    width: 500
    height: 50
}

HalGUI.setLayout("flexPanel", "flex")
HalGUI.setGap("flexPanel", 10)

button btnSmall "Маленькая" { height: 40 }
HalGUI.setFlex("btnSmall", 1.0)  // Займёт 1 часть

button btnMedium "Средняя" { height: 40 }
HalGUI.setFlex("btnMedium", 2.0)  // Займёт 2 части

button btnLarge "Большая" { height: 40 }
HalGUI.setFlex("btnLarge", 3.0)  // Займёт 3 части
```

## API Функции

### HalGUI.setLayout(panelName, layoutType)
Устанавливает тип layout для панели.

**Параметры:**
- `panelName` (string) - имя панели
- `layoutType` (string) - тип layout: "vertical", "horizontal", "grid", "flex"

**Пример:**
```hcs
HalGUI.setLayout("myPanel", "vertical")
```

### HalGUI.setGap(panelName, gap)
Устанавливает расстояние между виджетами в layout.

**Параметры:**
- `panelName` (string) - имя панели
- `gap` (number) - расстояние в пикселях (по умолчанию 8)

**Пример:**
```hcs
HalGUI.setGap("myPanel", 15)
```

### HalGUI.setFlex(widgetName, flex)
Устанавливает flex-вес для виджета (только для flex layout).

**Параметры:**
- `widgetName` (string) - имя виджета
- `flex` (number) - вес (1.0, 2.0, 3.0 и т.д.)

**Пример:**
```hcs
HalGUI.setFlex("myButton", 2.0)
```

### HalGUI.setMargin(widgetName, top, right, bottom, left)
Устанавливает отступы вокруг виджета.

**Параметры:**
- `widgetName` (string) - имя виджета
- `top` (number) - отступ сверху
- `right` (number) - отступ справа
- `bottom` (number) - отступ снизу
- `left` (number) - отступ слева

**Пример:**
```hcs
HalGUI.setMargin("myButton", 10, 5, 10, 5)
```

### HalGUI.applyLayout(panelName)
Принудительно применяет layout (обычно вызывается автоматически).

**Параметры:**
- `panelName` (string) - имя панели

**Пример:**
```hcs
HalGUI.applyLayout("myPanel")
```

## Полный пример

```hcs
HalGUI.init()
HalGUI.setTheme("dark")

window main "Layout Demo" 400 500

// Главная панель с вертикальным layout
panel mainPanel {
    x: 20
    y: 20
    width: 360
    height: 460
}

HalGUI.setLayout("mainPanel", "vertical")
HalGUI.setGap("mainPanel", 10)

// Заголовок
label lblTitle "Форма регистрации" {
    width: 340
    height: 30
}

// Поля ввода
label lblName "Имя:" { width: 340, height: 24 }
input txtName "" { width: 340, height: 36 }

label lblEmail "Email:" { width: 340, height: 24 }
input txtEmail "" { width: 340, height: 36 }

label lblPassword "Пароль:" { width: 340, height: 24 }
input txtPassword "" { width: 340, height: 36 }

// Панель с кнопками (горизонтальный layout)
panel btnPanel {
    width: 340
    height: 50
}

HalGUI.setLayout("btnPanel", "horizontal")
HalGUI.setGap("btnPanel", 10)

button btnSubmit "Отправить" { width: 165, height: 40 }
button btnCancel "Отмена" { width: 165, height: 40 }

// Статус
label lblStatus "" { width: 340, height: 30 }

// Обработчики
on btnSubmit clicked {
    lblStatus.text = "Регистрация..."
}

on btnCancel clicked {
    lblStatus.text = "Отменено"
}

HalGUI.run()
```

## Преимущества

1. **Меньше кода** - не нужно вручную вычислять координаты
2. **Проще поддержка** - изменение одного элемента не ломает остальные
3. **Адаптивность** - легко добавлять/удалять элементы
4. **Читаемость** - код становится понятнее

## Советы

- Используйте `vertical` для форм и списков
- Используйте `horizontal` для панелей инструментов
- Используйте `flex` когда нужно пропорциональное распределение
- Используйте `grid` для галерей и наборов кнопок
- Комбинируйте разные типы layout в одном приложении

## Примеры

Смотрите примеры в папке `examples/`:
- `simple_layout.hcs` - простой вертикальный layout
- `horizontal_layout.hcs` - горизонтальная панель инструментов
- `layout_demo.hcs` - полная демонстрация всех типов layout
