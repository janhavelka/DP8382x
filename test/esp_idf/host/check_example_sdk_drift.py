"""Check an example-only header edit against the actual dependency verifier."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    command = sys.argv[1:]
    sdk_index = command.index("--sdk") + 1
    sdk = Path(command[sdk_index]).resolve()
    manifest = Path(command[command.index("--manifest") + 1])
    files = json.loads(manifest.read_text(encoding="utf-8"))["files"]
    driver_files = json.loads(manifest.with_name("sdk-files.json").read_text(encoding="utf-8"))["files"]
    header = "components/esp_driver_gpio/include/driver/gpio.h"
    if header not in files or header in driver_files:
        raise SystemExit(f"Expected an example-only SDK dependency: {header}")

    # Git reads the original commit and blobs. The compiler and working-file
    # hasher see only independent copies of the selected SDK inputs.
    git_dir = subprocess.check_output(["git", "-C", str(sdk), "rev-parse", "--absolute-git-dir"],
                                      text=True).strip()
    with tempfile.TemporaryDirectory(prefix="example sdk-", dir=Path.cwd()) as temporary:
        checkout = Path(temporary)
        env = dict(os.environ, GIT_DIR=git_dir, GIT_WORK_TREE=str(checkout))
        for name in files:
            target = checkout / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(sdk / name, target)
        command[sdk_index] = checkout.as_posix()
        for index, argument in enumerate(command[:-1]):
            if argument == "--include":
                include = Path(command[index + 1]).resolve()
                if include.is_relative_to(sdk):
                    command[index + 1] = (checkout / include.relative_to(sdk)).as_posix()

        subprocess.run(command, env=env, check=True)
        # A compiler flag can select SDK headers outside the target's -I list.
        # This pinned header has no dependencies of its own.
        extra = "components/bootloader_support/include/bootloader_clock.h"
        extra_header = checkout / extra
        extra_header.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(sdk / extra, extra_header)
        system_scan = command + ["--option=-isystem", f"--option={extra_header.parent.relative_to(Path.cwd())}",
                                 "--option=-include", f"--option={extra_header.name}"]
        result = subprocess.run(system_scan, env=env, capture_output=True, text=True)
        if result.returncode != 1 or f"unrecorded={[extra]}, unused=[]" not in result.stderr:
            raise SystemExit(f"Unexpected system header verdict {result.returncode}: {result.stdout}{result.stderr}")

        with (checkout / header).open("ab") as changed:
            changed.write(b"\n/* Example-only SDK drift fixture. */\n")
        result = subprocess.run(command, env=env, capture_output=True, text=True)
        if result.returncode != 1 or not result.stderr.startswith(f"SDK working file differs from pin: {header} ("):
            raise SystemExit(f"Unexpected header drift verdict {result.returncode}: {result.stdout}{result.stderr}")
        print(result.stderr.strip())


if __name__ == "__main__":
    main()
