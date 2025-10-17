#!/usr/bin/env bash
set -euo pipefail

# fast build driver: configure + build using preset fast-debug
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# Ensure ccache environment mildly tuned
export CCACHE_CPP2=yes
# Optionally tune CCACHE_DIR:
# export CCACHE_DIR="${HOME}/.ccache"

# configure (will reuse existing config if unchanged)
echo "==> cmake configure --preset fast-debug"
cmake --preset fast-debug

# build meow-vm (only the executable target) with maximum parallelism
NPROCS="$(nproc || echo 2)"
echo "==> build (ninja -j${NPROCS})"
cmake --build --preset fast-debug -- -j"${NPROCS}" meow-vm

echo
echo "Build finished. Executable at build/fast-debug/bin/meow-vm"
