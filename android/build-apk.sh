#!/bin/bash
# Builds Lullaby Launcher for Android (LullabyLauncher.apk) without Gradle or the Google SDK download:
# only the Ubuntu/Debian packages aapt, android-sdk-platform-23, dalvik-exchange (dx), zipalign and apksigner.
#   sudo apt install aapt android-sdk-platform-23 dalvik-exchange zipalign apksigner openjdk-17-jdk-headless
# The app is signed with android/release.jks if it exists (KEYSTORE_PASS must hold its password),
# otherwise with a key generated once into android/build/debug.jks.
set -euo pipefail
cd "$(dirname "$0")"
export JAVA_TOOL_OPTIONS=""
ANDROID_JAR=${ANDROID_JAR:-/usr/lib/android-sdk/platforms/android-23/android.jar}
OUT=build
rm -rf "$OUT/gen" "$OUT/classes" "$OUT/apk"
mkdir -p "$OUT/gen" "$OUT/classes" "$OUT/apk"

python3 tools/gen_rules.py .. src/com/sculkshoot/client/Rules.java

aapt package -f -m -M AndroidManifest.xml -S res -A assets -I "$ANDROID_JAR" -J "$OUT/gen" -F "$OUT/apk/resources.apk"
javac -encoding UTF-8 -source 8 -target 8 -Xlint:-options -bootclasspath "$ANDROID_JAR" -d "$OUT/classes" \
    $(find src "$OUT/gen" -name '*.java')
dalvik-exchange --dex --min-sdk-version=21 --output="$OUT/apk/classes.dex" "$OUT/classes"

cp "$OUT/apk/resources.apk" "$OUT/apk/unsigned.apk"
(cd "$OUT/apk" && zip -q unsigned.apk classes.dex)
zipalign -f -p 4 "$OUT/apk/unsigned.apk" "$OUT/apk/aligned.apk"

if [ -f release.jks ]; then
    KEY=release.jks
    PASS=${KEYSTORE_PASS:?set KEYSTORE_PASS to the password of release.jks}
else
    KEY=$OUT/debug.jks
    PASS=android
    [ -f "$KEY" ] || keytool -genkeypair -keystore "$KEY" -storepass "$PASS" -keypass "$PASS" -alias sculkshoot \
        -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Lullaby Launcher" >/dev/null 2>&1
fi
apksigner sign --v4-signing-enabled false --ks "$KEY" --ks-pass "pass:$PASS" --out LullabyLauncher.apk "$OUT/apk/aligned.apk"
apksigner verify LullabyLauncher.apk
echo "Built $(pwd)/LullabyLauncher.apk ($(du -h LullabyLauncher.apk | cut -f1))"
