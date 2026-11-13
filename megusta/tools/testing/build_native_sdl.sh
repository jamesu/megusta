#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
case "$(uname -s)" in
  Darwin) preset=macos-sdl ;;
  Linux) preset=linux-sdl ;;
  *) echo "Native SDL builds support macOS and Linux" >&2; exit 1 ;;
esac
cd "$project_dir"
cmake --preset "$preset" "$@"
cmake --build --preset "$preset" --parallel "${MEGUSTA_BUILD_JOBS:-8}"
