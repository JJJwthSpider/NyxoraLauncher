#!/usr/bin/env python3
"""Makes NyxoraLauncher-src.zip: the sources at HEAD together with the git submodules.

`git archive` (and GitHub's "Download ZIP") leaves the submodules out: libraries/libnbtplusplus comes out empty and
cmake stops with "does not contain a CMakeLists.txt file". This puts each submodule's own `git archive` under its
path in the same zip.

    python3 tools/make-src-zip.py [NyxoraLauncher-src.zip]
"""
import io
import subprocess
import sys
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parent.parent
out = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "NyxoraLauncher-src.zip"


def archive(cwd, prefix=""):
    args = ["git", "archive", "--format=zip"] + (["--prefix=" + prefix] if prefix else []) + ["HEAD"]
    return zipfile.ZipFile(io.BytesIO(subprocess.check_output(args, cwd=cwd)))


paths = []
listing = subprocess.run(["git", "config", "-f", ".gitmodules", "--get-regexp", r"^submodule\..*\.path$"], cwd=root,
                         capture_output=True, text=True).stdout
for line in listing.splitlines():
    paths.append(line.split(None, 1)[1].strip())

parts = [archive(root)]
for path in paths:
    if not (root / path / ".git").exists():
        sys.exit(f"the submodule {path} is not checked out: git submodule update --init")
    parts.append(archive(root / path, path + "/"))

written = set()
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for part in parts:
        for info in part.infolist():
            if info.filename in written:
                continue  # the empty folder git archive writes for a submodule
            written.add(info.filename)
            z.writestr(info, part.read(info))
print(f"{out}: {len(written)} entries, submodules: {', '.join(paths) or 'none'}")
