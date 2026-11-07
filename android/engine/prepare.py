#!/usr/bin/env python3
"""Turns a PojavLauncher checkout into the full Lullaby Launcher: Lullaby's menu on PojavLauncher's game engine.

PojavLauncher (LGPL-3.0, https://github.com/PojavLauncherTeam/PojavLauncher) brings Java for Android support,
LWJGL and the OpenGL bridge; Lullaby brings everything you see. This script is run by build-full-apk.sh on a
clean checkout of the pinned commit (engine/pojav.commit) and changes it like this:

* the app is com.sculkshoot.client, signed with Lullaby's key, so it updates the light Lullaby APK in place;
* Lullaby's sources (src/ and engine/src/), pictures, icon, theme and font are added;
* the start screen opens Lullaby's menu instead of PojavLauncher's;
* Maven Central is also read from Google's mirror (Maven Central often answers "429 Too Many Requests").
"""
import os
import re
import shutil
import sys

POJAV, ANDROID, KEYSTORE = sys.argv[1], sys.argv[2], sys.argv[3]
APP = os.path.join(POJAV, "app_pojavlauncher")
MAIN = os.path.join(APP, "src", "main")
PACKAGE = "com.sculkshoot.client"


def edit(path, *pairs, count=1):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    for old, new in pairs:
        if old not in text:
            sys.exit(f"prepare.py: '{old[:60]}' not found in {path} (did PojavLauncher change?)")
        text = text.replace(old, new, count)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


# --- Gradle: mirror, application id, signing -------------------------------------------------------------
mirror = '        maven { url "https://maven-central.storage-download.googleapis.com/maven2/" }\n'
edit(os.path.join(POJAV, "settings.gradle"),
     ("    repositories {\n        gradlePluginPortal()", "    repositories {\n" + mirror + "        gradlePluginPortal()"),
     ("    repositories {\n        google()\n        mavenCentral()", "    repositories {\n" + mirror + "        google()\n        mavenCentral()"))

gradle = os.path.join(APP, "build.gradle")
edit(gradle,
     ('applicationId "net.kdt.pojavlaunch"', f'applicationId "{PACKAGE}"'),
     ("            applicationIdSuffix '.debug'\n", ""),
     ("'net.kdt.pojavlaunch.debug'", f"'{PACKAGE}'"),
     ("'net.kdt.pojavlaunch.scoped.gamefolder.debug'", f"'{PACKAGE}.scoped.gamefolder'"),
     ("'net.kdt.pojavlaunch.scoped.controlfolder.debug'", f"'{PACKAGE}.scoped.controlfolder'"),
     ('''            storeFile file("debug.keystore")
            storePassword "android"
            keyAlias "androiddebugkey"
            keyPassword "android"''', f'''            storeFile file("{KEYSTORE}")
            storePassword "android"
            keyAlias "sculkshoot"
            keyPassword "android"'''))

# --- Lullaby's code: its R is the app's R ------------------------------------------------------------------
target = os.path.join(MAIN, "java", *PACKAGE.split("."))
os.makedirs(target, exist_ok=True)
for folder in (os.path.join(ANDROID, "src", *PACKAGE.split(".")), os.path.join(ANDROID, "engine", "src", *PACKAGE.split("."))):
    for name in os.listdir(folder):
        if not name.endswith(".java") or name == "R.java":
            continue
        with open(os.path.join(folder, name), encoding="utf-8") as f:
            code = f.read()
        if re.search(r"\bR\.", code):
            code = code.replace(f"package {PACKAGE};", f"package {PACKAGE};\n\nimport net.kdt.pojavlaunch.R;", 1)
        with open(os.path.join(target, name), "w", encoding="utf-8") as f:
            f.write(code)

# --- pictures, icon, theme, font ---------------------------------------------------------------------------
res = os.path.join(MAIN, "res")
for folder in os.listdir(os.path.join(ANDROID, "res")):
    if folder == "values":
        continue
    os.makedirs(os.path.join(res, folder), exist_ok=True)
    for name in os.listdir(os.path.join(ANDROID, "res", folder)):
        shutil.copy(os.path.join(ANDROID, "res", folder, name), os.path.join(res, folder, name))
shutil.copy(os.path.join(ANDROID, "res", "values", "styles.xml"), os.path.join(res, "values", "lullaby_styles.xml"))
for name in os.listdir(os.path.join(ANDROID, "assets")):
    shutil.copy(os.path.join(ANDROID, "assets", name), os.path.join(MAIN, "assets", name))

# --- the manifest: Lullaby's menu, its file provider, the apps whose Java it can take ---------------------
edit(os.path.join(MAIN, "AndroidManifest.xml"),
     ('android:label="@string/app_name"\n        android:process=":launcher"', 'android:label="Lullaby Launcher"\n        android:process=":launcher"'),
     ('android:roundIcon="@mipmap/ic_launcher_round"', 'android:roundIcon="@mipmap/ic_launcher"'),
     ('''            android:name=".TestStorageActivity"
            android:exported="true"
            android:label="@string/app_short_name"''', '''            android:name=".TestStorageActivity"
            android:exported="true"
            android:label="Lullaby Launcher"'''),
     ('''        <activity
            android:name=".LauncherActivity"''', f'''        <activity
            android:name="{PACKAGE}.MainActivity"
            android:exported="false"
            android:theme="@style/SculkTheme"
            android:screenOrientation="sensorLandscape"
            android:label="Lullaby Launcher"
            android:windowSoftInputMode="adjustResize"
            android:configChanges="orientation|screenSize|screenLayout|keyboardHidden" />
        <provider
            android:name="{PACKAGE}.PackProvider"
            android:authorities="{PACKAGE}.packs"
            android:exported="false"
            android:grantUriPermissions="true" />
        <activity
            android:name=".LauncherActivity"'''),
     ('    <application', '''    <queries>
        <package android:name="net.kdt.pojavlaunch" />
        <package android:name="net.kdt.pojavlaunch.debug" />
        <package android:name="org.angelauramc.amethyst" />
        <package android:name="com.tungsten.fcl" />
        <package android:name="com.movtery.zalithlauncher" />
        <intent>
            <action android:name="android.intent.action.VIEW" />
            <data android:scheme="market" />
        </intent>
        <intent>
            <action android:name="android.intent.action.SEND" />
            <data android:mimeType="*/*" />
        </intent>
    </queries>

    <application'''))

# --- the start screen opens Lullaby ------------------------------------------------------------------------
edit(os.path.join(MAIN, "java", "net", "kdt", "pojavlaunch", "TestStorageActivity.java"),
     ("Intent intent =  new Intent(this, LauncherActivity.class);",
      f'Intent intent = new Intent().setClassName(this, "{PACKAGE}.MainActivity");  // Lullaby\'s menu'))
print("prepared", POJAV)
