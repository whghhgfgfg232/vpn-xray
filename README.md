# vpn-xray
Как получить готовый EXE без возни
Сразу честно: внутри текущего Workspace я не могу собрать настоящий Windows .exe, потому что здесь нет:

Windows toolchain
Qt for Windows
cmake
windeployqt
MinGW/MSVC cross-compiler
Но я подготовил самый быстрый вариант, чтобы вы получили готовый EXE почти без настройки.

Вариант 1 — GitHub Actions, почти без усилий
В проект добавлен файл:

text

.github/workflows/build-windows.yml
Что нужно сделать:

Загрузить папку xray_client в GitHub-репозиторий
При необходимости положить third_party/xray/xray.exe
Открыть вкладку Actions
Запустить workflow Build Windows EXE
Скачать готовый artifact XrayQtClient-Windows-Release
На выходе вы получите папку release/, где будет:

XrayQtClient.exe
Qt DLL
platform plugins
при наличии — third_party/xray/xray.exe
Вариант 2 — локально одной командой на Windows
Я добавил файл:

text

build_release.bat
Если у вас уже установлен Qt + MSVC/Qt Creator, просто запустите:

bat

build_release.bat
Итоговая portable-сборка появится в папке:

text

release/
Почему я не создал EXE прямо здесь
Потому что текущая среда — не Windows и в ней отсутствуют инструменты сборки Windows/Qt. Создать настоящий рабочий .exe без компилятора и Qt runtime здесь невозможно.

Если хотите — я могу следующим сообщением ещё подготовить
release.zip packaging script
Inno Setup .iss для установщика
автосборку релиза с версией и именованным архивом
