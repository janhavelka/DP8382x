"""Verify the complete selected SDK dependency closure before compiling real sources."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

PIN = "fff9895c82d744c7237be8847347bdd1b07c6643"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--include", action="append", default=[])
    parser.add_argument("--source", action="append", default=[])
    parser.add_argument("--define", action="append", default=[])
    parser.add_argument("--option", action="append", default=[])
    parser.add_argument("--force-include")
    parser.add_argument("--write-manifest", action="store_true")
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    revision = subprocess.check_output(["git", "-C", str(sdk), "rev-parse", "HEAD"], text=True).strip()
    if revision != PIN:
        raise SystemExit(f"SDK revision mismatch: {revision}; required {PIN}")
    selected = set()
    for source in args.source:
        cpp = Path(source).suffix in (".cpp", ".cc", ".cxx")
        # An SDK directory can also arrive through -isystem. Classify files
        # by their resolved checkout path after scanning both header classes.
        command = [args.cxx if cpp else args.compiler, "-std=c++17" if cpp else "-std=gnu17", "-M", "-MT", "closure"]
        for definition in args.define:
            command += ["-D", definition]
        for include in args.include:
            command += ["-I", include]
        command.extend(args.option)
        if args.force_include:
            command += ["-include", args.force_include]
        command.append(source)
        output = subprocess.check_output(command, text=True)
        # GCC prints a Make rule, including escaped spaces and hashes.
        # Backslashes before ordinary letters are Windows path separators.
        dependencies = output.replace("\\\n", " ").split("closure:", 1)[1]
        for token in re.findall(r"(?:\\[ \t#]|[^\s])+", dependencies):
            filename = re.sub(r"\\([ \t#])", r"\1", token).replace("$$", "$")
            path = Path(filename).resolve()
            if path.is_relative_to(sdk):
                selected.add(path.relative_to(sdk).as_posix())
    entries = {}
    tree = subprocess.check_output(
        ["git", "-C", str(sdk), "ls-tree", "-r", PIN, "--", *sorted(selected)], text=True
    )
    pinned_files = {line.split("\t", 1)[1]: line.split("\t", 1)[0].split()[2] for line in tree.splitlines()}
    for name in sorted(selected):
        # Git's Windows checkout may expand LF to CRLF. Compare canonical blob
        # content; no other source transformation or filter is accepted here.
        payload = (sdk / name).read_bytes().replace(b"\r\n", b"\n")
        digest = hashlib.sha1(f"blob {len(payload)}\0".encode() + payload).hexdigest()
        if name not in pinned_files:
            raise SystemExit(f"SDK dependency absent from pin: {name}")
        pinned = pinned_files[name]
        if digest != pinned:
            raise SystemExit(f"SDK working file differs from pin: {name} ({digest}, expected {pinned})")
        entries[name] = digest
    if args.write_manifest:
        args.manifest.write_text(json.dumps({"revision": PIN, "files": entries}, indent=2) + "\n", encoding="utf-8")
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if manifest["revision"] != PIN or manifest["files"] != entries:
        missing = sorted(entries.keys() - manifest["files"].keys())
        extra = sorted(manifest["files"].keys() - entries.keys())
        raise SystemExit(f"SDK dependency closure differs from manifest; unrecorded={missing}, unused={extra}")
    print(f"Verified {len(entries)} real SDK sources/headers at {PIN}")


if __name__ == "__main__":
    main()
