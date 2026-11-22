#!/usr/bin/env python3
"""Puts together Nyxora's menu for the iPhone (ios/nyxora/: the lobby, the wardrobe, the player in 3D) with what it
uses from the rest of Nyxora: the wardrobe's items (android/assets/cosmetics.json, as data.js: a page opened from the
app's files cannot fetch() a file), the pixel font, the interface sounds and the music.

    python3 ios/build_web.py <folder>

ios/prepare.py puts it into the app (Natives/resources/nyxora/); opened in a browser, the folder works on its own.
"""
import json
import pathlib
import shutil
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent


def build(dest):
    dest = pathlib.Path(dest)
    if dest.exists():
        shutil.rmtree(dest)
    dest.mkdir(parents=True)
    for name in ("index.html", "nyxora.css", "character.js", "app.js"):
        shutil.copyfile(HERE / "nyxora" / name, dest / name)
    shutil.copytree(HERE / "nyxora" / "music", dest / "music")
    data = json.loads((ROOT / "android" / "assets" / "cosmetics.json").read_text(encoding="utf-8"))
    (dest / "data.js").write_text("window.NYX_COSMETICS = " + json.dumps(data, ensure_ascii=False, separators=(",", ":")) + ";\n",
                                  encoding="utf-8")
    for name in ("Tiny5-Regular.ttf", "OFL-Tiny5.txt"):
        shutil.copyfile(ROOT / "launcher" / "resources" / "sculk_theme" / "fonts" / name, dest / name)
    (dest / "sounds").mkdir()
    for wav in sorted((ROOT / "launcher" / "resources" / "sculk_theme" / "sounds").glob("*.wav")):
        shutil.copyfile(wav, dest / "sounds" / wav.name)
    # the mod that shows the look in the game (ios/native/NyxoraLook.m puts it into Fabric and Quilt 1.20.1 packs)
    shutil.copyfile(ROOT / "android" / "assets" / "vellora-cosmetics-1.20.1.jar", dest / "vellora-cosmetics-1.20.1.jar")
    return dest


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    print("Built", build(sys.argv[1]))
