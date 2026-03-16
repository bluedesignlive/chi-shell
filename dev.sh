#!/bin/bash
set -e
cd "$(dirname "$0")"

RED='\033[0;31m'
GRN='\033[0;32m'
CYN='\033[0;36m'
RST='\033[0m'

case "${1:-run}" in
  clean)
    echo -e "${CYN}♻  Clean build...${RST}"
    rm -rf build
    cmake -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -1
    cmake --build build -j"$(nproc)"
    echo -e "${GRN}✓  Clean build done${RST}"
    ;;
  build|b)
    cmake -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -1
    cmake --build build -j"$(nproc)"
    echo -e "${GRN}✓  Build done${RST}"
    ;;
  run|r|"")
    cmake -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -1
    cmake --build build -j"$(nproc)"
    echo -e "${GRN}✓  Build done — launching...${RST}"
    ./build/chi-shell
    ;;
  kill|k)
    pkill -f chi-shell && echo -e "${RED}✗  Killed${RST}" || echo "Not running"
    ;;
  restart|re)
    pkill -f chi-shell 2>/dev/null || true
    sleep 0.3
    "$0" run
    ;;
  *)
    echo "Usage: ./dev.sh [run|build|clean|kill|restart]"
    echo "  run (default)  — build + launch"
    echo "  build / b      — build only"
    echo "  clean          — nuke build dir + rebuild"
    echo "  kill / k       — kill running chi-shell"
    echo "  restart / re   — kill + rebuild + launch"
    ;;
esac
