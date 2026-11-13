#!/bin/sh
# Builds the Nyxora Cosmetics mod (cosmetics-mod/, Fabric, Minecraft 1.20.1) and puts the jar where the launchers take
# it from: launcher/resources/vellora_mods/ (PC) and android/assets/ (phone).
# Needs: JDK 17 or newer, Gradle 8.13 (or the GRADLE variable), the internet (maven.fabricmc.net, Mojang's servers,
# Maven Central or its Google mirror).
#   tools/build-cosmetics-mod.sh
set -e
cd "$(dirname "$0")/.."
ROOT=$(pwd)
GRADLE=${GRADLE:-gradle}
(cd cosmetics-mod && "$GRADLE" --no-daemon build)
JAR=$(ls cosmetics-mod/build/libs/vellora-cosmetics-1.20.1-*.jar | grep -v sources | head -1)
cp "$JAR" launcher/resources/vellora_mods/vellora-cosmetics-1.20.1.jar
cp "$JAR" android/assets/vellora-cosmetics-1.20.1.jar
echo "Built $JAR -> launcher/resources/vellora_mods/ and android/assets/"
