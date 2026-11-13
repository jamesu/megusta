#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../../.." && pwd)
wine_prefix=${WINEPREFIX:-/tmp/mgm-wineprefix}
wine_bin=${WINE_BIN:-wine}
game_exe=${MEGUSTA_GAME_EXE:-$repo_root/megusta/build/win64-clang/megusta_game.exe}
wine_prefix=$(CDPATH= cd -- "$wine_prefix" && pwd)

if [ "${1:-}" = "--level" ]; then
  test "$#" -ge 2 || { echo "Usage: $0 --level CSL [-nointro ...]" >&2; exit 2; }
  case "$2" in ''|*[!A-Za-z0-9_]*) echo "Level must be a shipped script stem" >&2; exit 2;; esac
  export MEGUSTA_TEST_LEVEL="$2"
  shift 2
fi

python3 "$repo_root/megusta/tools/testing/wine_stage.py" stage-game \
  --architecture win64 \
  --source "$repo_root/MGM" \
  --wine-prefix "$wine_prefix" \
  --executable "$game_exe"

# Run from the staged C: tree. wine_stage.py verifies a win64 prefix with only
# its own drive_c mapped as C:, and copies the executable, assets and x64 DLLs.
cd "$wine_prefix/drive_c/megusta-tests/game"
log_file="$repo_root/megusta/build/win64-clang/game-launch.log"
mkdir -p "$(dirname -- "$log_file")"
set +e
env WINEPREFIX="$wine_prefix" MEGUSTA_NO_ERROR_DIALOG=1 \
  "$wine_bin" megusta_game.exe "$@" >"$log_file" 2>&1
status=$?
set -e
for stream in stdout stderr; do
  if [ -f "$stream.txt" ]; then
    cp "$stream.txt" "$repo_root/megusta/build/win64-clang/game-$stream.log"
  fi
done
cat "$log_file"
exit "$status"
