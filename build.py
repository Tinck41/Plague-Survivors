#!/usr/bin/env python3

import sys
import os
import subprocess
import shutil
from pathlib import Path

# ── Настройки ────────────────────────────────────────────────────────────────

ODIN_SRC     = "gameplay"           # папка с .odin файлами
OUT_DIR      = "."                  # куда класть готовый .so/.dylib/.dll
BUILD_DIR    = "build/ninja"              # где лежит libSoulEngine.a / SoulEngine.lib

# Имя без расширения — расширение добавляется автоматически под платформу
OUT_NAME     = "gameplay"

# Дополнительные флаги odin (например -debug, -o:speed)
EXTRA_ODIN_FLAGS = [
    "-debug",
]

# ── Платформы ────────────────────────────────────────────────────────────────

def get_platform() -> str:
    if len(sys.argv) < 2:
        print("Usage: python3 build.py <platform>")
        print("  platforms: linux  mac  win32")
        sys.exit(1)
    return sys.argv[1].lower()


def find_engine_lib(build_dir: Path, platform: str) -> Path:
    """Ищет libSoulEngine.a / SoulEngine.lib в build_dir и подпапках."""
    if platform == "win32":
        patterns = ["SoulEngine.lib", "*/SoulEngine.lib", "**/SoulEngine.lib"]
    else:
        patterns = ["libSoulEngine.a", "*/libSoulEngine.a", "**/libSoulEngine.a"]

    for pattern in patterns:
        found = list(build_dir.glob(pattern))
        if found:
            return found[0].resolve()

    print(f"ERROR: could not find SoulEngine lib in '{build_dir}'")
    print("  Run CMake build first (cmake --build build)")
    sys.exit(1)


def run(cmd: list[str]):
    print("▶", " ".join(cmd))
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(f"\nERROR: command failed with code {result.returncode}")
        sys.exit(result.returncode)


# ── Билд ─────────────────────────────────────────────────────────────────────

def build_linux(engine_lib: Path, out_dir: Path):
    out_file = out_dir / f"{OUT_NAME}.so"
    lib_dir  = engine_lib.parent

    linker_flags = " ".join([
        f"-L{lib_dir}",
        "-Wl,--whole-archive",
        "-lSoulEngine",
        "-Wl,--no-whole-archive",
        "-lstdc++",
        "-lm",
        "-ldl",
        "-lpthread",
    ])

    cmd = [
        "odin", "build", ODIN_SRC,
        "-build-mode:shared",
        f"-out:{out_file}",
        f"-extra-linker-flags:{linker_flags}",
        *EXTRA_ODIN_FLAGS,
    ]
    run(cmd)
    print(f"\n✓ Built: {out_file}")


def build_mac(engine_lib: Path, out_dir: Path):
    out_file = out_dir / f"{OUT_NAME}.dylib"
    lib_dir  = engine_lib.parent

    linker_flags = " ".join([
        f"-L{lib_dir}",
        f"{engine_lib}",
        "-lc++",
    ])

    cmd = [
        "odin", "build", ODIN_SRC,
        "-build-mode:shared",
        f"-out:{out_file}",
        f"-extra-linker-flags:{linker_flags}",
        *EXTRA_ODIN_FLAGS,
    ]
    run(cmd)
    print(f"\n✓ Built: {out_file}")


def build_win32(engine_lib: Path, out_dir: Path):
    out_file = out_dir / f"{OUT_NAME}.dll"
    lib_dir  = engine_lib.parent

    linker_flags = " ".join([
        f"/LIBPATH:{lib_dir}",
        "SoulEngine.lib",
    ])

    cmd = [
        "odin", "build", ODIN_SRC,
        "-build-mode:shared",
        f"-out:{out_file}",
        f"-extra-linker-flags:{linker_flags}",
        *EXTRA_ODIN_FLAGS,
    ]
    run(cmd)
    print(f"\n✓ Built: {out_file}")


# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    platform  = get_platform()
    root      = Path(__file__).parent.resolve()
    build_dir = root / BUILD_DIR
    out_dir   = root / OUT_DIR

    if not (root / ODIN_SRC).exists():
        print(f"ERROR: Odin source folder '{ODIN_SRC}' not found")
        print(f"  Expected at: {root / ODIN_SRC}")
        sys.exit(1)

    if not build_dir.exists():
        print(f"ERROR: build dir '{build_dir}' not found")
        print("  Run CMake first: cmake -B build && cmake --build build")
        sys.exit(1)

    if not shutil.which("odin"):
        print("ERROR: 'odin' not found in PATH")
        print("  Install Odin: https://odin-lang.org/docs/install/")
        sys.exit(1)

    engine_lib = find_engine_lib(build_dir, platform)
    print(f"Platform : {platform}")
    print(f"Engine   : {engine_lib}")
    print(f"Out dir  : {out_dir}\n")

    if platform == "linux":
        build_linux(engine_lib, out_dir)
    elif platform == "mac":
        build_mac(engine_lib, out_dir)
    elif platform == "win32":
        build_win32(engine_lib, out_dir)
    else:
        print(f"ERROR: unknown platform '{platform}'")
        print("  Use: linux  mac  win32")
        sys.exit(1)


if __name__ == "__main__":
    main()
