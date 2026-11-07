"""Use Espressif's native GCC 8.4 build on Apple Silicon without Rosetta.

The pinned PlatformIO registry package labels its Intel executable as compatible
with darwin_arm64. Replace only this project's cached toolchain with the same
Espressif release for arm64, verifying Espressif's published SHA-256 first.
Other host systems keep PlatformIO's normal toolchain.
"""
import hashlib
import platform
from pathlib import Path
import shutil
import struct
import tarfile
import tempfile
import urllib.request

Import("env")  # noqa: F821 - supplied by PlatformIO/SCons


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ensure_native_toolchain():
    if platform.system() != "Darwin" or platform.machine() != "arm64":
        return
    package = Path(env.PioPlatform().get_package_dir("toolchain-riscv32-esp"))
    compiler = package / "bin/riscv32-esp-elf-g++"
    with compiler.open("rb") as executable:
        magic, cpu = struct.unpack("<II", executable.read(8))
    if magic == 0xFEEDFACF and cpu == 0x0100000C:  # Mach-O arm64
        return
    # Keep this adaptation confined to the project, even with a core-dir override.
    project = Path(env.subst("$PROJECT_DIR")).resolve()
    if project not in package.resolve().parents:
        raise RuntimeError("Use platformio.ini's project-local core_dir on Apple Silicon.")

    filename = "riscv32-esp-elf-gcc8_4_0-esp-2021r2-patch5-macos-arm64.tar.gz"
    url = "https://github.com/espressif/crosstool-NG/releases/download/esp-2021r2-patch5/" + filename
    digest = "6e03f2ab1f145be13f8890c6de77b53f52c7bffe3d9d5824549db20298f5ba91"
    cache = project / ".pio-core/.cache"
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / filename
    if not archive.exists() or sha256(archive) != digest:
        print("Downloading Espressif's native Apple Silicon GCC 8.4 toolchain...")
        with urllib.request.urlopen(url, timeout=60) as response, archive.open("wb") as output:
            shutil.copyfileobj(response, output)
    if sha256(archive) != digest:
        archive.unlink()
        raise RuntimeError("Espressif toolchain checksum mismatch")

    with tempfile.TemporaryDirectory(dir=cache) as temporary:
        with tarfile.open(archive) as tar:
            # Python 3.12+ and maintained 3.10/3.11 releases support data_filter.
            tar.extractall(temporary, filter="data")
        root = Path(temporary) / "riscv32-esp-elf"
        if not (root / "bin/riscv32-esp-elf-g++").is_file():
            raise RuntimeError("Unexpected Espressif toolchain archive layout")
        for metadata in ("package.json", ".piopm"):
            shutil.copy2(package / metadata, root / metadata)
        previous = Path(temporary) / "previous"
        package.rename(previous)
        try:
            root.rename(package)
        except OSError:
            previous.rename(package)
            raise
    print("Installed native Apple Silicon GCC 8.4 (esp-2021r2-patch5).")


ensure_native_toolchain()
