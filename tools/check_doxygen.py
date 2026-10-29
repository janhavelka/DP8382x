#!/usr/bin/env python3
"""Check actual generated public API documentation using Doxygen and stdlib only.

Run from any directory: python tools/check_doxygen.py
Doxygen must be on PATH. Each invocation has a 120-second timeout. HTML/XML and
two negative source fixtures live only in an automatically removed temporary
directory; this check publishes no documentation or last-good cache. External
links are not fetched. Success establishes documentation structure and warning
coverage, not contract correctness or hardware behavior.
"""

from html.parser import HTMLParser
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from urllib.parse import unquote, urlsplit
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
TIMEOUT_SECONDS = 120
REQUIRED_TYPES = {"DP8382x::Status", "DP8382x::Config", "DP8382x::LinkSettings",
                  "DP8382x::MdioFrame", "DP8382x::MdioBatch", "DP8382x::BatchResult",
                  "DP8382x::Transport"}


class HtmlPage(HTMLParser):
    """Collect generated local references and named anchors without fetching URLs."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.links = []
        self.anchors = set()

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        for name in ("href", "src"):
            if values.get(name):
                self.links.append(values[name])
        if values.get("id"):
            self.anchors.add(values["id"])
        if tag == "a" and values.get("name"):
            self.anchors.add(values["name"])


def check_html(html):
    """Require existing generated targets and HTML anchors within the output tree."""
    pages = {}
    for path in html.rglob("*.html"):
        page = HtmlPage()
        page.feed(path.read_text(encoding="utf-8"))
        pages[path.resolve()] = page
    errors = set()
    if (html / "index.html") not in pages:
        errors.add("Missing HTML landing page")
    for path, page in pages.items():
        for link in page.links:
            url = urlsplit(link)
            if url.scheme or url.netloc:
                continue
            target = (path.parent / unquote(url.path)).resolve() if url.path else path
            label = f"{path.relative_to(html)}: {link}"
            if not target.is_relative_to(html) or not target.is_file():
                errors.add(f"Missing local document or asset: {label}")
            elif (url.fragment and target in pages and
                  unquote(url.fragment) not in pages[target].anchors):
                errors.add(f"Missing local anchor: {label}")
    return pages, errors


def has_documentation(element):
    """Test source-derived descriptions, independently of page extraction."""
    return any("".join(element.itertext()).strip()
               for element in element.findall("briefdescription") +
               element.findall("detaileddescription"))


def check_api(output, pages):
    """Require header/type pages, public comments, and rendered member anchors."""
    html = output / "html"
    xml = output / "xml"
    errors = set()
    files = set()
    public_types = set()
    for entry in ET.parse(xml / "index.xml").getroot().findall("compound"):
        refid = entry.attrib["refid"]
        compound = ET.parse(xml / f"{refid}.xml").getroot().find("compounddef")
        name = compound.findtext("compoundname")
        kind = compound.get("kind")
        path = html / f"{refid}.html"
        if kind == "file":
            files.add(compound.find("location").attrib["file"].replace("\\", "/"))
        if compound.get("prot") == "private":
            if path in pages:
                errors.add(f"Private type exposed in HTML: {name}")
            continue
        if kind not in {"file", "class", "struct", "namespace"}:
            continue
        if kind in {"class", "struct"}:
            public_types.add(name)
        if path not in pages:
            errors.add(f"Public API page missing: {name}")
        if kind != "namespace" and not has_documentation(compound):
            errors.add(f"Missing public documentation: {name}")
        for member in compound.findall("sectiondef/memberdef"):
            member_name = member.findtext("qualifiedname") or member.findtext("name")
            # XML can contain private members even when HTML extraction omits them.
            member_id = member.attrib["id"]
            owner, anchor = member_id.rsplit("_1", 1)
            member_page = pages.get(html / f"{owner}.html")
            visible = member_page is not None and anchor in member_page.anchors
            if member.get("prot") == "private":
                if visible:
                    errors.add(f"Private member exposed in HTML: {member_name}")
                continue
            if not has_documentation(member):
                errors.add(f"Missing public documentation: {member_name}")
            if not visible:
                errors.add(f"Public member missing in HTML: {member_name}")
            for value in member.findall("enumvalue"):
                if not has_documentation(value):
                    errors.add(f"Missing enum-value documentation: {member_name}::{value.findtext('name')}")
    for missing in sorted(REQUIRED_TYPES - public_types):
        errors.add(f"Required public type missing: {missing}")
    headers = ROOT / "include" / "DP8382x"
    expected = {path.relative_to(ROOT).as_posix() for path in headers.rglob("*.h")
                if not {"private", "vendor", "build"}.intersection(path.relative_to(headers).parts)}
    for missing in sorted(expected - files):
        errors.add(f"Public header missing from documentation: {missing}")
    for unexpected in sorted(files - expected):
        errors.add(f"Unexpected implementation, vendor or build input: {unexpected}")
    return errors


def run_doxygen(executable, configuration, output, source=None):
    """Run one finite generation, optionally replacing inputs with a test fixture."""
    configuration += (f'\nOUTPUT_DIRECTORY = "{output.as_posix()}"\n'
                      'GENERATE_XML = YES\nWARN_LOGFILE =\nWARN_AS_ERROR = YES\n')
    if source is not None:
        configuration += f'INPUT = "{source.as_posix()}"\n'
    return subprocess.run([executable, "-"], input=configuration, cwd=ROOT,
                          text=True, capture_output=True, encoding="utf-8",
                          errors="replace", timeout=TIMEOUT_SECONDS)


def check_negative_fixtures(executable, configuration, workspace):
    """Prove missing public comments and invalid parameter names cause failure."""
    fixtures = (
        ("missing-docs", """/// @file fixture.h
/// @brief Missing-documentation rejection fixture.
namespace Fixture {
int undocumentedPublicFunction(int value);
}
""", "undocumentedPublicFunction", "not documented"),
        ("invalid-param", """/// @file fixture.h
/// @brief Invalid-parameter rejection fixture.
namespace Fixture {
/// @brief Echo a caller value.
/// @param[in] nonexistent Parameter deliberately absent from the declaration.
/// @return The caller value.
int documentedPublicFunction(int value);
}
""", "nonexistent", "not found"),
    )
    for name, source, symbol, diagnostic in fixtures:
        directory = workspace / name
        directory.mkdir()
        fixture = directory / "fixture.h"
        fixture.write_text(source, encoding="utf-8")
        result = run_doxygen(executable, configuration, directory / "output", fixture)
        messages = result.stdout + result.stderr
        if result.returncode == 0 or symbol not in messages or diagnostic not in messages:
            raise ValueError(f"Negative fixture {name} did not fail for the expected reason:\n{messages}")


def main():
    executable = shutil.which("doxygen")
    if executable is None:
        print("BLOCKED: Doxygen is required on PATH; run python tools/check_doxygen.py "
              "after installing it.", file=sys.stderr)
        return 1
    configuration = (ROOT / "Doxyfile").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="dp8382x-doxygen-") as temporary:
        workspace = Path(temporary).resolve()
        output = workspace / "generated"
        result = run_doxygen(executable, configuration, output)
        if result.returncode or result.stderr.strip():
            print(result.stdout + result.stderr, file=sys.stderr)
            return 1
        pages, errors = check_html(output / "html")
        errors.update(check_api(output, pages))
        if errors:
            print("\n".join(sorted(errors)), file=sys.stderr)
            return 1
        check_negative_fixtures(executable, configuration, workspace)
    print(f"PASS: {len(pages)} generated HTML pages; public comments/pages, local "
          "links/assets, and 2 rejected negative fixtures. Temporary output removed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.TimeoutExpired:
        print(f"Doxygen check failed: generation exceeded {TIMEOUT_SECONDS} seconds.",
              file=sys.stderr)
        raise SystemExit(1)
    except (OSError, ValueError, ET.ParseError) as error:
        print(f"Doxygen check failed: {error}", file=sys.stderr)
        raise SystemExit(1)
