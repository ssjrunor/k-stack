#!/usr/bin/env python3
"""Feed synthetic text to the running macOS clipboard app and check saved history."""

import argparse
from collections import Counter
import json
from pathlib import Path
import platform
import secrets
import shutil
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "build/dev/apps/desktop/kstack_desktop.app/Contents/MacOS/kstack_desktop"


def content_for(number: int, run_id: str) -> str:
    serial = f"{number:06d}"
    kind = number % 6
    if kind == 0:
        return f"Clipboard stress run {run_id}, note {serial}, was copied."
    if kind == 1:
        return f"const stressItem_{run_id}_{serial} = {number};"
    if kind == 2:
        return f"http://localhost/stress/{run_id}/{serial}"
    if kind == 3:
        return f"#{(int(run_id, 16) + number) & 0xFFFFFFFF:08X}"
    if kind == 4:
        return str(int(run_id, 16) * 100_000 + number)
    return f"Stress run {run_id}, item {serial}\nSecond line of text."


def app_is_running() -> bool:
    return subprocess.run(["pgrep", "-x", "kstack_desktop"],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0


QUIT_APP_HELP = ("The clipboard app runs in the macOS menu bar, not the Dock. "
                 "Choose Quit from its menu bar icon, or run `pkill -x kstack_desktop` "
                 "in Terminal, then retry.")


def stress_build_available() -> bool:
    if not APP.is_file():
        return False
    check = subprocess.run([str(APP), "--stress-enabled"], capture_output=True,
                           text=True, timeout=10)
    return check.returncode == 0 and check.stdout.strip() == "kstack-stress-v1"


def require_stress_build() -> None:
    if not APP.is_file():
        raise RuntimeError(f"Build the desktop app first: {APP}")
    if not stress_build_available():
        raise RuntimeError("Build with -DKSTACK_PERSISTENCE=ON -DKSTACK_ENABLE_STRESS_PROFILE=ON")


def start_app(profile: Path, log: Path) -> subprocess.Popen:
    (profile / "ready").unlink(missing_ok=True)
    with log.open("ab") as output:
        process = subprocess.Popen([str(APP), "--stress-profile", str(profile)],
                                   stdout=output, stderr=subprocess.STDOUT)
    try:
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f"App exited with status {process.returncode}; see {log}")
            ready = profile / "ready"
            if ready.exists():
                state = ready.read_text(encoding="utf-8")
                if state != "capturing":
                    raise RuntimeError(f"Clipboard capture is {state}; see {log}")
                return process
            time.sleep(0.1)
        raise RuntimeError(f"App did not become ready within 60 seconds; see {log}")
    except BaseException:
        stop_app(process)
        raise


def stop_app(process: subprocess.Popen | None) -> None:
    if process is None or process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)


def report(profile: Path) -> list[str]:
    result = subprocess.run([str(APP), "--stress-report", str(profile)],
                            capture_output=True, text=True, timeout=120)
    if result.returncode != 0:
        raise RuntimeError(f"Cannot read stress history: {result.stderr.strip()}")
    entries = json.loads(result.stdout)["entries"]
    if not isinstance(entries, list) or not all(isinstance(item, str) for item in entries):
        raise RuntimeError("Stress report has an invalid shape")
    return entries


def cleanup(profile: Path) -> None:
    result = subprocess.run([str(APP), "--stress-cleanup", str(profile)],
                            capture_output=True, text=True, timeout=30)
    if result.returncode != 0:
        raise RuntimeError(f"Cannot remove stress Keychain item: {result.stderr.strip()}")
    shutil.rmtree(profile)


def copy_items(expected: list[str], interval: float, process: subprocess.Popen | None = None) -> None:
    for number, content in enumerate(expected, 1):
        if process is not None and process.poll() is not None:
            raise RuntimeError("Isolated app exited during capture")
        subprocess.run(["pbcopy"], input=content.encode("utf-8"), check=True, timeout=10)
        time.sleep(interval)
        if number % 100 == 0 or number == len(expected):
            print(f"Copied {number}/{len(expected)}", flush=True)


def verify_live(expected: list[str]) -> tuple[int, list[int]]:
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", prefix="kstack-stress-manifest-",
                                     suffix=".json", delete=False) as manifest:
        path = Path(manifest.name)
        path.chmod(0o600)
        json.dump(expected, manifest)
    try:
        result = subprocess.run([str(APP), "--stress-verify-live", str(path)],
                                capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise RuntimeError(f"Cannot verify normal history: {result.stderr.strip()}")
        summary = json.loads(result.stdout)
        saved, missing = summary["saved"], summary["missing"]
        if not isinstance(saved, int) or not isinstance(missing, list) or any(
                not isinstance(index, int) or index < 0 or index >= len(expected) for index in missing):
            raise RuntimeError("Invalid verification result")
        return saved, missing
    finally:
        path.unlink(missing_ok=True)


def run_live(count: int, interval: float, run_id: str, verify: bool) -> int:
    if not app_is_running():
        raise RuntimeError("Open the normal clipboard app first and enable capture in its menu bar menu")
    if verify:
        require_stress_build()
    expected = [content_for(number, run_id) for number in range(count)]
    started = time.monotonic()
    print(f"Run ID: {run_id}; copying {count} items into the running app", flush=True)
    try:
        copy_items(expected, interval)
        if not app_is_running():
            raise RuntimeError("The clipboard app exited during capture")
        if not verify:
            print(f"Copied {count} items; elapsed: {(time.monotonic() - started) / 60:.1f} min")
            print("Saved history was not checked; open it from the menu bar to inspect the items")
            return 0
        saved, missing = verify_live(expected)
        print(f"Saved in normal encrypted history: {saved}/{count}; missing: {len(missing)}; "
              f"elapsed: {(time.monotonic() - started) / 60:.1f} min")
        if missing:
            print("Missing sequence examples:", ", ".join(str(index) for index in missing[:12]))
        print("Open history from the menu bar to inspect the saved items")
        return 0 if not missing and saved == count else 1
    finally:
        print("The runner does not clear normal history; the system clipboard holds the last item")


def run_isolated(count: int, interval: float, run_id: str) -> int:
    if app_is_running():
        raise RuntimeError(QUIT_APP_HELP)
    profile = Path(tempfile.mkdtemp(prefix="kstack-stress-", dir=tempfile.gettempdir())).resolve()
    profile.chmod(0o700)
    log = profile / "app.log"
    process = None
    success = False
    expected = [content_for(number, run_id) for number in range(count)]
    started = time.monotonic()
    print(f"Profile: {profile}", flush=True)
    print(f"Writing {count} clipboard items, {interval:.2f} seconds apart", flush=True)
    try:
        process = start_app(profile, log)
        copy_items(expected, interval, process)
        stop_app(process)
        process = start_app(profile, log)
        stop_app(process)
        actual = report(profile)
        missing = list((Counter(expected) - Counter(actual)).elements())
        extra = list((Counter(actual) - Counter(expected)).elements())
        print(f"Saved after restart: {len(actual)}/{count}; missing: {len(missing)}; "
              f"unexpected: {len(extra)}; elapsed: {(time.monotonic() - started) / 60:.1f} min")
        if missing:
            print("Missing sequence examples:", ", ".join(str(expected.index(item)) for item in missing[:12]))
        if extra:
            print(f"Unexpected entry count: {len(extra)} (contents omitted)")
        success = not missing and not extra and len(actual) == count
        return 0 if success else 1
    finally:
        stop_app(process)
        print(f"Stress profile kept at {profile}; clean up with: "
              f"python3 scripts/stress-clipboard.py --cleanup {profile}")
        if success:
            print(f"Open for inspection: {APP} --stress-profile {profile}")
        print("The system clipboard still contains the last synthetic item")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=5000)
    parser.add_argument("--interval", type=float, default=0.5,
                        help="seconds each item remains on the clipboard (default: 0.5)")
    parser.add_argument("--isolated", action="store_true",
                        help="use a separate profile and verify capture after restarting it")
    parser.add_argument("--no-verify", action="store_true",
                        help="only feed the running app; skip checking its saved history")
    parser.add_argument("--dry-run", action="store_true",
                        help="check generated items and print estimated duration")
    parser.add_argument("--cleanup", type=Path, metavar="PROFILE",
                        help="remove a kept profile and its separate Keychain item")
    args = parser.parse_args()
    if platform.system() != "Darwin":
        parser.error("this runner requires macOS")
    if args.cleanup:
        require_stress_build()
        if app_is_running():
            parser.error(QUIT_APP_HELP)
        cleanup(args.cleanup.resolve())
        print("Disposable database and Keychain item removed")
        return 0
    if args.count < 1 or args.count > 100_000 or args.interval < 0.3:
        parser.error("count must be 1–100000 and interval must be at least 0.3 seconds")
    run_id = secrets.token_hex(4)
    if args.dry_run:
        items = [content_for(number, run_id) for number in range(args.count)]
        if len(set(items)) != len(items):
            raise RuntimeError("Generated clipboard items are not unique")
        print(f"{len(items)} unique synthetic items; estimated feed time "
              f"{args.count * args.interval / 60:.1f} minutes")
        return 0
    if args.isolated:
        require_stress_build()
        return run_isolated(args.count, args.interval, run_id)
    return run_live(args.count, args.interval, run_id, not args.no_verify)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, subprocess.SubprocessError, RuntimeError, ValueError, KeyError) as error:
        print(f"Stress run failed: {error}", file=sys.stderr)
        sys.exit(2)
