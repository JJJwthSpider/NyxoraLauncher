@echo off
chcp 65001 >nul
setlocal EnableExtensions EnableDelayedExpansion
title Сборка Vellora Client

rem ======================================================================
rem  Vellora Client: сборка для Windows одним файлом.
rem  Ставит недостающие программы (winget), скачивает Qt и библиотеки,
rem  собирает лаунчер и кладёт ярлык на рабочий стол.
rem ======================================================================

set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
set "WORK=C:\Vellora"
set "QT_VERSION=6.8.3"
set "QTDIR=%WORK%\Qt\%QT_VERSION%\msvc2022_64"
set "OUT=%WORK%\Vellora Client"
set "LOG=%WORK%\build-log.txt"

echo.
echo  ================================================================
echo    Vellora Client - сборка для Windows
echo  ================================================================
echo   Скрипт сам скачает всё нужное и соберёт лаунчер.
echo   Нужно около 10 ГБ свободного места на диске C: и интернет.
echo   Первый раз это займёт 30-90 минут, потом - несколько минут.
echo   Готовый лаунчер появится в папке:  %OUT%
echo   и ярлык "Vellora Client" на рабочем столе.
echo.
pause

rem ---------- 0. исходный код должен лежать рядом с этим файлом ----------
rem  Запуск прямо из zip-архива: Windows достаёт во временную папку один этот файл,
rem  а исходников рядом нет. Тогда не копируем ничего, а объясняем, что делать.
if exist "%SRC%\CMakePresets.json" if exist "%SRC%\launcher\" goto :have_sources
echo.
echo  [x] Рядом с build-windows.bat нет исходного кода - нет файла CMakePresets.json.
echo      Скорее всего, этот файл запущен прямо из zip-архива.
echo.
echo      Что сделать:
echo        1. Правой кнопкой по архиву VelloraClient-src.zip - «Извлечь все...» - «Извлечь».
echo        2. Открой распакованную папку и запусти build-windows.bat уже из неё.
echo.
echo      Если нужно просто играть, собирать ничего не надо: распакуй
echo      VelloraClient-Windows.zip и запусти vellora.exe - это готовый лаунчер.
echo.
echo      Сейчас файл запущен из папки: %SRC%
goto :fail
:have_sources

if not exist "%WORK%" mkdir "%WORK%"

where winget >nul 2>&1
if errorlevel 1 (
    echo.
    echo  [x] Не найден winget. Обнови Windows или установи "Установщик приложений"
    echo      из Microsoft Store, потом запусти этот файл снова.
    goto :fail
)

rem ---------- 1. программы ----------
echo.
echo  [1/6] Проверяю программы...
set "INSTALLED="
call :ensure "git --version" Git.Git
call :ensure "cmake --version" Kitware.CMake
call :ensure "ninja --version" Ninja-build.Ninja
call :ensure "py -3 -c 0" Python.Python.3.12
call :ensure "java -version" EclipseAdoptium.Temurin.17.JDK

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
)
if not defined VSPATH (
    echo  Устанавливаю компилятор C++ - Visual Studio Build Tools. Это самая долгая часть, ждите...
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    set "INSTALLED=1"
)
if defined INSTALLED (
    echo.
    echo  ================================================================
    echo   Нужные программы установлены.
    echo   ЗАКРОЙ это окно и запусти build-windows.bat ЕЩЁ РАЗ -
    echo   Windows должна увидеть новые программы.
    echo  ================================================================
    pause
    exit /b 0
)

rem ---------- 2. исходники в папку без русских букв ----------
echo.
echo  [2/6] Копирую исходный код в %WORK%\src ...
if /i not "%SRC%"=="%WORK%\src" (
    robocopy "%SRC%" "%WORK%\src" /MIR /XD build .git /NFL /NDL /NJH /NJS /NP >nul
    if errorlevel 8 (
        echo  [x] Не удалось скопировать исходный код.
        goto :fail
    )
)

rem ---------- 3. Qt ----------
echo.
echo  [3/6] Qt %QT_VERSION% ...
if not exist "%QTDIR%\bin\Qt6Core.dll" (
    py -3 -m pip install --upgrade --quiet aqtinstall
    if errorlevel 1 goto :fail
    py -3 -m aqt install-qt windows desktop %QT_VERSION% win64_msvc2022_64 -m qtnetworkauth qtimageformats -O "%WORK%\Qt"
    if errorlevel 1 goto :fail
) else (
    echo  уже скачан
)

rem ---------- 4. vcpkg - остальные библиотеки ----------
echo.
echo  [4/6] vcpkg ...
if not exist "%WORK%\vcpkg\vcpkg.exe" (
    if not exist "%WORK%\vcpkg\.git" git clone https://github.com/microsoft/vcpkg "%WORK%\vcpkg"
    if errorlevel 1 goto :fail
    call "%WORK%\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
    if errorlevel 1 goto :fail
) else (
    echo  уже есть
)
set "VCPKG_ROOT=%WORK%\vcpkg"

if not defined JAVA_HOME (
    for /d %%j in ("%ProgramFiles%\Eclipse Adoptium\jdk-17*") do set "JAVA_HOME=%%j"
)

rem ---------- 5. сборка ----------
echo.
echo  [5/6] Собираю лаунчер. В первый раз vcpkg собирает библиотеки - это долго.
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :fail
cd /d "%WORK%\src"
cmake --preset windows_msvc -DCMAKE_PREFIX_PATH="%QTDIR%" -DLauncher_WARNINGS_AS_ERRORS=OFF -DENABLE_LTO=OFF > "%LOG%" 2>&1
if errorlevel 1 (
    type "%LOG%"
    goto :fail
)
cmake --build --preset windows_msvc --config Release >> "%LOG%" 2>&1
if errorlevel 1 (
    powershell -NoProfile -Command "Get-Content -Tail 60 '%LOG%'"
    goto :fail
)

rem ---------- 6. готовая папка и ярлык ----------
echo.
echo  [6/6] Собираю готовую папку с лаунчером...
cmake --install build --config Release --prefix "%OUT%" >> "%LOG%" 2>&1
if errorlevel 1 (
    powershell -NoProfile -Command "Get-Content -Tail 60 '%LOG%'"
    goto :fail
)
powershell -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut([Environment]::GetFolderPath('Desktop')+'\Vellora Client.lnk'); $s.TargetPath='%OUT%\vellora.exe'; $s.WorkingDirectory='%OUT%'; $s.Save()"

echo.
echo  ================================================================
echo   ГОТОВО. Лаунчер собран:  %OUT%\vellora.exe
echo   На рабочем столе есть ярлык "Vellora Client".
echo  ================================================================
explorer "%OUT%"
pause
exit /b 0

rem ---------- помощники ----------
:ensure
%~1 >nul 2>&1
if not errorlevel 1 exit /b 0
echo  Устанавливаю %~2 ...
winget install --id %~2 -e --accept-package-agreements --accept-source-agreements --silent
set "INSTALLED=1"
exit /b 0

:fail
echo.
echo  ================================================================
echo   Что-то пошло не так. Сделай скриншот этого окна и пришли его.
echo   Подробный журнал сборки:  %LOG%
echo  ================================================================
pause
exit /b 1
