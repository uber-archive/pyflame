#!/bin/bash
#
# Generate a manual page from man.md, using pandoc(1). The generated man page is
# checked in, so that users don't need to install pandoc to build/package
# Pyflame.

set -eu

SILENT=0
BASEDIR="$(dirname "${BASH_SOURCE[0]}")"
OUTPUT="${BASEDIR}/pyflame.man"

while getopts ":ho:s" opt; do
  case $opt in
    h)
      echo "Usage: $0 [-h] [-s] [-o OUTPUT]"
      exit
      ;;
    o)
      OUTPUT="$OPTARG"
      ;;
    s)
      SILENT=1
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

pandoc "${BASEDIR}/man.md" -M date="$(date '+%B %Y')" -s -t man -o "$OUTPUT"

if [ "$SILENT" -eq 0 ]; then
  man -l "$OUTPUT"
fi
