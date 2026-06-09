# Как получить готовый EXE без возни

Сразу честно: **внутри текущего Workspace я не могу собрать настоящий Windows `.exe`**, потому что здесь нет:

- Windows toolchain
- Qt for Windows
- `cmake`
- `windeployqt`
- MinGW/MSVC cross-compiler

Но я подготовил **самый быстрый вариант**, чтобы вы получили готовый EXE почти без настройки.

## Вариант 1 — GitHub Actions, почти без усилий

В проект добавлен файл:

```text
.github/workflows/build-windows.yml
```

Что нужно сделать:

1. Загрузить папку `xray_client` в GitHub-репозиторий
2. При необходимости положить `third_party/xray/xray.exe`
3. Открыть вкладку **Actions**
4. Запустить workflow **Build Windows EXE**
5. Скачать готовый artifact **XrayQtClient-Windows-Release**

На выходе вы получите папку `release/`, где будет:

- `XrayQtClient.exe`
- Qt DLL
- platform plugins
- при наличии — `third_party/xray/xray.exe`

## Вариант 2 — локально одной командой на Windows

Я добавил файл:

```text
build_release.bat
```

Если у вас уже установлен Qt + MSVC/Qt Creator, просто запустите:

```bat
build_release.bat
```

Итоговая portable-сборка появится в папке:

```text
release/
```

## Почему я не создал EXE прямо здесь

Потому что текущая среда — не Windows и в ней отсутствуют инструменты сборки Windows/Qt. Создать настоящий рабочий `.exe` без компилятора и Qt runtime здесь невозможно.

## Если хотите — я могу следующим сообщением ещё подготовить

1. `release.zip` packaging script
2. Inno Setup `.iss` для установщика
3. автосборку релиза с версией и именованным архивом
