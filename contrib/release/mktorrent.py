#!/usr/bin/env python3
# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.
"""Make a .torrent for a release file, with an HTTP web seed.

There is no torrent tool on either machine here and no root to install one, so
this writes the file itself. Bencode is four types and forty lines; a dependency
that has to be installed on the machine that publishes releases is worse.

The web seed is the point. A swarm with no seeder is a dead link, and keeping a
seeding daemon alive is one more thing that fails quietly. BEP 19 lets the
torrent name an HTTP URL that every major client will pull from, so the download
works from the first minute with nobody seeding, and peers still trade pieces
with each other instead of all leaning on the server.

  mktorrent.py FILE --webseed URL [--tracker URL ...] [-o OUT] [--comment TEXT]

Piece length defaults to whatever keeps the torrent file small without making
pieces so large that a slow peer holds one forever: 2 MiB up to 4 GiB, which is
what most clients pick for a release-sized file.
"""

import argparse
import hashlib
import math
import os
import sys


def bencode(v):
    """Bencode a value. Dict keys are emitted in the sorted order the spec wants:
    two encoders that sort differently produce two different infohashes, which
    means two swarms for one file."""
    if isinstance(v, int):
        return b"i" + str(v).encode() + b"e"
    if isinstance(v, bytes):
        return str(len(v)).encode() + b":" + v
    if isinstance(v, str):
        return bencode(v.encode("utf-8"))
    if isinstance(v, (list, tuple)):
        return b"l" + b"".join(bencode(x) for x in v) + b"e"
    if isinstance(v, dict):
        out = b"d"
        for k in sorted(v, key=lambda k: k.encode("utf-8") if isinstance(k, str) else k):
            out += bencode(k) + bencode(v[k])
        return out + b"e"
    raise TypeError(f"cannot bencode {type(v).__name__}")


def piece_length_for(size):
    # Aim for a few thousand pieces: enough that a peer can share early, few
    # enough that the piece hashes stay a sensible fraction of the torrent file.
    target = 1 << 21  # 2 MiB
    while size // target > 8192:
        target <<= 1
    return target


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file")
    ap.add_argument("--webseed", action="append", default=[],
                    help="HTTP URL serving this exact file (BEP 19). Repeatable.")
    ap.add_argument("--tracker", action="append", default=[],
                    help="Announce URL. Repeatable. Without one the torrent is "
                         "DHT-only, which works but finds peers slowly.")
    ap.add_argument("--comment", default=None)
    ap.add_argument("--piece-length", type=int, default=None,
                    help="Bytes, must be a power of two.")
    ap.add_argument("-o", "--output", default=None)
    args = ap.parse_args()

    path = args.file
    size = os.path.getsize(path)
    name = os.path.basename(path)
    plen = args.piece_length or piece_length_for(size)
    if plen & (plen - 1):
        sys.exit("piece length must be a power of two")

    pieces = bytearray()
    with open(path, "rb") as f:
        while True:
            block = f.read(plen)
            if not block:
                break
            pieces += hashlib.sha1(block).digest()

    info = {
        "name": name,
        "length": size,
        "piece length": plen,
        "pieces": bytes(pieces),
    }
    infohash = hashlib.sha1(bencode(info)).hexdigest()

    torrent = {"info": info}
    if args.tracker:
        torrent["announce"] = args.tracker[0]
        # Every tracker in its own tier, so a client tries them all rather than
        # picking one at random out of a shared tier and stopping there.
        torrent["announce-list"] = [[t] for t in args.tracker]
    if args.webseed:
        torrent["url-list"] = args.webseed if len(args.webseed) > 1 else args.webseed[0]
    if args.comment:
        torrent["comment"] = args.comment
    torrent["created by"] = "Quarlcoin contrib/release/mktorrent.py"

    out = args.output or (path + ".torrent")
    with open(out, "wb") as f:
        f.write(bencode(torrent))

    print(f"{out}")
    print(f"  name          {name}")
    print(f"  size          {size} bytes ({size / 2**20:.1f} MiB)")
    print(f"  piece length  {plen} ({plen // 1024} KiB), {math.ceil(size / plen)} pieces")
    print(f"  infohash      {infohash}")
    print(f"  sha256        {hashlib.sha256(open(path, 'rb').read()).hexdigest()}")
    for w in args.webseed:
        print(f"  web seed      {w}")
    for t in args.tracker:
        print(f"  tracker       {t}")
    print()
    print(f"magnet:?xt=urn:btih:{infohash}&dn={name}"
          + "".join(f"&tr={t}" for t in args.tracker)
          + "".join(f"&ws={w}" for w in args.webseed))


if __name__ == "__main__":
    main()
