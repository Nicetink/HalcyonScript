# HalcyonScript Audio API

API для воспроизведения аудио файлов в HalcyonScript.

## Поддерживаемые форматы

- MP3
- WAV
- WMA
- MIDI
- AIF/AIFF
- AU

## Функции

### Audio.create(name)
Создаёт новый аудио плеер.

```hcs
Audio.create("player")
```

### Audio.load(name, filePath)
Загружает аудио файл.

```hcs
Audio.load("player", "C:/Music/song.mp3")
```

### Audio.play(name)
Начинает воспроизведение.

```hcs
Audio.play("player")
```

### Audio.pause(name)
Ставит на паузу.

```hcs
Audio.pause("player")
```

### Audio.resume(name)
Продолжает воспроизведение после паузы.

```hcs
Audio.resume("player")
```

### Audio.stop(name)
Останавливает воспроизведение и сбрасывает позицию.

```hcs
Audio.stop("player")
```

### Audio.seek(name, positionMs)
Перематывает на указанную позицию (в миллисекундах).

```hcs
Audio.seek("player", 30000)  # Перемотать на 30 секунд
```

### Audio.setVolume(name, volume)
Устанавливает громкость (0-100).

```hcs
Audio.setVolume("player", 75)
```

### Audio.getVolume(name)
Возвращает текущую громкость.

```hcs
var vol = Audio.getVolume("player")
```

### Audio.getPosition(name)
Возвращает текущую позицию в миллисекундах.

```hcs
var pos = Audio.getPosition("player")
```

### Audio.getDuration(name)
Возвращает длительность трека в миллисекундах.

```hcs
var duration = Audio.getDuration("player")
```

### Audio.getState(name)
Возвращает текущее состояние: "playing", "paused", или "stopped".

```hcs
var state = Audio.getState("player")
if state == "playing"
    print("Playing!")
end
```

### Audio.setLoop(name, loop)
Включает/выключает повтор.

```hcs
Audio.setLoop("player", true)
```

### Audio.setMute(name, mute)
Включает/выключает звук.

```hcs
Audio.setMute("player", true)
```

### Audio.isMuted(name)
Проверяет, выключен ли звук.

```hcs
var muted = Audio.isMuted("player")
```

### Audio.destroy(name)
Уничтожает плеер и освобождает ресурсы.

```hcs
Audio.destroy("player")
```

## Пример: Простой плеер

```hcs
HalGUI.init()
HalGUI.setTheme("dark")

create window app "Music Player" 400 200

create button btnPlay "Play" x:20 y:20 width:80 height:40
create button btnPause "Pause" x:110 y:20 width:80 height:40
create button btnStop "Stop" x:200 y:20 width:80 height:40

create slider sliderVolume x:20 y:80 width:360 height:30 min:0 max:100

# Создаём плеер и загружаем файл
Audio.create("music")
Audio.load("music", "C:/Music/song.mp3")

on btnPlay click
    Audio.play("music")
end

on btnPause click
    Audio.pause("music")
end

on btnStop click
    Audio.stop("music")
end

on sliderVolume change
    Audio.setVolume("music", sliderVolume.value)
end

set sliderVolume.value = 100

HalGUI.run()
```

## Пример: Плеер с прогрессом

```hcs
# Обновление прогресса каждую секунду
func updateProgress()
    var pos = Audio.getPosition("player")
    var dur = Audio.getDuration("player")
    if dur > 0
        var progress = (pos * 100) / dur
        set sliderProgress.value = progress
    end
end

# Форматирование времени
func formatTime(ms)
    var seconds = ms / 1000
    var mins = seconds / 60
    var secs = seconds % 60
    return mins + ":" + secs
end
```

## Примечания

- Плеер использует Windows MCI (Media Control Interface)
- Можно создать несколько плееров с разными именами
- Файлы загружаются асинхронно
- Для MP3 требуется установленный кодек (обычно есть в Windows)
