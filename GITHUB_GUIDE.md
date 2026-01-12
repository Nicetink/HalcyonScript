# Инструкция: Публикация HalcyonScript на GitHub

## Шаг 1: Создание репозитория на GitHub

1. Откройте https://github.com
2. Войдите в аккаунт (или создайте новый)
3. Нажмите зеленую кнопку "New" (или перейдите на https://github.com/new)
4. Заполните форму:
   - Repository name: `HalcyonScript`
   - Description: `Native programming language for Windows desktop applications`
   - Выберите: Public
   - НЕ ставьте галочки на "Add a README file", "Add .gitignore", "Choose a license" (они уже есть в проекте)
5. Нажмите "Create repository"

## Шаг 2: Настройка Git на компьютере

Откройте PowerShell и выполните:

```powershell
# Перейдите в папку проекта
cd C:\Users\oem\Desktop\Halcyon\HalcyonScript-Native

# Настройте имя и email (замените на свои данные)
& "C:\Program Files\Git\bin\git.exe" config user.name "KAInaps"
& "C:\Program Files\Git\bin\git.exe" config user.email "your-email@example.com"
```

## Шаг 3: Инициализация репозитория

```powershell
# Инициализация (если еще не сделано)
& "C:\Program Files\Git\bin\git.exe" init

# Добавление всех файлов
& "C:\Program Files\Git\bin\git.exe" add .

# Проверка статуса
& "C:\Program Files\Git\bin\git.exe" status

# Создание первого коммита
& "C:\Program Files\Git\bin\git.exe" commit -m "Initial commit: HalcyonScript programming language"
```

## Шаг 4: Подключение к GitHub

```powershell
# Добавьте удаленный репозиторий (замените YOUR_USERNAME на ваш логин GitHub)
& "C:\Program Files\Git\bin\git.exe" remote add origin https://github.com/YOUR_USERNAME/HalcyonScript.git

# Переименуйте ветку в main (если нужно)
& "C:\Program Files\Git\bin\git.exe" branch -M main
```

## Шаг 5: Публикация на GitHub

```powershell
# Отправка кода на GitHub
& "C:\Program Files\Git\bin\git.exe" push -u origin main
```

При первой отправке GitHub запросит авторизацию:
- Откроется окно браузера для входа в GitHub
- Или введите логин/пароль в терминале
- Для пароля используйте Personal Access Token (не обычный пароль)

## Создание Personal Access Token

Если GitHub требует токен вместо пароля:

1. Откройте https://github.com/settings/tokens
2. Нажмите "Generate new token" → "Generate new token (classic)"
3. Введите название: `HalcyonScript`
4. Выберите срок действия
5. Отметьте галочки: `repo` (полный доступ к репозиториям)
6. Нажмите "Generate token"
7. Скопируйте токен (он показывается только один раз!)
8. Используйте этот токен вместо пароля при push

## Шаг 6: Проверка

Откройте в браузере:
```
https://github.com/YOUR_USERNAME/HalcyonScript
```

Вы должны увидеть все файлы проекта и README.md на главной странице.

## Обновление репозитория

После внесения изменений:

```powershell
cd C:\Users\oem\Desktop\Halcyon\HalcyonScript-Native

# Добавить изменения
& "C:\Program Files\Git\bin\git.exe" add .

# Создать коммит
& "C:\Program Files\Git\bin\git.exe" commit -m "Описание изменений"

# Отправить на GitHub
& "C:\Program Files\Git\bin\git.exe" push
```

## Быстрый скрипт для публикации

Создайте файл `publish.bat` в папке проекта:

```batch
@echo off
cd /d "%~dp0"
set GIT="C:\Program Files\Git\bin\git.exe"

echo Adding files...
%GIT% add .

set /p MSG="Commit message: "
%GIT% commit -m "%MSG%"

echo Pushing to GitHub...
%GIT% push

echo Done!
pause
```

## Структура репозитория

После публикации репозиторий будет содержать:

```
HalcyonScript/
├── README.md           # Описание проекта
├── LICENSE             # MIT лицензия
├── .gitignore          # Игнорируемые файлы
├── build.bat           # Скрипт сборки
├── build_and_install.bat
├── src/                # Исходный код
│   ├── main.c
│   ├── lexer.c
│   ├── parser.c
│   ├── runtime.c
│   └── halgui/         # GUI фреймворк
├── examples/           # Примеры программ
├── docs/               # Документация
└── logo/               # Логотип
```

## Добавление Release

После публикации можно создать релиз с готовым exe:

1. На странице репозитория нажмите "Releases" (справа)
2. Нажмите "Create a new release"
3. Tag version: `v1.0.0`
4. Release title: `HalcyonScript v1.0.0`
5. Опишите изменения
6. Прикрепите файл `halcyon.exe` (перетащите в поле)
7. Нажмите "Publish release"

## Полезные команды Git

```powershell
# Просмотр истории коммитов
& "C:\Program Files\Git\bin\git.exe" log --oneline

# Просмотр изменений
& "C:\Program Files\Git\bin\git.exe" diff

# Отмена изменений в файле
& "C:\Program Files\Git\bin\git.exe" checkout -- filename

# Просмотр удаленных репозиториев
& "C:\Program Files\Git\bin\git.exe" remote -v
```
