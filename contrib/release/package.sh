#!/usr/bin/env bash
#
# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.
#
# Package a built tree into a release archive.
#
#   package.sh <build-dir> <host-triplet> <version>
#
# The host triplet is what depends calls the target, and it becomes the name of
# the archive -- quarlcoin-0.1.0-x86_64-linux-gnu.tar.gz and so on -- so that a
# person with two downloads in one directory can tell which is which without
# opening either.
#
# What this does not do is sign anything. A signature made on the machine that
# produced the binaries proves that the machine produced them, which is what the
# checksum already says. Signing belongs to a separate key on separate hardware,
# and pretending otherwise here would be worse than leaving it out.

set -euo pipefail

BUILD="${1:?usage: package.sh <build-dir> <host-triplet> <version>}"
HOST="${2:?}"
VERSION="${3:?}"

SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$SRC/release"
NAME="quarlcoin-$VERSION-$HOST"
STAGE="$OUT/$NAME"

BINARIES=(quarld quarl-cli quarl-tx quarl-util quarl-wallet quarl-miner quarl-qt)

# Windows binaries carry .exe and go in the same place; everything else is the
# same shape, so the layout does not fork on platform.
SUFFIX=""
case "$HOST" in *mingw32*) SUFFIX=".exe" ;; esac

rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/share"

missing=()
for b in "${BINARIES[@]}"; do
    found=""
    for candidate in "$BUILD/bin/$b$SUFFIX" "$BUILD/$b$SUFFIX" "$BUILD/src/$b$SUFFIX"; do
        [ -f "$candidate" ] && { found="$candidate"; break; }
    done
    if [ -n "$found" ]; then
        cp "$found" "$STAGE/bin/"
    else
        missing+=("$b")
    fi
done

# A missing GUI is a build without -DENABLE_GUI and not a failure; a missing
# daemon is a failure, and the difference is worth stating rather than leaving
# the archive quietly short.
for m in "${missing[@]}"; do
    if [ "$m" = "quarl-qt" ]; then
        echo "note: quarl-qt not in $BUILD -- packaging without the GUI" >&2
    else
        echo "error: $m not found in $BUILD" >&2
        exit 1
    fi
done

cp "$SRC/README.md" "$SRC/COPYING" "$STAGE/"
cp "$SRC/share/examples/quarlcoin.conf" "$STAGE/share/"
[ -d "$SRC/doc" ] && cp -r "$SRC/doc" "$STAGE/doc"

strip "$STAGE"/bin/* 2>/dev/null || true

mkdir -p "$OUT"
cd "$OUT"
case "$HOST" in
    *mingw32*) rm -f "$NAME.zip";     zip -qr "$NAME.zip" "$NAME";     ARCHIVE="$NAME.zip" ;;
    *)         rm -f "$NAME.tar.gz";  tar czf "$NAME.tar.gz" "$NAME";  ARCHIVE="$NAME.tar.gz" ;;
esac

# Replace this archive's line rather than appending one. Appending leaves two
# sums for one file after a rebuild, and a reader has no way to tell which is
# the release -- both look equally official.
touch SHA256SUMS
grep -v "  $ARCHIVE\$" SHA256SUMS > SHA256SUMS.new || true
sha256sum "$ARCHIVE" >> SHA256SUMS.new
sort -k2 -o SHA256SUMS SHA256SUMS.new
rm -f SHA256SUMS.new

echo "$OUT/$ARCHIVE"
ls -lh "$ARCHIVE" | awk '{print "  " $5, $9}'
echo "  binaries: $(ls "$STAGE/bin" | tr '\n' ' ')"
