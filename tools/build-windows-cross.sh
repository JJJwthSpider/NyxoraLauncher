#!/bin/bash
# Builds Vellora Client for Windows (64 bit) on Linux, without Windows: MinGW-w64 GCC (posix threads), Qt 6.4.2 for
# MinGW (aqtinstall), the libraries from MSYS2 (libarchive, qrencode, zlib, toml++ and the GCC runtime) and cmark built
# here. The result is a portable folder and VelloraClient-Windows.zip: unpack it anywhere and start vellora.exe.
#
# Needs: g++-mingw-w64-x86-64-posix, cmake, ninja, zstd, python3 (pip: aqtinstall), Java (for the jars), the Qt 6.4 tools
# of the Linux build (moc, rcc, uic, lrelease) and extra-cmake-modules.
#   tools/build-windows-cross.sh [work dir]
set -euo pipefail
SRC="$(cd "$(dirname "$0")/.." && pwd)"
W="${1:-$SRC/build-windows}"
mkdir -p "$W"
cd "$W"

# --- Qt 6.4.2 for MinGW (the same version as the Linux Qt tools, so moc and rcc match)
if [ ! -d qt/6.4.2/mingw_64 ]; then
    python3 -m aqt install-qt windows desktop 6.4.2 win64_mingw -m qtnetworkauth qtimageformats -O qt -b https://ftp.fau.de/qtproject/
fi

# --- the libraries from MSYS2 (mingw64, msvcrt like MinGW-w64), with everything they depend on
if [ ! -d sysroot/mingw64 ]; then
    mkdir -p msys/db msys/pkgs sysroot
    curl -sS -o msys/mingw64.db https://repo.msys2.org/mingw/mingw64/mingw64.db
    zstd -dq -f msys/mingw64.db -o msys/mingw64.tar
    tar -xf msys/mingw64.tar -C msys/db
    python3 - msys/db qrencode libarchive tomlplusplus zlib libwinpthread > msys/list.txt <<'EOF'
import os, re, sys
db, want = sys.argv[1], sys.argv[2:]
packages, provides = {}, {}
for d in os.listdir(db):
    fields = {m.group(1): m.group(2).split() for m in re.finditer(r'%(\w+)%\n((?:.+\n)+)', open(os.path.join(db, d, 'desc')).read())}
    packages[fields['NAME'][0]] = fields
    for p in fields.get('PROVIDES', []):
        provides[re.split('[<>=]', p)[0]] = fields['NAME'][0]
skip = {'mingw-w64-x86_64-crt-git', 'mingw-w64-x86_64-headers-git'}
seen, out = set(), []
def add(name):
    name = re.split('[<>=]', name)[0]
    name = name if name in packages else provides.get(name, name)
    if name in seen or name in skip or name not in packages:
        return
    seen.add(name)
    for dep in packages[name].get('DEPENDS', []):
        add(dep)
    out.append(packages[name]['FILENAME'][0])
for w in want:
    add('mingw-w64-x86_64-' + w)
print('\n'.join(out))
EOF
    for f in $(cat msys/list.txt); do
        curl -sS -o "msys/pkgs/$f" "https://repo.msys2.org/mingw/mingw64/$f"
        tar --zstd -xf "msys/pkgs/$f" -C sysroot
    done
fi

# --- cmark (not in MSYS2's mingw64 any more): a static library
cat > toolchain.cmake <<EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH $W/qt/6.4.2/mingw_64 $W/sysroot/mingw64 $W/prefix /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
EOF
if [ ! -f prefix/lib/libcmark.a ]; then
    [ -d cmark-src ] || git clone -q --depth 1 --branch 0.31.1 https://github.com/commonmark/cmark cmark-src
    cmake -S cmark-src -B cmark-build -G Ninja -DCMAKE_TOOLCHAIN_FILE="$W/toolchain.cmake" -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$W/prefix" -DBUILD_TESTING=OFF -DCMARK_TESTS=OFF
    ninja -C cmark-build install
fi

# --- the launcher
export PKG_CONFIG_LIBDIR="$W/sysroot/mingw64/lib/pkgconfig:$W/prefix/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$W/sysroot"
cmake -S "$SRC" -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE="$W/toolchain.cmake" -DCMAKE_BUILD_TYPE=Release \
      -DQT_HOST_PATH=/usr -DQT_HOST_PATH_CMAKE_DIR=/usr/lib/x86_64-linux-gnu/cmake \
      -DCMAKE_PREFIX_PATH="$W/qt/6.4.2/mingw_64;$W/prefix;$W/sysroot/mingw64" \
      -DLauncher_APP_BINARY_NAME=vellora -DCMAKE_INSTALL_PREFIX="$W/install" -DBUILD_TESTING=OFF
ninja -C build
rm -rf install
cmake --install build
cmake --install build --component portable

# --- the DLLs it needs: every import, found in the Qt, MSYS2 and cmark folders (MSYS2's newer GCC runtime wins)
python3 - "$W" <<'EOF'
import os, shutil, subprocess, sys
w = sys.argv[1]
out = os.path.join(w, 'install')
search = [os.path.join(w, 'sysroot/mingw64/bin'), os.path.join(w, 'qt/6.4.2/mingw_64/bin'), os.path.join(w, 'prefix/bin')]
plugins = {'platforms': ['qwindows.dll'], 'styles': ['qwindowsvistastyle.dll'], 'iconengines': ['qsvgicon.dll'],
           'imageformats': ['qsvg.dll', 'qjpeg.dll', 'qgif.dll', 'qico.dll', 'qwebp.dll', 'qtga.dll', 'qwbmp.dll', 'qtiff.dll'],
           'tls': ['qschannelbackend.dll', 'qcertonlybackend.dll']}
qt_plugins = os.path.join(w, 'qt/6.4.2/mingw_64/plugins')
todo = [os.path.join(out, f) for f in os.listdir(out) if f.lower().endswith('.exe')]
for folder, names in plugins.items():
    os.makedirs(os.path.join(out, folder), exist_ok=True)
    for n in names:
        src = os.path.join(qt_plugins, folder, n)
        if os.path.exists(src):
            shutil.copy(src, os.path.join(out, folder, n))
            todo.append(os.path.join(out, folder, n))
done = set()
while todo:
    f = todo.pop()
    dump = subprocess.run(['x86_64-w64-mingw32-objdump', '-p', f], capture_output=True, text=True).stdout
    for line in dump.splitlines():
        if 'DLL Name:' not in line:
            continue
        dll = line.split('DLL Name:')[1].strip()
        if dll.lower() in done:
            continue
        done.add(dll.lower())
        for d in search:
            src = os.path.join(d, dll)
            if os.path.exists(src):
                shutil.copy(src, os.path.join(out, dll))
                todo.append(os.path.join(out, dll))
                break
print('bundled', len([d for d in done if os.path.exists(os.path.join(out, d))]), 'DLLs')
EOF
printf '[Paths]\nPlugins = .\n' > install/qt.conf
rm -f VelloraClient-Windows.zip
(cd install && zip -qr9 "$W/VelloraClient-Windows.zip" .)
echo "Built $W/VelloraClient-Windows.zip ($(du -h "$W/VelloraClient-Windows.zip" | cut -f1))"
