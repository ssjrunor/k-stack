#!/usr/bin/env python3
# Author: Runor Ewhro
# Description: downloads Bergamot translation models into the folder the app
#              reads them from, checking each archive's SHA-256 against the
#              translateLocally registry. The app itself never downloads.
#
#   scripts/fetch-translation-models.py --list
#   scripts/fetch-translation-models.py de-en es-en fr-en
#
# A pair such as "de-en" takes the larger "base" model when one exists;
# name a model exactly, such as "de-en-tiny", to choose.
import argparse
import hashlib
import json
import os
import shutil
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

REGISTRY = "https://translatelocally.com/models.json"
APP_NAME = "Clipboard History"


def default_directory() -> Path:
    # matches QStandardPaths::AppDataLocation for the app's name
    home = Path.home()
    if sys.platform == "darwin":
        base = home / "Library" / "Application Support"
    elif sys.platform == "win32":
        base = Path(os.environ.get("APPDATA", home / "AppData" / "Roaming"))
    else:
        base = Path(os.environ.get("XDG_DATA_HOME", home / ".local" / "share"))
    return base / APP_NAME / "translation-models"


def load_registry() -> list:
    with urllib.request.urlopen(REGISTRY, timeout=30) as response:
        return json.load(response)["models"]


def pick(models: list, name: str) -> dict:
    exact = [m for m in models if m["shortName"] == name]
    if exact:
        return exact[0]
    pair = [m for m in models if m["shortName"].rsplit("-", 1)[0] == name]
    if not pair:
        raise SystemExit(f"no model named {name}; see --list")
    return next((m for m in pair if m["type"] == "base"), pair[0])


def install(model: dict, directory: Path) -> None:
    with tempfile.TemporaryDirectory() as scratch:
        archive = Path(scratch) / "model.tar.gz"
        with urllib.request.urlopen(model["url"], timeout=120) as response, open(archive, "wb") as out:
            shutil.copyfileobj(response, out)
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != model["checksum"]:
            raise SystemExit(f"{model['shortName']}: checksum mismatch, not installed")
        with tarfile.open(archive) as tar:
            members = tar.getmembers()
            for member in members:
                target = (Path(scratch) / member.name).resolve()
                if not target.is_relative_to(Path(scratch).resolve()) or not (member.isfile() or member.isdir()):
                    raise SystemExit(f"{model['shortName']}: unsafe archive entry {member.name}")
            if hasattr(tarfile, "data_filter"):
                tar.extractall(scratch, members=members, filter="data")
            else:
                tar.extractall(scratch, members=members)
        folders = [p for p in Path(scratch).iterdir() if p.is_dir()]
        if len(folders) != 1 or not (folders[0] / "model_info.json").exists():
            raise SystemExit(f"{model['shortName']}: unexpected archive layout")
        destination = directory / model["shortName"]
        if destination.exists():
            shutil.rmtree(destination)
        directory.mkdir(parents=True, exist_ok=True)
        shutil.move(str(folders[0]), destination)
    print(f"installed {model['shortName']} ({model['src']} to {model['trg']})")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("models", nargs="*", help='pairs such as "de-en", or exact names')
    parser.add_argument("--list", action="store_true", help="list the models available")
    parser.add_argument("--dir", type=Path, default=default_directory(), help="install folder")
    args = parser.parse_args()

    models = load_registry()
    if args.list or not args.models:
        for m in models:
            print(f"{m['shortName']:16} {m['src']} to {m['trg']}")
        return
    for name in args.models:
        install(pick(models, name), args.dir)
    print(f"models are in {args.dir}; restart the app to use them")


if __name__ == "__main__":
    main()
