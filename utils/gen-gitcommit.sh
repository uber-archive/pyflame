#!/bin/sh
#
# Generate a git commit tag.

if [ $# -ne 1 ]; then
  echo "usage: gen-gitcommit.sh OUTPUT"
  exit
fi

TARGET="$1"

if ! git describe --always >/dev/null 2>&1; then
  printf "#define BUILDTAG \"\"\\n" >"$TARGET"
  exit
fi

TAG="$(printf "#define BUILDTAG \" (git %s)\"\\n" "$(git describe --always)")"

if [ ! -f "$TARGET" ] || [ "$(cat "$TARGET")" != "$TAG" ]; then
  echo "$TAG" >"$TARGET"
fi
