#!/usr/bin/env python3
"""Validate Frox option_info[] persistence identifiers."""

from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path


ENTRY_RE = re.compile(
    r"\{\s*&(?P<var>[A-Za-z0-9_]+)\s*,\s*"
    r"(?P<norm>TRUE|FALSE)\s*,\s*"
    r"(?P<page>OPT_PAGE_[A-Z]+)\s*,\s*"
    r"(?P<set>\d+)\s*,\s*"
    r"(?P<bit>\d+)\s*,\s*\n\s*"
    r'"(?P<text>[^"]*)"\s*,\s*"(?P<desc>[^"]*)"',
    re.MULTILINE,
)


def _source_root() -> Path:
    if len(sys.argv) > 1:
        return Path(sys.argv[1]).resolve()

    cwd = Path.cwd().resolve()
    if (cwd / "src" / "tables.c").is_file():
        return cwd

    sibling = Path(__file__).resolve().parents[1] / "froxcomposband"
    if (sibling / "src" / "tables.c").is_file():
        return sibling

    raise RuntimeError("Could not locate Frox source root; pass it as an argument")


def _option_table(source: str) -> str:
    try:
        start = source.index("option_type option_info[]")
        end = source.index("{ NULL", start)
    except ValueError as exc:
        raise RuntimeError("Could not locate option_info[] table in src/tables.c") from exc
    return source[start:end]


def main() -> int:
    tables_path = _source_root() / "src" / "tables.c"
    table = _option_table(tables_path.read_text())
    entries = []

    for match in ENTRY_RE.finditer(table):
        data = match.groupdict()
        data["set"] = int(data["set"])
        data["bit"] = int(data["bit"])
        entries.append(data)

    if not entries:
        print("ERROR: No option_info[] entries parsed from src/tables.c", file=sys.stderr)
        return 1

    errors = []
    slots = defaultdict(list)
    names = defaultdict(list)

    for entry in entries:
        if not 0 <= entry["set"] <= 7:
            errors.append(f"{entry['text']}: o_set {entry['set']} is outside 0..7")
        if not 0 <= entry["bit"] <= 31:
            errors.append(f"{entry['text']}: o_bit {entry['bit']} is outside 0..31")
        slots[(entry["set"], entry["bit"])].append(entry)
        names[entry["text"]].append(entry)

    for slot, matches in sorted(slots.items()):
        if len(matches) > 1:
            details = ", ".join(f"{m['text']} (&{m['var']})" for m in matches)
            errors.append(f"Duplicate option slot {slot}: {details}")

    for name, matches in sorted(names.items()):
        if len(matches) > 1:
            details = ", ".join(f"({m['set']},{m['bit']}) &{m['var']}" for m in matches)
            errors.append(f"Duplicate option key {name!r}: {details}")

    if errors:
        print("Option table validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print(f"Option table validation passed ({len(entries)} options).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
