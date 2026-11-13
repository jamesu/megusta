#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../../.." && pwd)
wine_prefix=${WINEPREFIX:-/tmp/mgm-wineprefix}
if [ "${1:-}" = "--level" ]; then
  test "$#" -ge 2 || { echo "Usage: $0 --level CSL [-nointro ...]" >&2; exit 2; }
  case "$2" in ''|*[!A-Za-z0-9_]*) echo "Level must be a shipped script stem" >&2; exit 2;; esac
  export MEGUSTA_TEST_LEVEL="$2"
  shift 2
fi

python3 "$repo_root/megusta/tools/testing/wine_stage.py" stage-game \
  --source "$repo_root/MGM" \
  --wine-prefix "$wine_prefix" \
  --executable "$repo_root/megusta/build/win32-clang/megusta_game.exe"

# Run with a C:-backed working directory. The prefix intentionally has no Z:
# mapping, so launching from the repository host path makes Wine complain that
# its initial working directory is inaccessible.
cd "$wine_prefix/drive_c/megusta-tests/game"
log_file="$repo_root/megusta/build/win32-clang/game-launch.log"
mkdir -p "$(dirname -- "$log_file")"
set +e
env WINEPREFIX="$wine_prefix" MEGUSTA_NO_ERROR_DIALOG=1 wine megusta_game.exe "$@" >"$log_file" 2>&1
status=$?
set -e
for stream in stdout stderr; do
  if [ -f "$stream.txt" ]; then cp "$stream.txt" "$repo_root/megusta/build/win32-clang/game-$stream.log"; fi
done
cat "$log_file"
exit "$status"
