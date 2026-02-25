# DVD/CD Optical Drive API

API для работы с DVD/CD приводами в HalcyonScript.

## Обзор

DVD API предоставляет функции для работы с оптическими приводами, включая:
- Обнаружение DVD/CD приводов
- Проверка наличия диска
- Получение информации о диске
- Управление лотком (открытие/закрытие)
- Чтение файлов с диска
- Копирование файлов с диска
- Получение возможностей привода

## Функции

### DVD.getDrives()

Возвращает список всех DVD/CD приводов в системе.

**Возвращает:** `Array` - Массив объектов с информацией о приводах

**Пример:**
```hcs
let drives = DVD.getDrives();
for (let i = 0; i < drives.length; i++) {
    Console.writeln("Drive: " + drives[i].letter + " - " + drives[i].label);
}
// Output: Drive: D:\ - DVD_ROM
```

**Структура объекта привода:**
- `letter` (string) - Буква привода (например, "D:\\")
- `label` (string) - Метка тома

---

### DVD.isDiscPresent(drive)

Проверяет, вставлен ли диск в указанный привод.

**Параметры:**
- `drive` (string) - Буква привода ("D" или "D:\\")

**Возвращает:** `boolean` - true, если диск присутствует

**Пример:**
```hcs
if (DVD.isDiscPresent("D")) {
    Console.writeln("Диск найден в приводе D:");
} else {
    Console.writeln("Диск не найден");
}
```

---

### DVD.getDiscInfo(drive)

Получает подробную информацию о диске в приводе.

**Параметры:**
- `drive` (string) - Буква привода

**Возвращает:** `Object` - Объект с информацией о диске или null, если диск отсутствует

**Пример:**
```hcs
let info = DVD.getDiscInfo("D");
if (info != null) {
    Console.writeln("Метка: " + info.label);
    Console.writeln("Тип: " + info.type);
    Console.writeln("Размер: " + info.totalSize + " МБ");
    Console.writeln("Свободно: " + info.freeSpace + " МБ");
    Console.writeln("Файловая система: " + info.fileSystem);
}
```

**Структура объекта информации о диске:**
- `label` (string) - Метка диска
- `type` (string) - Тип диска ("CD", "DVD", "Blu-ray", "Other")
- `fileSystem` (string) - Файловая система (CDFS, UDF, и т.д.)
- `serialNumber` (number) - Серийный номер диска
- `totalSize` (number) - Общий размер в МБ
- `freeSpace` (number) - Свободное место в МБ
- `usedSpace` (number) - Использованное место в МБ

---

### DVD.eject(drive)

Извлекает диск из привода (открывает лоток).

**Параметры:**
- `drive` (string) - Буква привода

**Возвращает:** `boolean` - true при успешном извлечении

**Пример:**
```hcs
if (DVD.eject("D")) {
    Console.writeln("Диск извлечен");
} else {
    Console.writeln("Ошибка извлечения диска");
}
```

---

### DVD.closeTray(drive)

Закрывает лоток привода.

**Параметры:**
- `drive` (string) - Буква привода

**Возвращает:** `boolean` - true при успешном закрытии

**Пример:**
```hcs
if (DVD.closeTray("D")) {
    Console.writeln("Лоток закрыт");
} else {
    Console.writeln("Ошибка закрытия лотка");
}
```

---

### DVD.readFiles(drive, [path])

Читает список файлов и папок на диске.

**Параметры:**
- `drive` (string) - Буква привода
- `path` (string, опционально) - Путь к папке на диске (по умолчанию корень)

**Возвращает:** `Array` - Массив объектов с информацией о файлах

**Пример:**
```hcs
let files = DVD.readFiles("D");
for (let i = 0; i < files.length; i++) {
    let file = files[i];
    if (file.isDirectory) {
        Console.writeln("[DIR] " + file.name);
    } else {
        Console.writeln(file.name + " (" + file.size + " МБ)");
    }
}

// Чтение конкретной папки
let videoFiles = DVD.readFiles("D", "VIDEO_TS");
```

**Структура объекта файла:**
- `name` (string) - Имя файла или папки
- `isDirectory` (boolean) - true, если это папка
- `size` (number) - Размер файла в МБ (только для файлов)
- `modified` (string) - Дата изменения в формате "YYYY-MM-DD HH:MM:SS"

---

### DVD.copyFile(sourceDrive, sourceFile, destPath)

Копирует файл с диска на жесткий диск.

**Параметры:**
- `sourceDrive` (string) - Буква привода-источника
- `sourceFile` (string) - Путь к файлу на диске
- `destPath` (string) - Путь назначения на жестком диске

**Возвращает:** `boolean` - true при успешном копировании

**Пример:**
```hcs
// Копирование одного файла
if (DVD.copyFile("D", "README.TXT", "C:\\temp\\readme.txt")) {
    Console.writeln("Файл скопирован");
}

// Копирование файла из подпапки
if (DVD.copyFile("D", "DOCS\\manual.pdf", "C:\\Documents\\manual.pdf")) {
    Console.writeln("Руководство скопировано");
}
```

---

### DVD.getCapabilities(drive)

Получает информацию о возможностях привода.

**Параметры:**
- `drive` (string) - Буква привода

**Возвращает:** `Object` - Объект с возможностями привода

**Пример:**
```hcs
let caps = DVD.getCapabilities("D");
if (caps != null) {
    Console.writeln("Чтение: " + (caps.canRead ? "Да" : "Нет"));
    Console.writeln("Запись: " + (caps.canWrite ? "Да" : "Нет"));
    Console.writeln("Извлечение: " + (caps.canEject ? "Да" : "Нет"));
}
```

**Структура объекта возможностей:**
- `canRead` (boolean) - Поддержка чтения
- `canWrite` (boolean) - Поддержка записи
- `canEject` (boolean) - Поддержка извлечения

---

### DVD.writeFile(sourcePath, drive, destPath)

Записывает файл на диск.

**Параметры:**
- `sourcePath` (string) - Путь к исходному файлу на жестком диске
- `drive` (string) - Буква привода
- `destPath` (string) - Путь назначения на диске

**Возвращает:** `boolean` - true при успешной записи

**Пример:**
```hcs
if DVD.writeFile("C:\\temp\\document.txt", "D", "backup\\document.txt") {
    Console.writeln("File written successfully")
} else {
    Console.writeln("Write failed")
}
```

---

### DVD.createDirectory(drive, dirPath)

Создает папку на диске.

**Параметры:**
- `drive` (string) - Буква привода
- `dirPath` (string) - Путь к создаваемой папке

**Возвращает:** `boolean` - true при успешном создании

**Пример:**
```hcs
if DVD.createDirectory("D", "backup\\documents") {
    Console.writeln("Directory created")
}
```

---

### DVD.verifyFile(originalPath, drive, discPath)

Проверяет целостность файла на диске путем сравнения с оригиналом.

**Параметры:**
- `originalPath` (string) - Путь к оригинальному файлу
- `drive` (string) - Буква привода
- `discPath` (string) - Путь к файлу на диске

**Возвращает:** `boolean` - true если файлы идентичны

**Пример:**
```hcs
if DVD.verifyFile("C:\\temp\\document.txt", "D", "document.txt") {
    Console.writeln("File integrity verified - files are identical")
} else {
    Console.writeln("File verification failed - files differ")
}
```

---

### DVD.getWriteCapacity(drive)

Получает информацию о доступном месте для записи на диске.

**Параметры:**
- `drive` (string) - Буква привода

**Возвращает:** `Object` - Объект с информацией о месте на диске

**Пример:**
```hcs
let capacity = DVD.getWriteCapacity("D")
if capacity != null {
    Console.writeln("Total: " + capacity.total + " MB")
    Console.writeln("Free: " + capacity.free + " MB")
    Console.writeln("Used: " + capacity.used + " MB")
}
```

**Структура объекта емкости:**
- `total` (number) - Общий размер в МБ
- `free` (number) - Свободное место в МБ
- `used` (number) - Использованное место в МБ

## Примеры использования

### Простой просмотрщик дисков

```hcs
Console.init();
Console.writeln("=== Просмотрщик DVD/CD дисков ===");

let drives = DVD.getDrives();
if (drives.length == 0) {
    Console.writeln("DVD/CD приводы не найдены");
    return;
}

for (let i = 0; i < drives.length; i++) {
    let drive = drives[i];
    Console.writeln("Привод: " + drive.letter);
    
    if (DVD.isDiscPresent(drive.letter)) {
        let info = DVD.getDiscInfo(drive.letter);
        Console.writeln("  Диск: " + info.label + " (" + info.type + ")");
        Console.writeln("  Размер: " + info.totalSize.toFixed(0) + " МБ");
        
        let files = DVD.readFiles(drive.letter);
        Console.writeln("  Файлов: " + files.length);
    } else {
        Console.writeln("  Диск отсутствует");
    }
}
```

### Копирование всех файлов с диска

```hcs
function copyAllFiles(drive, destFolder) {
    if (!DVD.isDiscPresent(drive)) {
        Console.writeln("Диск не найден в приводе " + drive);
        return false;
    }
    
    let files = DVD.readFiles(drive);
    let copied = 0;
    
    for (let i = 0; i < files.length; i++) {
        let file = files[i];
        if (!file.isDirectory) {
            let destPath = destFolder + "\\" + file.name;
            if (DVD.copyFile(drive, file.name, destPath)) {
                Console.writeln("Скопирован: " + file.name);
                copied++;
            } else {
                Console.writeln("Ошибка копирования: " + file.name);
            }
        }
    }
    
    Console.writeln("Скопировано файлов: " + copied);
    return true;
}

// Использование
copyAllFiles("D", "C:\\BackupDVD");
```

### Автоматическое извлечение диска после копирования

```hcs
function backupAndEject(drive) {
    if (!DVD.isDiscPresent(drive)) {
        Console.writeln("Вставьте диск в привод " + drive);
        return;
    }
    
    let info = DVD.getDiscInfo(drive);
    Console.writeln("Создание резервной копии: " + info.label);
    
    // Создаем папку с именем диска
    let backupFolder = "C:\\DVDBackup\\" + info.label;
    Dir.create(backupFolder);
    
    // Копируем все файлы
    let files = DVD.readFiles(drive);
    for (let i = 0; i < files.length; i++) {
        let file = files[i];
        if (!file.isDirectory) {
            DVD.copyFile(drive, file.name, backupFolder + "\\" + file.name);
        }
    }
    
    Console.writeln("Резервное копирование завершено");
    
    // Извлекаем диск
    if (DVD.eject(drive)) {
        Console.writeln("Диск извлечен");
    }
}
```

## Примечания

- Все функции работают только с оптическими приводами (DVD/CD/Blu-ray)
- Для работы с функциями записи требуются соответствующие права доступа
- Некоторые функции могут не работать с защищенными дисками
- Рекомендуется проверять наличие диска перед выполнением операций
- Размеры файлов возвращаются в мегабайтах для удобства