#!/bin/sh
# Builds the full Lullaby Launcher: Lullaby's menu on PojavLauncher's game engine, so "Играть" starts
# Minecraft: Java Edition inside the app (no other launcher needed).
#
# Needs: the Android SDK (platforms;android-34, build-tools;34.0.0, ndk;25.2.9519653) in ANDROID_HOME,
# JDK 21 (or 17) for Gradle, JDK 8 for PojavLauncher's LWJGL part, git, python3 and the internet
# (github.com, dl.google.com, jitpack.io, Maven Central or its Google mirror).
#   JDK8=/usr/lib/jvm/java-8-openjdk-amd64 ANDROID_HOME=~/Android/Sdk ./build-full-apk.sh
#
# Java for Android itself is not inside the APK (PojavLauncher's CI downloads it from build artifacts);
# Lullaby takes it from an installed PojavLauncher or from a file the first time "Играть" needs it.
set -e
cd "$(dirname "$0")"
HERE=$(pwd)
: "${ANDROID_HOME:?set ANDROID_HOME to the Android SDK}"
JDK8=${JDK8:-/usr/lib/jvm/java-8-openjdk-amd64}
SRC=$HERE/build/engine-src
COMMIT=$(cat engine/pojav.commit)

mkdir -p build
KEY=$HERE/build/debug.jks  # the same key as build-apk.sh: the full APK updates the light one
[ -f "$KEY" ] || keytool -genkeypair -keystore "$KEY" -storepass android -keypass android -alias sculkshoot \
    -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Lullaby Launcher" >/dev/null 2>&1

if [ ! -d "$SRC/.git" ]; then
    git clone https://github.com/PojavLauncherTeam/PojavLauncher.git "$SRC"
fi
cd "$SRC"
git fetch --depth 1 origin "$COMMIT" 2>/dev/null || true
git checkout -q -f "$COMMIT"
git reset -q --hard
git clean -q -fdx -e local.properties -e .gradle
echo "sdk.dir=$ANDROID_HOME" > local.properties
python3 "$HERE/engine/prepare.py" "$SRC" "$HERE" "$KEY"

# LTW (LGPL-3.0, github.com/PojavLauncherTeam/LTW): OpenGL 3 on OpenGL ES 3 for Minecraft 1.17+, which
# PojavLauncher's own CI adds the same way
mkdir -p app_pojavlauncher/libs
curl -fsSL -o app_pojavlauncher/libs/ltw-release.aar https://github.com/PojavLauncherTeam/LTW/releases/latest/download/ltw-release.aar \
    || echo "warning: LTW not downloaded; the game uses GL4ES or Zink"

./gradlew --no-daemon -Porg.gradle.java.installations.paths="$JDK8" -Porg.gradle.java.installations.auto-detect=false \
    :app_pojavlauncher:assembleDebug -x lint
cp app_pojavlauncher/build/outputs/apk/debug/app_pojavlauncher-debug.apk "$HERE/VelloraClient-full.apk"
echo "Built $HERE/VelloraClient-full.apk ($(du -h "$HERE/VelloraClient-full.apk" | cut -f1))"
