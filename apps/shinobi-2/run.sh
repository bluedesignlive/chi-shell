#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════
# Shinobi Terminal — Build & Run
# ═══════════════════════════════════════════════════════════════
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BINARY="${BUILD_DIR}/shinobi"
CHI_QML_PATH="${PROJECT_DIR}/../../../chi/qml"
BUILD_TYPE="${1:-Debug}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
DIM='\033[0;90m'
RESET='\033[0m'

step() { echo -e "${CYAN}▸${RESET} $1"; }
ok()   { echo -e "${GREEN}✓${RESET} $1"; }
warn() { echo -e "${YELLOW}⚠${RESET} $1"; }
fail() { echo -e "${RED}✗${RESET} $1"; exit 1; }

# ─── Detect if reconfiguration is needed ─────────────────────
needs_configure() {
    # No build directory at all
    [[ ! -d "${BUILD_DIR}" ]] && return 0

    # No CMakeCache
    [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]] && return 0

    # CMakeLists.txt is newer than the cache
    [[ "${PROJECT_DIR}/CMakeLists.txt" -nt "${BUILD_DIR}/CMakeCache.txt" ]] && return 0

    # Chi path changed
    local cached_chi
    cached_chi=$(grep -oP 'CHI_QML_PATH:PATH=\K.*' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null || echo "")
    [[ "${cached_chi}" != "${CHI_QML_PATH}" ]] && return 0

    # Build type changed
    local cached_type
    cached_type=$(grep -oP 'CMAKE_BUILD_TYPE:STRING=\K.*' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null || echo "")
    [[ "${cached_type}" != "${BUILD_TYPE}" ]] && return 0

    return 1
}

# ─── Detect if any source is newer than binary ───────────────
needs_build() {
    [[ ! -f "${BINARY}" ]] && return 0

    # Check if any source file is newer than the binary
    local newest_source
    newest_source=$(find "${PROJECT_DIR}/src" "${PROJECT_DIR}/qml" \
                         "${PROJECT_DIR}/CMakeLists.txt" \
                         -type f \( -name '*.cpp' -o -name '*.h' \
                                    -o -name '*.qml' -o -name 'CMakeLists.txt' \) \
                         -newer "${BINARY}" 2>/dev/null | head -1)
    [[ -n "${newest_source}" ]] && return 0

    return 1
}

echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════${RESET}"
echo -e "${CYAN}  忍  Shinobi Terminal${RESET}"
echo -e "${DIM}  Build type: ${BUILD_TYPE}${RESET}"
echo -e "${CYAN}═══════════════════════════════════════════════════════${RESET}"
echo ""

# ─── Step 1: Configure (only if needed) ──────────────────────
if needs_configure; then
    step "Configuring CMake..."

    # Clean stale build if CMakeLists changed structurally
    if [[ -d "${BUILD_DIR}" && "${PROJECT_DIR}/CMakeLists.txt" -nt "${BUILD_DIR}/CMakeCache.txt" ]]; then
        warn "CMakeLists.txt changed — cleaning build cache"
        rm -rf "${BUILD_DIR}"
    fi

    mkdir -p "${BUILD_DIR}"

    cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" \
          -G Ninja \
          -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
          -DCHI_QML_PATH="${CHI_QML_PATH}" \
          -Wno-dev \
          2>&1 | sed 's/^/  /'

    ok "Configuration complete"
    echo ""
else
    ok "Configuration up to date"
fi

# ─── Step 2: Build (only if needed) ──────────────────────────
if needs_build; then
    step "Building..."
    echo ""

    BUILD_START=$(date +%s%N)

    if ! cmake --build "${BUILD_DIR}" --parallel 2>&1 | sed 's/^/  /'; then
        echo ""
        fail "Build failed"
    fi

    BUILD_END=$(date +%s%N)
    BUILD_MS=$(( (BUILD_END - BUILD_START) / 1000000 ))

    echo ""
    ok "Build complete (${BUILD_MS}ms)"
else
    ok "Binary up to date — skipping build"
fi

# ─── Step 3: Run ─────────────────────────────────────────────
echo ""
step "Launching Shinobi..."
echo -e "${DIM}────────────────────────────────────────────────────${RESET}"
echo ""

# Set QML import paths
export QML_IMPORT_PATH="${CHI_QML_PATH}:${BUILD_DIR}"
export QML2_IMPORT_PATH="${QML_IMPORT_PATH}"

# Wayland by default, fallback to X11
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland;xcb}"

# Run
exec "${BINARY}" "$@"
