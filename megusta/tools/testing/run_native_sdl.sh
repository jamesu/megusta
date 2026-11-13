#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
if [ "$#" -lt 1 ]; then echo "Usage: $0 GAME_DIRECTORY [game arguments...]" >&2; exit 1; fi
game_dir=$(CDPATH= cd -- "$1" && pwd)
shift
case "$(uname -s)" in
  Darwin) preset=macos-sdl ;;
  Linux) preset=linux-sdl ;;
  *) echo "Native SDL builds support macOS and Linux" >&2; exit 1 ;;
esac
executable="$project_dir/build/$preset/megusta_game"
if [ ! -x "$executable" ]; then echo "Build first: tools/testing/build_native_sdl.sh" >&2; exit 1; fi
# Run in the supplied game folder so startup finds its minigolf.cfg and all
# relative game resources resolve from the same location.
cd "$game_dir"
exec "$executable" "$@"
