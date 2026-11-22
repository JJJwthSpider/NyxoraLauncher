#!/bin/bash
# Собирает Nyxora для iPhone (NyxoraLauncher-iPhone.ipa) на Mac одной командой.
# Запуск из папки с исходниками Nyxora (распакованный NyxoraLauncher-src.zip):
#     bash ios/build-on-mac.sh
# Нужны Xcode (из App Store, открыть один раз) и Homebrew (https://brew.sh); остальное скрипт ставит сам:
# cmake, ldid, GNU make, JDK 8. Потом он берёт Amethyst (ios/prepare.py встраивает в него Nyxora) и собирает его,
# как сборка «iPhone» на GitHub (.github/workflows/ios.yml). Готовый файл кладётся на Рабочий стол, а весь вывод —
# в ~/Desktop/nyxora-iphone-build.log (если что-то сломается, пришли этот файл).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$HOME/nyxora-ios"
OUT="$HOME/Desktop/NyxoraLauncher-iPhone.ipa"
LOG="$HOME/Desktop/nyxora-iphone-build.log"

step() { printf '\n\033[1;35m== %s\033[0m\n' "$*"; }
fail() { printf '\n\033[1;31mНе получилось: %s\033[0m\n' "$*" >&2; exit 1; }
trap 'printf "\n\033[1;31mСборка остановилась с ошибкой. Весь вывод: %s\033[0m\n" "$LOG" >&2' ERR

[ "$(uname)" = "Darwin" ] || fail "этот скрипт запускается только на Mac."
[ -f "$ROOT/ios/prepare.py" ] || fail "не нашёл ios/prepare.py: запусти скрипт из папки с исходниками Nyxora."
mkdir -p "$HOME/Desktop"
exec > >(tee "$LOG") 2>&1

step "Проверяю Xcode"
if ! SDK="$(xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)"; then
    fail "нужен Xcode. Установи его из App Store, открой один раз (он попросит принять лицензию) и запусти скрипт снова.
Если Xcode уже стоит, выполни: sudo xcode-select -s /Applications/Xcode.app"
fi
echo "SDK iPhone: $SDK"

step "Проверяю Homebrew"
for brew in /opt/homebrew/bin/brew /usr/local/bin/brew; do
    if [ -x "$brew" ]; then eval "$("$brew" shellenv)"; fi
done
command -v brew >/dev/null || fail "нужен Homebrew. Открой https://brew.sh, вставь команду оттуда в Терминал, дождись конца
и запусти этот скрипт снова."

step "Ставлю инструменты: cmake, ldid, GNU make"
brew install cmake ldid make
GNUBIN="$(brew --prefix make)/libexec/gnubin"
export PATH="$GNUBIN:$PATH"

step "Проверяю Java 8 (нужна для сборки Java-части Amethyst)"
if ! JDK8="$(/usr/libexec/java_home -v 1.8 2>/dev/null)"; then
    echo "Ставлю JDK 8 (Zulu); macOS может спросить пароль от Mac."
    brew install --cask zulu@8
    JDK8="$(/usr/libexec/java_home -v 1.8)"
fi
echo "JDK 8: $JDK8"

step "Pillow для иконок (не обязательно)"
python3 -m pip install --quiet --user pillow 2>/dev/null || echo "Pillow не поставился: маленькие иконки останутся от Amethyst."

step "Беру Amethyst и встраиваю Nyxora (меню, гардероб, образ в игре)"
python3 "$ROOT/ios/prepare.py" "$WORK"

step "Собираю приложение (это долго: 15–40 минут)"
cd "$WORK"
rm -rf artifacts
gmake -j"$(sysctl -n hw.ncpu)" dsym package PLATFORM=2 BOOTJDK="$JDK8/bin" SDKPATH="$SDK"

IPA=""
for f in artifacts/*.ipa; do
    case "$f" in *slimmed*) ;; *) IPA="$f" ;; esac
done
[ -n "$IPA" ] || fail "сборка закончилась, но файла .ipa нет в $WORK/artifacts."
cp "$IPA" "$OUT"

step "Готово"
echo "Файл: $OUT"
echo "Поставь его на iPhone через SideStore, как написано в HOW-TO-IPHONE-RU.txt."
