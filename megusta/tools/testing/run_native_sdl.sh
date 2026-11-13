#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
if [ "$#" -lt 1 ]; then echo "Usage: $0 ASSET_DIRECTORY [game arguments...]" >&2; exit 1; fi
asset_dir=$(CDPATH= cd -- "$1" && pwd)
shift
case "$(uname -s)" in
  Darwin) preset=macos-sdl ;;
  Linux) preset=linux-sdl ;;
  *) echo "Native SDL builds support macOS and Linux" >&2; exit 1 ;;
esac
executable="$project_dir/build/$preset/megusta_game"
if [ ! -x "$executable" ]; then echo "Build first: tools/testing/build_native_sdl.sh" >&2; exit 1; fi
run_dir="$project_dir/build/$preset/run"
mkdir -p "$run_dir"
# Copy assets into a writable session directory. Never write preferences into
# the shipped source directory, and preserve existing session preferences.
for source in "$asset_dir"/*; do
  [ -e "$source" ] || continue
  name=$(basename -- "$source")
  [ "$name" != out ] || continue
  if [ ! -e "$run_dir/$name" ]; then cp -R "$source" "$run_dir/$name"; fi
done
cd "$run_dir"
exec "$executable" "$@"
