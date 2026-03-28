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

needs_configure() {
    [[ ! -d "${BUILD_DIR}" ]] && return 0
    [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]] && return 0
    [[ "${PROJECT_DIR}/CMakeLists.txt" -nt "${BUILD_DIR}/CMakeCache.txt" ]] && return 0
    local cached_chi
    cached_chi=$(grep -oP 'CHI_QML_PATH:PATH=\K.*' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null || echo "")
    [[ "${cached_chi}" != "${CHI_QML_PATH}" ]] && return 0
    local cached_type
    cached_type=$(grep -oP 'CMAKE_BUILD_TYPE:STRING=\K.*' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null || echo "")
    [[ "${cached_type}" != "${BUILD_TYPE}" ]] && return 0
    return 1
}

needs_build() {
    [[ ! -f "${BINARY}" ]] && return 0
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
echo -e "${DIM}  Build: ${BUILD_TYPE} | Logs: shinobi.*${RESET}"
echo -e "${CYAN}═══════════════════════════════════════════════════════${RESET}"
echo ""

if needs_configure; then
    step "Configuring CMake..."
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

echo ""
step "Launching Shinobi..."
echo -e "${DIM}────────────────────────────────────────────────────${RESET}"
echo ""

export QML_IMPORT_PATH="${CHI_QML_PATH}:${BUILD_DIR}"
export QML2_IMPORT_PATH="${QML_IMPORT_PATH}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland;xcb}"

# ── Logging ──────────────────────────────────────────────────
# Show all shinobi.* logs by default.
# Suppress noisy PTY read logs in normal mode.
# For verbose: ./run.sh Debug --verbose
if [[ "${2:-}" == "--verbose" ]]; then
    export QT_LOGGING_RULES="shinobi.*.debug=true"
else
    # Info + warnings for all, debug only for app/session/render
    export QT_LOGGING_RULES="shinobi.*.debug=false;shinobi.app.debug=true;shinobi.session.debug=true;shinobi.render.debug=true"
fi

# Suppress noisy Qt Wayland text-input warnings
export QT_LOGGING_RULES="${QT_LOGGING_RULES};qt.qpa.wayland.textinput=false"

exec "${BINARY}"
