#!/usr/bin/env python3

from pathlib import Path
import json
import re
import sys


SUPPORTED_EXTENSIONS = {
    ".png",
    ".jpg",
    ".jpeg",
    ".wav",
    ".ogg",
    ".mp3",
    ".flac",
    ".ttf",
    ".otf",
}


def sanitize_name(name: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_]", "_", name)


def generate_header(output: Path):
    content = """#pragma once

#include <cstddef>
#include <string_view>

namespace embedded_assets {

struct Asset {
    const char* path;
    const unsigned char* data;
    std::size_t size;
};

const Asset* Find(std::string_view path);

}
"""

    output.write_text(content, encoding="utf-8")


def generate_source(asset_dir: Path, output: Path):

    files = sorted(
        (
            p for p in asset_dir.rglob("*")
            if p.is_file()
            and p.suffix.lower() in SUPPORTED_EXTENSIONS
            and p.stat().st_size > 0
        ),
        key=lambda p: p.relative_to(asset_dir).as_posix()
    )

    lines = [
        '#include "embedded_assets.hpp"',
        "",
        "namespace embedded_assets {",
        "",
    ]

    entries = []

    for index, path in enumerate(files):

        relative = path.relative_to(asset_dir).as_posix()
        identifier = f"asset_{index:04d}_{sanitize_name(path.stem)}"

        data = path.read_bytes()

        lines.append(f"// {relative}")
        lines.append(
            f"static constexpr unsigned char {identifier}[] = {{"
        )

        for i in range(0, len(data), 16):

            chunk = data[i:i + 16]

            values = ", ".join(
                f"0x{byte:02X}" for byte in chunk
            )

            lines.append(f"    {values},")

        lines.append("};")
        lines.append("")

        entries.append(
            (relative, identifier, len(data))
        )

    lines.append("namespace {")

    if entries:

        lines.append("static constexpr Asset assets[] = {")

        for relative, identifier, size in entries:

            escaped_path = json.dumps(relative)

            lines.append(
                f"    {{{escaped_path}, {identifier}, {size}}},"
            )

        lines.append("};")

    else:
        lines.append("static constexpr Asset assets[] = {};")

    lines.extend([
        "",
        "}",
        "",
        "const Asset* Find(std::string_view path) {",
        "    for (const auto& asset : assets) {",
        "        if (asset.path && path == asset.path)",
        "            return &asset;",
        "    }",
        "",
        "    return nullptr;",
        "}",
        "",
        "}",
    ])

    output.write_text("\n".join(lines) + "\n", encoding="utf-8")

    total_size = sum(size for _, _, size in entries)

    print(f"Embedded assets: {len(entries)}")
    print(f"Total asset size: {total_size:,} bytes")
    print(f"Generated: {output}")


def main():

    if len(sys.argv) != 4:
        print(
            "Usage: embed_assets.py <asset_dir> "
            "<output.cpp> <output.hpp>"
        )
        sys.exit(1)

    asset_dir = Path(sys.argv[1]).resolve()
    output_cpp = Path(sys.argv[2]).resolve()
    output_hpp = Path(sys.argv[3]).resolve()

    if not asset_dir.is_dir():
        raise SystemExit(f"Asset directory not found: {asset_dir}")

    output_cpp.parent.mkdir(parents=True, exist_ok=True)
    output_hpp.parent.mkdir(parents=True, exist_ok=True)

    generate_header(output_hpp)
    generate_source(asset_dir, output_cpp)


if __name__ == "__main__":
    main()