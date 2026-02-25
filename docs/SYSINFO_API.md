# System Information API

API для получения информации о системе в HalcyonScript.

## Обзор

System Information API предоставляет функции для получения различной информации о системе, включая:
- Информацию об операционной системе
- Имя компьютера и пользователя
- Информацию о процессоре
- Информацию о памяти
- Информацию о дисковом пространстве
- Разрешение экрана

## Функции

### SysInfo.getOS()

Возвращает название и версию операционной системы.

**Возвращает:** `string` - Название ОС (например, "Windows 11 (Build 22000)")

**Пример:**
```hcs
let os = SysInfo.getOS();
Console.writeln("Operating System: " + os);
// Output: Operating System: Windows 11 (Build 22000)
```

---

### SysInfo.getComputerName()

Возвращает имя компьютера.

**Возвращает:** `string` - Имя компьютера

**Пример:**
```hcs
let computerName = SysInfo.getComputerName();
Console.writeln("Computer: " + computerName);
// Output: Computer: DESKTOP-ABC123
```

---

### SysInfo.getUserName()

Возвращает имя текущего пользователя.

**Возвращает:** `string` - Имя пользователя

**Пример:**
```hcs
let userName = SysInfo.getUserName();
Console.writeln("User: " + userName);
// Output: User: John
```

---

### SysInfo.getCPUCount()

Возвращает количество процессорных ядер.

**Возвращает:** `number` - Количество ядер процессора

**Пример:**
```hcs
let cpuCount = SysInfo.getCPUCount();
Console.writeln("CPU Cores: " + cpuCount);
// Output: CPU Cores: 8
```

---

### SysInfo.getMemoryTotal()

Возвращает общий объем физической памяти в мегабайтах.

**Возвращает:** `number` - Общий объем памяти в МБ

**Пример:**
```hcs
let totalMem = SysInfo.getMemoryTotal();
Console.writeln("Total Memory: " + totalMem + " MB");
// Output: Total Memory: 16384 MB
```

---

### SysInfo.getMemoryAvailable()

Возвращает доступный объем физической памяти в мегабайтах.

**Возвращает:** `number` - Доступный объем памяти в МБ

**Пример:**
```hcs
let availMem = SysInfo.getMemoryAvailable();
Console.writeln("Available Memory: " + availMem + " MB");
// Output: Available Memory: 8192 MB
```

---

### SysInfo.getDiskSpace(drive)

Возвращает информацию о дисковом пространстве для указанного диска.

**Параметры:**
- `drive` (string) - Путь к диску (например, "C:\\")

**Возвращает:** `object` - Объект с полями:
- `total` (number) - Общий объем в ГБ
- `free` (number) - Свободное пространство в ГБ
- `used` (number) - Использованное пространство в ГБ

**Пример:**
```hcs
let diskInfo = SysInfo.getDiskSpace("C:\\");
Console.writeln("Total: " + diskInfo.total + " GB");
Console.writeln("Free: " + diskInfo.free + " GB");
Console.writeln("Used: " + diskInfo.used + " GB");
// Output:
// Total: 500 GB
// Free: 250 GB
// Used: 250 GB
```

---

### SysInfo.getScreenResolution()

Возвращает разрешение экрана.

**Возвращает:** `object` - Объект с полями:
- `width` (number) - Ширина экрана в пикселях
- `height` (number) - Высота экрана в пикселях

**Пример:**
```hcs
let screen = SysInfo.getScreenResolution();
Console.writeln("Resolution: " + screen.width + " x " + screen.height);
// Output: Resolution: 1920 x 1080
```

---

## Полный пример

```hcs
use HalGUI;

// Получаем всю информацию о системе
let os = SysInfo.getOS();
let computer = SysInfo.getComputerName();
let user = SysInfo.getUserName();
let cpuCount = SysInfo.getCPUCount();
let memTotal = SysInfo.getMemoryTotal();
let memAvail = SysInfo.getMemoryAvailable();
let diskInfo = SysInfo.getDiskSpace("C:\\");
let screen = SysInfo.getScreenResolution();

// Создаем окно для отображения
Window("System Info", 500, 400);

Panel("info", 10, 10, 480, 380);
Layout("info", "vertical", 5);

Label("osLabel", "OS: " + os);
Label("computerLabel", "Computer: " + computer);
Label("userLabel", "User: " + user);
Label("cpuLabel", "CPU Cores: " + cpuCount);
Label("memLabel", "Memory: " + memAvail + " / " + memTotal + " MB");
Label("diskLabel", "Disk C: " + diskInfo.free + " / " + diskInfo.total + " GB");
Label("screenLabel", "Screen: " + screen.width + " x " + screen.height);

Show();
Run();
```

## Примечания

- Все функции работают только на Windows
- Информация о памяти и диске обновляется в реальном времени при каждом вызове
- Для получения информации о других дисках используйте соответствующие буквы (например, "D:\\", "E:\\")
- Разрешение экрана возвращает размер основного монитора

## См. также

- [Clipboard API](CLIPBOARD_API.md) - Работа с буфером обмена
- [Console API](CONSOLE_API.md) - Работа с консолью
- [File System API](FILESYSTEM_API.md) - Работа с файловой системой
