#!/usr/bin/env python3
import argparse
import base64
from pathlib import Path
from urllib.request import urlopen


KEYS_BY_CODENAME = {
    "bullseye": (
        "https://ftp-master.debian.org/keys/archive-key-11.asc",
        "https://ftp-master.debian.org/keys/archive-key-11-security.asc",
        "https://ftp-master.debian.org/keys/release-11.asc",
    ),
    "bookworm": (
        "https://ftp-master.debian.org/keys/archive-key-12.asc",
        "https://ftp-master.debian.org/keys/archive-key-12-security.asc",
        "https://ftp-master.debian.org/keys/release-12.asc",
    ),
    "trixie": (
        "https://ftp-master.debian.org/keys/archive-key-13.asc",
        "https://ftp-master.debian.org/keys/archive-key-13-security.asc",
        "https://ftp-master.debian.org/keys/release-13.asc",
    ),
}


def fetch_text(url: str) -> str:
    with urlopen(url) as response:
        return response.read().decode("utf-8")


def dearmor(armored_text: str) -> bytes:
    begin_marker = "-----BEGIN PGP PUBLIC KEY BLOCK-----"
    end_marker = "-----END PGP PUBLIC KEY BLOCK-----"
    output = bytearray()
    cursor = 0

    while True:
        begin = armored_text.find(begin_marker, cursor)
        if begin == -1:
            break

        end = armored_text.find(end_marker, begin)
        if end == -1:
            raise ValueError("Malformed ASCII-armored OpenPGP block")

        block = armored_text[begin + len(begin_marker):end]
        lines = [line.strip() for line in block.splitlines()]
        payload_lines = []
        body_started = False

        for line in lines:
            if not line:
                if not body_started:
                    body_started = True
                continue
            if line.startswith("="):
                break
            if not body_started and ":" in line:
                continue
            body_started = True
            payload_lines.append(line)

        if not payload_lines:
            raise ValueError("Missing OpenPGP payload")

        output.extend(base64.b64decode("".join(payload_lines), validate=True))
        cursor = end + len(end_marker)

    if not output:
        raise ValueError("No OpenPGP key blocks found")

    return bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--codename", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    try:
        urls = KEYS_BY_CODENAME[args.codename]
    except KeyError as exc:
        raise SystemExit(f"Unsupported Debian codename for key bootstrap: {args.codename}") from exc

    armored = "\n".join(fetch_text(url) for url in urls)
    dearmored = dearmor(armored)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = output_path.with_suffix(output_path.suffix + ".tmp")
    tmp_path.write_bytes(dearmored)
    tmp_path.replace(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
