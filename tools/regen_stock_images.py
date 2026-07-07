#!/usr/bin/env python3
"""
Regenerate the stock image data table in
src/server/tools/nxdbmgr/stock_images.cpp by embedding base64-encoded PNG
payloads from the source images/ directory.

Stock image metadata (guid -> (name, category, mime)) is fixed; this
script just refreshes the marker-delimited s_stockImages[] block from the
files on disk.

Run from repository root:
    python3 tools/regen_stock_images.py
"""

import base64
import os
import sys

# (guid, name, category, mimetype) — preserve insertion order
STOCK_IMAGES = [
    ("092e4b35-4e7c-42df-b9b7-d5805bfac64e", "Service",  "Network Objects", "image/png"),
    ("1ddb76a3-a05f-4a42-acda-22021768feaf", "ATM",      "Network Objects", "image/png"),
    ("7cd999e9-fbe0-45c3-a695-f84523b3a50c", "Unknown",  "Network Objects", "image/png"),
    ("904e7291-ee3f-41b7-8132-2bd29288ecc8", "Node",     "Network Objects", "image/png"),
    ("b314cf44-b2aa-478e-b23a-73bc5bb9a624", "HSM",      "Network Objects", "image/png"),
    ("ba6ab507-f62d-4b8f-824c-ca9d46f22375", "Server",   "Network Objects", "image/png"),
    ("bacde727-b183-4e6c-8dca-ab024c88b999", "Router",   "Network Objects", "image/png"),
    ("f5214d16-1ab1-4577-bb21-063cfd45d7af", "Printer",  "Network Objects", "image/png"),
    ("f9105c54-8dcf-483a-b387-b4587dfd3cba", "Switch",   "Network Objects", "image/png"),
]

BEGIN_MARKER = "   // BEGIN GENERATED STOCK IMAGE DATA (tools/regen_stock_images.py)\n"
END_MARKER = "   // END GENERATED STOCK IMAGE DATA\n"


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    images_dir = os.path.join(root, "images")
    target = os.path.join(root, "src", "server", "tools", "nxdbmgr", "stock_images.cpp")

    with open(target, "r", encoding="utf-8") as f:
        source = f.read()

    begin = source.index(BEGIN_MARKER) + len(BEGIN_MARKER)
    end = source.index(END_MARKER)

    lines = []
    for guid, name, category, mime in STOCK_IMAGES:
        path = os.path.join(images_dir, guid)
        with open(path, "rb") as f:
            payload = base64.b64encode(f.read()).decode("ascii")
        lines.append(f"   // {name}")
        lines.append(f'   {{ L"{guid}", L"{name}", L"{category}", "{mime}",')
        lines.append(f'     "{payload}" }},')

    with open(target, "w", encoding="utf-8", newline="\n") as out:
        out.write(source[:begin] + "\n".join(lines) + "\n" + source[end:])

    print(f"Updated {target} ({len(STOCK_IMAGES)} images)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
