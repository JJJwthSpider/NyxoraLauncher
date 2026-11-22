#!/usr/bin/env python3
"""Turns a ready-made iPhone app of Amethyst (or of PojavLauncher for iOS, which it grew out of) into Nyxora Launcher,
without a Mac: an .ipa is a zip, and what makes it Nyxora is plain files in it.

    python3 ios/brand_ipa.py <Amethyst or PojavLauncher .ipa> [NyxoraLauncher-iPhone.ipa]

What changes in Payload/<App>.app:
- Info.plist: the name on the home screen ("Nyxora"); the icon is taken from the plain icon files (the compiled asset
  catalog, Assets.car, can only be rebuilt with Xcode, so the app is told to use the files instead);
- the plain icon files (AppIcon*.png): Nyxora's icon (ios/art/icon.png, made by ios/make_icons.py);
- the texts of every language (*.lproj/*.strings): "Nyxora" for the app's name.
The program itself (the executable, its entitlements) is not touched. The old signature no longer matches the
changed files, which is how it should be: SideStore or AltStore sign the app again with the person's Apple ID when
they install it (HOW-TO-IPHONE-RU.txt). The bundle id stays, so StikDebug and GetMoreRam treat it as Amethyst.

Like Amethyst itself, the app signs in only with a Microsoft account and plays the demo without the game; nothing
here changes that (CLAUDE.md).
"""
import io
import pathlib
import plistlib
import re
import sys
import zipfile

HERE = pathlib.Path(__file__).resolve().parent
NAME = "Nyxora"
OLD_NAMES = ("Angel Aura Amethyst", "Amethyst", "PojavLauncher")
VALUE = re.compile(r'^(\s*"(?:[^"\\]|\\.)*"\s*=\s*")((?:[^"\\]|\\.)*)("\s*;.*)$')


def rename(text):
    for old in OLD_NAMES:
        text = text.replace(old, NAME)
    return text


def patch_info(data):
    info = plistlib.loads(data)
    binary = data.startswith(b"bplist")
    info["CFBundleDisplayName"] = NAME
    for key in ("CFBundleIcons", "CFBundleIcons~ipad"):
        icons = info.get(key, {})
        # without the name of an icon in Assets.car, iOS takes the icon files listed beside it (CFBundleIconFiles):
        # the main icon and the ones to choose in the settings
        entries = [icons.get("CFBundlePrimaryIcon")] + list(icons.get("CFBundleAlternateIcons", {}).values())
        for entry in entries:
            if isinstance(entry, dict) and entry.get("CFBundleIconFiles"):
                entry.pop("CFBundleIconName", None)
    return plistlib.dumps(info, fmt=plistlib.FMT_BINARY if binary else plistlib.FMT_XML)


def patch_strings(data):
    if data.startswith(b"bplist"):
        table = plistlib.loads(data)
        return plistlib.dumps({k: rename(v) if isinstance(v, str) else v for k, v in table.items()}, fmt=plistlib.FMT_BINARY), True
    for encoding in ("utf-8", "utf-16"):
        try:
            text = data.decode(encoding)
            break
        except UnicodeDecodeError:
            continue
    else:
        return data, False
    lines = text.split("\n")
    changed = False
    for i, line in enumerate(lines):
        m = VALUE.match(line)
        if m and any(old in m.group(2) for old in OLD_NAMES):
            lines[i] = m.group(1) + rename(m.group(2)) + m.group(3)
            changed = True
    return "\n".join(lines).encode(encoding), changed


def icon_bytes(icon, size):
    from PIL import Image
    out = io.BytesIO()
    icon.resize(size, Image.LANCZOS).save(out, "PNG")
    return out.getvalue()


def brand(source, target):
    from PIL import Image
    icon = Image.open(HERE / "art" / "icon.png").convert("RGB")
    with zipfile.ZipFile(source) as zin:
        names = zin.namelist()
        apps = sorted({n.split("/")[1] for n in names if n.startswith("Payload/") and n.count("/") >= 2 and n.split("/")[1].endswith(".app")})
        if len(apps) != 1:
            sys.exit(f"{source}: expected one app in Payload/, found {apps or 'none'}")
        app = f"Payload/{apps[0]}/"
        if app + "Info.plist" not in names:
            sys.exit(f"{source}: {app}Info.plist is missing")
        counts = {"icons": 0, "texts": 0}
        with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as zout:
            for item in zin.infolist():
                data = zin.read(item.filename)
                rel = item.filename[len(app):] if item.filename.startswith(app) else None
                if rel == "Info.plist":
                    data = patch_info(data)
                elif rel is not None and "/" not in rel and re.fullmatch(r"AppIcon.*\.png", rel):
                    data = icon_bytes(icon, Image.open(io.BytesIO(data)).size)
                    counts["icons"] += 1
                elif rel is not None and re.fullmatch(r"[^/]+\.lproj/[^/]+\.strings", rel):
                    data, changed = patch_strings(data)
                    counts["texts"] += changed
                # keep the entry as it was (its permissions, its symlink bit), with the new contents
                zout.writestr(item, data, compress_type=zipfile.ZIP_STORED if item.compress_type == zipfile.ZIP_STORED else zipfile.ZIP_DEFLATED)
    print(f"{target}: {apps[0]} named {NAME}, {counts['icons']} icon files, {counts['texts']} text files")


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    source = pathlib.Path(sys.argv[1])
    target = pathlib.Path(sys.argv[2] if len(sys.argv) == 3 else "NyxoraLauncher-iPhone.ipa")
    brand(source, target)


if __name__ == "__main__":
    main()
