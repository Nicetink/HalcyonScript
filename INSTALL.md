# Установка HalcyonScript

## Требования

HalcyonScript компилируется в Windows код и требует MSYS2 с MinGW-w64.

## Установка MSYS2

### 1. Скачайте MSYS2

Перейдите на https://www.msys2.org/ и скачайте установщик.

### 2. Установите MSYS2

Запустите установщик и следуйте инструкциям. По умолчанию устанавливается в `C:\msys64`.

### 3. Установите MinGW-w64

Откройте терминал MSYS2 MinGW64 (не MSYS2 MSYS!) и выполните:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

### 4. Добавьте MinGW в PATH

Добавьте `C:\msys64\mingw64\bin` в системную переменную PATH:

1. Откройте "Параметры системы" → "Дополнительные параметры системы"
2. Нажмите "Переменные среды"
3. В "Системные переменные" найдите PATH и нажмите "Изменить"
4. Добавьте `C:\msys64\mingw64\bin`

## Сборка HalcyonScript

### Из командной строки Windows (CMD):

```cmd
cd HalcyonScript
build.bat
```

### Из MSYS2 MinGW64:

```bash
cd /path/to/HalcyonScript
./build.sh
```

### С помощью Make:

```bash
make
```

## Результат сборки

После успешной сборки появится файл `dist/halcyon.exe` — это полностью автономный исполняемый файл,
## Добавление в PATH (рекомендуется)

Чтобы HellkaiIDE и другие программы могли найти HalcyonScript, добавьте его в системный PATH:

### Автоматически (рекомендуется):

**Вариант 1: PowerShell (рекомендуется)**
```powershell
# Запустите PowerShell от имени Администратора
powershell -ExecutionPolicy Bypass -File add_to_path.ps1
```

**Вариант 2: Batch файл**
```cmd
# Запустите от имени Администратора
add_to_path.bat
```

### Вручную:

1. Откройте "Параметры системы" → "Дополнительные параметры системы"
2. Нажмите "Переменные среды"
3. В "Системные переменные" найдите PATH и нажмите "Изменить"
4. Добавьте полный путь к папке `dist`, например:
   ```
   C:\Users\YourName\Desktop\Halcyon\HalcyonScript\dist
   ```
5. Нажмите OK и перезапустите терминал/IDE

### Проверка установки:

После добавления в PATH откройте новый терминал и выполните:
```cmd
halcyon version
halcyon help
```

Если команды работают, установка прошла успешно!

## Использование

```bash
# Запуск скрипта
halcyon.exe hello.hcs

# Проверка синтаксиса
halcyon.exe check hello.hcs

# Справка
halcyon.exe help
```
