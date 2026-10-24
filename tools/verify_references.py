"""Offline integrity and local-document-link checks for the reference workspace."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit
import zipfile


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/reference/source"


def main() -> int:
    errors: list[str] = []
    manifest = SOURCE / "provenance.json"
    try:
        entries = json.loads(manifest.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        print(f"FAIL: cannot read provenance: {exc}")
        return 1

    expected = {manifest.resolve()}
    expected_text: set[Path] = set()
    names: set[str] = set()
    total_bytes = 0
    for entry in entries:
        name = entry.get("file", "")
        if not name or name in names or Path(name).name != name:
            errors.append(f"Invalid or duplicate artifact name: {name!r}")
            continue
        names.add(name)
        artifact = SOURCE / name
        expected.add(artifact.resolve())
        required = ("title", "document", "source_url", "resolved_url",
                    "retrieved_utc", "sha256", "bytes", "rights_holder", "terms")
        for field in required:
            if not entry.get(field):
                errors.append(f"{name}: missing provenance {field}")
        for field in ("source_url", "resolved_url"):
            url = urlsplit(entry.get(field, ""))
            if url.scheme != "https" or url.hostname not in ("www.ti.com", "ti.com"):
                errors.append(f"{name}: unexpected official source URL in {field}")
        try:
            data = artifact.read_bytes()
            total_bytes += len(data)
            if len(data) != entry.get("bytes"):
                errors.append(f"{name}: byte count differs")
            if hashlib.sha256(data).hexdigest() != entry.get("sha256"):
                errors.append(f"{name}: SHA-256 differs")
            if artifact.suffix == ".pdf":
                if not data.startswith(b"%PDF-") or b"%%EOF" not in data[-4096:]:
                    errors.append(f"{name}: invalid PDF signature/trailer")
                if not entry.get("pages") or not entry.get("derived_text"):
                    errors.append(f"{name}: missing PDF page/text metadata")
            elif artifact.suffix == ".zip":
                with zipfile.ZipFile(artifact) as archive:
                    if archive.testzip() is not None:
                        errors.append(f"{name}: ZIP CRC failure")
                    if archive.namelist() != entry.get("members"):
                        errors.append(f"{name}: ZIP member inventory differs")
            else:
                errors.append(f"{name}: unrecognized artifact format")
            if entry.get("derived_text"):
                text_path = (SOURCE / entry["derived_text"]).resolve()
                text_root = (ROOT / "docs/reference/text").resolve()
                if not text_path.is_relative_to(text_root):
                    errors.append(f"{name}: derived-text path leaves archive")
                    continue
                expected_text.add(text_path)
                text_data = text_path.read_bytes()
                text_data.decode("utf-8")
                if b"\x00" in text_data:
                    errors.append(f"{name}: NUL glyph prevents normal text searching")
                if hashlib.sha256(text_data).hexdigest() != entry.get("derived_text_sha256"):
                    errors.append(f"{name}: derived-text SHA-256 differs")
        except (OSError, UnicodeError, zipfile.BadZipFile) as exc:
            errors.append(f"{name}: {exc}")

    actual = {p.resolve() for p in SOURCE.iterdir() if p.is_file()}
    for path in sorted(actual - expected):
        errors.append(f"Unrecorded source artifact: {path.name}")
    actual_text = {p.resolve() for p in (ROOT / "docs/reference/text").iterdir() if p.is_file()}
    for path in sorted(actual_text - expected_text):
        errors.append(f"Unrecorded derived text: {path.name}")

    link_count = 0
    markdown_files = [p for p in ROOT.rglob("*.md") if ".git" not in p.parts]
    for doc in markdown_files:
        content = re.sub(r"```.*?```", "", doc.read_text(encoding="utf-8"), flags=re.S)
        # The repository uses inline Markdown links without titles or parentheses
        # in local filenames. Remote links and heading anchors are not checked.
        for target in re.findall(r"\[[^\]]*\]\(([^)]+)\)", content):
            target = target.strip().strip("<>")
            parsed = urlsplit(target)
            if parsed.scheme or target.startswith("#") or not parsed.path:
                continue
            link_count += 1
            destination = (doc.parent / unquote(parsed.path)).resolve()
            if not destination.is_relative_to(ROOT):
                errors.append(f"{doc.relative_to(ROOT)}: link leaves repository: {target}")
            elif not destination.exists():
                errors.append(f"{doc.relative_to(ROOT)}: missing local link: {target}")

    if not entries:
        errors.append("Reference inventory is empty")
    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1
    print(f"PASS: {len(entries)} vendor artifacts ({total_bytes:,} bytes), "
          f"{len(expected_text)} text extracts, {link_count} local links "
          f"in {len(markdown_files)} Markdown files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
