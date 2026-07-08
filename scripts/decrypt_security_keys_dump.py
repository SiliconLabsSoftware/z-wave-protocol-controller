#!/usr/bin/env python3
"""Decrypt a ZPC encrypted security-keys dump (envelope).

The ZPC seals its S2 network keys with an X25519 recipient public key and
writes the envelope to disk as `<home_id>.bin`. This script decrypts that
envelope, given the matching recipient private key, and writes two output
files in the chosen directory named `<home_id>.yaml` and `<home_id>.txt`:

  - <home_id>.yaml  Human-readable YAML map of `<class_name>: '<hex_key>'`.
  - <home_id>.txt   Legacy Zniffer-friendly key list, one key per line as
                    `<class_id>;<hex_key>;1`. The class id is `98` for the
                    S0 network key and `9F` for every S2 class.

By default, the outputs are written next to `--in`. Pass `--out <output_dir>`
to write them into a specific directory (which must already exist).

see `components/security/doc/security_keys_dump_mqtt_api.md` for details.
"""

import argparse
import re
import sys
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PublicKey

MAGIC = b"ZPCK"
MAGIC_LEN = len(MAGIC)
VERSION_LEN = 1
EPH_PUB_LEN = 32
IV_LEN = 12
TAG_LEN = 16
HEADER_LEN = MAGIC_LEN + VERSION_LEN + EPH_PUB_LEN + IV_LEN

# Maps each supported envelope version to its required HKDF "info" string.
# Single source of truth for the (version, hkdf_info) pairing on the decryptor side
# so a future maintainer cannot bump one without bumping the other.
INFO_BY_VERSION = {
    0x01: b"zpc-keydump-v1",
}

# "<class_name>: '<32 uppercase hex chars>'\n".
_KEY_LINE_RE = re.compile(rb"^([A-Z0-9_]+):\s*'([0-9A-F]{32})'\s*$")

_HOME_ID_RE = re.compile(r"^[0-9A-F]{8}$")

_ZNIFFER_CLASS_ID = {
    b"S0": b"98",
}
_ZNIFFER_DEFAULT_CLASS_ID = b"9F"


def yaml_to_zniffer(plaintext: bytes) -> bytes:
    lines = []
    for raw_line in plaintext.splitlines():
        if not raw_line.strip():
            continue
        m = _KEY_LINE_RE.match(raw_line)
        if not m:
            raise ValueError(f"unrecognized plaintext line: {raw_line!r}")
        class_name, hex_key = m.group(1), m.group(2)
        class_id = _ZNIFFER_CLASS_ID.get(class_name, _ZNIFFER_DEFAULT_CLASS_ID)
        lines.append(class_id + b";" + hex_key + b";1")
    return b"\n".join(lines) + (b"\n" if lines else b"")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--priv", required=True, type=Path, help="PEM-encoded X25519 recipient private key.")
    parser.add_argument("--in", dest="inp", required=True, type=Path, help="Encrypted dump (envelope) file.")
    parser.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Directory where <home_id>.yaml and <home_id>.txt are written. "
             "Defaults to the parent directory of --in. Must already exist.",
    )
    args = parser.parse_args()

    home_id = args.inp.stem
    if not _HOME_ID_RE.match(home_id):
        print(
            f"error: --in basename {args.inp.name!r} does not match <home_id>.bin "
            f"(<home_id> = 8 uppercase hex chars)",
            file=sys.stderr,
        )
        return 2

    priv = serialization.load_pem_private_key(args.priv.read_bytes(), password=None)
    if not isinstance(priv, X25519PrivateKey):
        print(f"error: --priv {args.priv} is not a valid X25519 recipient private key", file=sys.stderr)
        return 2

    dump = args.inp.read_bytes()
    if len(dump) < HEADER_LEN + TAG_LEN:
        print(f"error: envelope too short ({len(dump)} bytes)", file=sys.stderr)
        return 2

    pos = 0
    if dump[pos : pos + MAGIC_LEN] != MAGIC:
        print(f"error: invalid magic ({dump[pos:pos+MAGIC_LEN]!r}, expected {MAGIC!r})", file=sys.stderr)
        return 2
    pos += MAGIC_LEN

    version = dump[pos]
    pos += VERSION_LEN
    hkdf_info = INFO_BY_VERSION.get(version)
    if hkdf_info is None:
        supported = ", ".join(f"0x{v:02x}" for v in sorted(INFO_BY_VERSION))
        print(
            f"error: unsupported envelope version 0x{version:02x}; supported: {supported}",
            file=sys.stderr,
        )
        return 2

    eph_pub = dump[pos : pos + EPH_PUB_LEN]
    pos += EPH_PUB_LEN
    iv = dump[pos : pos + IV_LEN]
    pos += IV_LEN
    ct_and_tag = dump[pos:]
    ct, tag = ct_and_tag[:-TAG_LEN], ct_and_tag[-TAG_LEN:]

    recipient_pub = priv.public_key().public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw,
    )

    eph_pub_key = X25519PublicKey.from_public_bytes(eph_pub)
    shared = priv.exchange(eph_pub_key)
    aes_key = HKDF(
        algorithm=hashes.SHA256(),
        length=32,
        salt=eph_pub + recipient_pub,
        info=hkdf_info,
    ).derive(shared)

    aad = MAGIC + bytes([version]) + eph_pub + recipient_pub
    decryptor = Cipher(algorithms.AES(aes_key), modes.GCM(iv, tag)).decryptor()
    decryptor.authenticate_additional_data(aad)
    try:
        plaintext = decryptor.update(ct) + decryptor.finalize()
    except Exception as exc:
        print(f"error: decryption failed: {exc}", file=sys.stderr)
        return 1

    try:
        zniffer = yaml_to_zniffer(plaintext)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    out_dir = args.out if args.out is not None else args.inp.parent
    if not out_dir.is_dir():
        print(f"error: --out {out_dir} is not an existing directory", file=sys.stderr)
        return 2

    yaml_path = out_dir / (home_id + ".yaml")
    txt_path = out_dir / (home_id + ".txt")

    yaml_path.write_bytes(plaintext)
    txt_path.write_bytes(zniffer)
    return 0


if __name__ == "__main__":
    sys.exit(main())
