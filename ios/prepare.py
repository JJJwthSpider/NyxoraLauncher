#!/usr/bin/env python3
"""Prepares the iPhone app of Nyxora Launcher: Amethyst (github.com/AngelAuraMC/Amethyst-iOS, GPL-3.0), the Minecraft:
Java Edition launcher for iOS, at the commit in ios/amethyst.commit, with Nyxora's name and icon.

Amethyst is to the iPhone what PojavLauncher is to the full Android build (android/engine/): the game engine. Like
Nyxora on the PC and on Android, it signs in only with a Microsoft account; an account without the game plays the
official demo (accounts "Demo.<name>"), so the full game is never playable without buying it.

What changes: the name on the home screen and in the texts ("Nyxora" for "Amethyst"; the version line in the settings
still says Angel Aura Amethyst, whose work it is), the app icons and the logo (ios/art/, made by ios/make_icons.py).
The bundle id stays Amethyst's, so JIT helpers (StikDebug's scripts) and GetMoreRam treat it as Amethyst.

And Nyxora itself, on the first page of the app ("Nyxora" in the menu, before Amethyst's news and profiles): the
lobby with the player in 3D, the wardrobe with every item and outfit, the neon rain, the sounds and the music
(ios/nyxora/, put together by ios/build_web.py), and the look in the game: before every start of a Fabric or Quilt
1.20.1 profile the Nyxora Cosmetics mod and the look go into its folder, as on the PC and on Android
(ios/native/: NyxoraHomeViewController shows the menu and answers it, NyxoraLook writes the look).

    python3 ios/prepare.py <folder>          clones Amethyst into <folder> and applies the changes
Then, on a Mac with Xcode, JDK 8, cmake, ldid and GNU make (what .github/workflows/ios.yml does on GitHub's Macs):
    cd <folder> && gmake -j8 dsym package PLATFORM=2 SLIMMED=1
The .ipa lands in <folder>/artifacts/. HOW-TO-IPHONE-RU.txt says how to put it on an iPhone.
"""
import pathlib
import plistlib
import re
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
REPO = "https://github.com/AngelAuraMC/Amethyst-iOS"
NAME = "Nyxora"


def run(*args, cwd=None):
    print("+", " ".join(args), flush=True)
    subprocess.run(args, cwd=cwd, check=True)


def fetch(dest, commit):
    if (dest / ".git").is_dir():
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=dest, capture_output=True, text=True).stdout.strip()
        if head == commit:
            run("git", "checkout", "--", ".", cwd=dest)  # a fresh copy of the files this script changes
            return
        shutil.rmtree(dest)
    dest.mkdir(parents=True, exist_ok=True)
    run("git", "init", "-q", cwd=dest)
    run("git", "remote", "add", "origin", REPO, cwd=dest)
    run("git", "fetch", "-q", "--depth", "1", "origin", commit, cwd=dest)
    run("git", "checkout", "-q", "FETCH_HEAD", cwd=dest)
    run("git", "submodule", "update", "--init", "--recursive", cwd=dest)


def icons(natives):
    from_art = lambda name: HERE / "art" / name  # noqa: E731
    assets = natives / "Assets.xcassets"
    copies = {
        "AppIcon-Light.appiconset/1024x1024.png": "icon.png",
        "AppIcon-Light.appiconset/1024x1024-Transparent.png": "icon-clear.png",
        "AppIcon-Light.appiconset/1024x1024-Monochrome-White.png": "icon-tinted.png",
        "AppIcon-Dark.appiconset/AppIcon-Dark_1024x1024.png": "icon.png",
        "AppIcon-Development.appiconset/AppIcon-Development_1024x1024.png": "icon.png",
        "AppLogo-Vector.imageset/1024x1024-Transparent.png": "icon-clear.png",
    }
    for target, source in copies.items():
        assert (assets / target).exists(), "Amethyst moved its icon: " + target
        shutil.copyfile(from_art(source), assets / target)
    # the plain icon files beside the asset catalog (older iOS, the icon chooser in the settings)
    try:
        from PIL import Image
    except ImportError:
        print("Pillow is missing: the small icon files stay Amethyst's (the asset catalog has Nyxora's)")
        return
    icon = Image.open(from_art("icon.png")).convert("RGB")
    for png in sorted((natives / "resources").glob("AppIcon*.png")):
        size = Image.open(png).size
        icon.resize(size, Image.LANCZOS).save(png)


VALUE = re.compile(r'^(\s*"(?:[^"\\]|\\.)*"\s*=\s*")((?:[^"\\]|\\.)*)("\s*;.*)$')


def texts(natives):
    """the app's name in the texts of every language: only in the translations, never in the keys"""
    changed = 0
    for strings in sorted((natives / "resources").glob("*.lproj/*.strings")):
        lines = strings.read_text(encoding="utf-8").split("\n")
        for i, line in enumerate(lines):
            m = VALUE.match(line)
            if m and "Amethyst" in m.group(2):
                lines[i] = m.group(1) + m.group(2).replace("Angel Aura Amethyst", NAME).replace("Amethyst", NAME) + m.group(3)
                changed += 1
        strings.write_text("\n".join(lines), encoding="utf-8")
    print(f"{changed} texts name {NAME}")


def info(natives):
    plist = natives / "Info.plist"
    data = plistlib.loads(plist.read_bytes())
    data["CFBundleDisplayName"] = NAME
    plist.write_bytes(plistlib.dumps(data))


NATIVE = ("NyxoraHomeViewController.h", "NyxoraHomeViewController.m", "NyxoraLook.h", "NyxoraLook.m")


def patch(path, old, new, count=1):
    """one change in one of Amethyst's files; a file that changed under it stops the build here, not on the Mac"""
    text = path.read_text(encoding="utf-8")
    if new in text:
        return  # already there (the script ran twice)
    assert text.count(old) == count, f"Amethyst changed {path.name}: {old!r} is not there {count} time(s)"
    path.write_text(text.replace(old, new), encoding="utf-8")


def nyxora(natives):
    """Nyxora's menu as the first page, the look for the game before every start"""
    sys.path.insert(0, str(HERE))
    import build_web
    build_web.build(natives / "resources" / "nyxora")
    for name in NATIVE:
        shutil.copyfile(HERE / "native" / name, natives / name)

    # built with the rest of the app
    patch(natives / "CMakeLists.txt", "  LauncherNewsViewController.m\n",
          "  LauncherNewsViewController.m\n  NyxoraHomeViewController.m\n  NyxoraLook.m\n")

    # the first page: Nyxora, then Amethyst's news, profiles and settings
    menu = natives / "LauncherMenuViewController.m"
    patch(menu, '#import "LauncherNewsViewController.h"\n',
          '#import "LauncherNewsViewController.h"\n#import "NyxoraHomeViewController.h"\n')
    patch(menu, "        [LauncherMenuCustomItem vcClass:LauncherNewsViewController.class],\n",
          "        [LauncherMenuCustomItem vcClass:NyxoraHomeViewController.class],\n"
          "        [LauncherMenuCustomItem vcClass:LauncherNewsViewController.class],\n")
    patch(menu, "    NSIndexPath *indexPath = [NSIndexPath indexPathForRow:1 inSection:0];\n"
                "    [self.tableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionNone];\n"
                "    [self tableView:self.tableView didSelectRowAtIndexPath:indexPath];\n"
                "    self.lastSelectedIndex = 1;\n",
          "    NSIndexPath *indexPath = [NSIndexPath indexPathForRow:0 inSection:0];\n"
          "    [self.tableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionNone];\n"
          "    [self tableView:self.tableView didSelectRowAtIndexPath:indexPath];\n"
          "    self.lastSelectedIndex = 0;\n")
    split = natives / "LauncherSplitViewController.m"
    patch(split, '#import "LauncherProfilesViewController.h"\n',
          '#import "LauncherProfilesViewController.h"\n#import "NyxoraHomeViewController.h"\n')
    patch(split, "initWithRootViewController:[[LauncherProfilesViewController alloc] init]];",
          "initWithRootViewController:[[NyxoraHomeViewController alloc] init]];")

    # the menu hears about a new account or profile and about the start of the game
    patch(menu, "- (void)updateAccountInfo {\n",
          "- (void)updateAccountInfo {\n"
          "    dispatch_async(dispatch_get_main_queue(), ^{\n"
          "        [NSNotificationCenter.defaultCenter postNotificationName:@\"NyxoraChanged\" object:nil];\n"
          "    });\n")
    patch(natives / "PLProfiles.m", "- (void)setSelectedProfileName:(NSString *)name {\n",
          "- (void)setSelectedProfileName:(NSString *)name {\n"
          "    dispatch_async(dispatch_get_main_queue(), ^{\n"
          "        [NSNotificationCenter.defaultCenter postNotificationName:@\"NyxoraChanged\" object:nil];\n"
          "    });\n")
    patch(natives / "ios_uikit_bridge.m", "void UIKit_launchMinecraftSurfaceVC(UIWindow* window, NSDictionary* metadata) {\n",
          "void UIKit_launchMinecraftSurfaceVC(UIWindow* window, NSDictionary* metadata) {\n"
          "    [NSNotificationCenter.defaultCenter postNotificationName:@\"NyxoraGameStarts\" object:nil];\n")

    # the look for the game: after the game folder is known, before the game starts
    launcher = natives / "JavaLauncher.m"
    patch(launcher, '#import "JavaLauncher.h"\n', '#import "JavaLauncher.h"\n#import "NyxoraLook.h"\n')
    patch(launcher, "    [MinecraftOptionUtils setupOptionsAtGameDir:gameDir];\n",
          "    [MinecraftOptionUtils setupOptionsAtGameDir:gameDir];\n"
          "    if (!launchJar) {\n"
          "        // Nyxora: the Nyxora Cosmetics mod and the look in a Fabric or Quilt 1.20.1 pack, the mod out of any other\n"
          "        [NyxoraLook prepareGameDir:gameDir versionId:PLProfiles.current.selectedProfile[@\"lastVersionId\"]];\n"
          "    }\n")
    print("Nyxora's menu and the look for the game are in")


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    dest = pathlib.Path(sys.argv[1]).resolve()
    fetch(dest, (HERE / "amethyst.commit").read_text().strip())
    natives = dest / "Natives"
    info(natives)
    icons(natives)
    texts(natives)
    nyxora(natives)
    print("Ready:", dest)


if __name__ == "__main__":
    main()
