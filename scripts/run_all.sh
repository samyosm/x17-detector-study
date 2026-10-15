#!/usr/bin/env bash
# Builds the light map, then simulates Cs-137 (312), Co-60 (319), Bi-207 (308), Na-22 (332) and
# cosmic muons, one file each with its own seed. A failed run does not stop the rest.
#
#   scripts/run_all.sh                 # from anywhere
#   BUILD_DIR=build SEED=1729 scripts/run_all.sh
#
# Outputs: data/simulations/<lab-id>=<isotope>-<yyyy.mm.dd>-seed<seed>.root (+ sidecar files)
# Logs:    data/simulations/logs/
set -u
cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-build}"
CONFIG="${CONFIG:-config/lab_sources.toml}"
SEED="${SEED:-1729}"
BINARY="$BUILD_DIR/simulate_detection"
LOGS=data/simulations/logs
mkdir -p "$LOGS"

cmake --build "$BUILD_DIR" -j"$(nproc)" || { echo "Build failed"; exit 1; }

declare -a summary
run() { # name, then simulate_detection arguments
  local name="$1"; shift
  local start=$SECONDS
  echo "[$(date '+%F %T')] starting $name"
  "$BINARY" --config "$CONFIG" "$@" > "$LOGS/$name.log" 2>&1
  status=$?
  summary+=("$(printf '%-28s exit %d  %d min' "$name" "$status" $(((SECONDS - start) / 60)))")
  echo "[$(date '+%F %T')] finished $name (exit $status)"
}

run light-map --build-light-map
if [ "$status" -ne 0 ]; then
  echo "Light map failed; the lab and muon runs need it. See $LOGS/light-map.log"
  printf '%s\n' "${summary[@]}"
  exit 1
fi

seed=$SEED
for source in 312 319 308 332 muons; do
  run "$source" --source "$source" --seed "$seed"
  seed=$((seed + 1))
done

echo
echo "Summary"
printf '%s\n' "${summary[@]}"
ls -lh data/simulations/*.root
