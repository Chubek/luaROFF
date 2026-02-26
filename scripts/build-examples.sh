#!/usr/bin/env bash
# =========================================================================
#  build-examples.sh — Build all pplua example .roff files into a
#                      selected output target.
#
#  Usage:
#      ./build-examples.sh [OPTIONS] [file ...]
#
#  Options:
#      -t TARGET    Output target: pdf (default), ps, utf8, ascii, html
#      -m MACRO     Macro package: -ms (default), -mm, -me, -mom, -man
#      -o OUTDIR    Output directory (default: ./output)
#      -I LUAPATH   Additional Lua package.path for pplua
#      -l PREAMBLE  Lua preamble file passed to pplua -l
#      -D VAR=VAL   Define variable passed to pplua -D (repeatable)
#      -P PPLUA     Path to pplua binary (default: pplua)
#      -j JOBS      Parallel jobs (default: 1; 0 = nproc)
#      -n           Suppress .lf line directives (passed to pplua -n)
#      -v           Verbose output
#      -h           Print help and exit
#
#  If no files are given, all *.roff files in the current directory
#  (and subdirectories, non-recursively) are processed.
#
#  Examples:
#      ./build-examples.sh -t pdf
#      ./build-examples.sh -t utf8 -m -ms release-notes.roff
#      ./build-examples.sh -t html -o /tmp/docs *.roff
#      ./build-examples.sh -t pdf -D SEED=42 -D DRAFT=1 exam.roff
# =========================================================================

set -euo pipefail

# ── Defaults ──────────────────────────────────────────────────────────────

TARGET="pdf"
MACRO="-ms"
OUTDIR="./output"
PPLUA="pplua"
VERBOSE=0
SUPPRESS_LF=0
JOBS=1

# Repeatable options accumulated into arrays
declare -a PPLUA_DEFINES=()
declare -a PPLUA_EXTRAS=()
LUA_PREAMBLE=""
LUA_INCLUDE=""

# ── Colors (if tty) ──────────────────────────────────────────────────────

if [ -t 1 ]; then
    C_RESET='\033[0m'
    C_BOLD='\033[1m'
    C_GREEN='\033[32m'
    C_YELLOW='\033[33m'
    C_RED='\033[31m'
    C_CYAN='\033[36m'
else
    C_RESET='' C_BOLD='' C_GREEN='' C_YELLOW='' C_RED='' C_CYAN=''
fi

# ── Helpers ──────────────────────────────────────────────────────────────

usage() {
    cat <<'EOF'
Usage: build-examples.sh [OPTIONS] [file ...]

Options:
    -t TARGET    Output target: pdf (default), ps, utf8, ascii, html
    -m MACRO     Macro package: -ms (default), -mm, -me, -mom, -man
    -o OUTDIR    Output directory (default: ./output)
    -I LUAPATH   Additional Lua package.path for pplua
    -l PREAMBLE  Lua preamble file passed to pplua -l
    -D VAR=VAL   Define variable for pplua -D (repeatable)
    -P PPLUA     Path to pplua binary (default: pplua)
    -j JOBS      Parallel jobs (default: 1; 0 = nproc)
    -n           Suppress .lf line directives
    -v           Verbose output
    -h           Print help and exit

Targets:
    pdf          PDF via groff -Tpdf
    ps           PostScript via groff -Tps
    utf8         UTF-8 plain text via groff -Tutf8
    ascii        ASCII plain text via groff -Tascii
    html         HTML via groff -Thtml

If no files are given, all *.roff files in the current directory are built.
EOF
}

log_info()  { printf "${C_CYAN}[INFO]${C_RESET}  %s\n" "$*"; }
log_ok()    { printf "${C_GREEN}[  OK]${C_RESET}  %s\n" "$*"; }
log_warn()  { printf "${C_YELLOW}[WARN]${C_RESET}  %s\n" "$*" >&2; }
log_err()   { printf "${C_RED}[ ERR]${C_RESET}  %s\n" "$*" >&2; }
log_verb()  { [[ "$VERBOSE" -eq 1 ]] && printf "${C_BOLD}[VERB]${C_RESET}  %s\n" "$*"; }

die() { log_err "$@"; exit 1; }

# ── Parse Options ────────────────────────────────────────────────────────

while getopts ":t:m:o:I:l:D:P:j:nvh" opt; do
    case "$opt" in
        t) TARGET="$OPTARG" ;;
        m) MACRO="$OPTARG" ;;
        o) OUTDIR="$OPTARG" ;;
        I) LUA_INCLUDE="$OPTARG" ;;
        l) LUA_PREAMBLE="$OPTARG" ;;
        D) PPLUA_DEFINES+=("-D" "$OPTARG") ;;
        P) PPLUA="$OPTARG" ;;
        j) JOBS="$OPTARG" ;;
        n) SUPPRESS_LF=1 ;;
        v) VERBOSE=1 ;;
        h) usage; exit 0 ;;
        :) die "Option -$OPTARG requires an argument." ;;
        *) die "Unknown option: -$OPTARG. Use -h for help." ;;
    esac
done
shift $((OPTIND - 1))

# ── Validate target ─────────────────────────────────────────────────────

case "$TARGET" in
    pdf)   GROFF_DEV="-Tpdf";  EXT="pdf"  ;;
    ps)    GROFF_DEV="-Tps";   EXT="ps"   ;;
    utf8)  GROFF_DEV="-Tutf8"; EXT="txt"  ;;
    ascii) GROFF_DEV="-Tascii";EXT="txt"  ;;
    html)  GROFF_DEV="-Thtml"; EXT="html" ;;
    *)     die "Unknown target '$TARGET'. Choose: pdf, ps, utf8, ascii, html." ;;
esac

# ── Resolve parallel jobs ───────────────────────────────────────────────

if [[ "$JOBS" -eq 0 ]]; then
    JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

# ── Locate tools ────────────────────────────────────────────────────────

command -v "$PPLUA" >/dev/null 2>&1 || die "pplua not found at '$PPLUA'. Use -P to specify path."
command -v groff    >/dev/null 2>&1 || die "groff not found in PATH."

# Check for optional preprocessors
HAS_TBL=0;  command -v tbl  >/dev/null 2>&1 && HAS_TBL=1
HAS_EQN=0;  command -v eqn  >/dev/null 2>&1 && HAS_EQN=1
HAS_PIC=0;  command -v pic  >/dev/null 2>&1 && HAS_PIC=1
HAS_SOELIM=0; command -v soelim >/dev/null 2>&1 && HAS_SOELIM=1

log_verb "Available preprocessors: tbl=$HAS_TBL eqn=$HAS_EQN pic=$HAS_PIC soelim=$HAS_SOELIM"

# ── Collect input files ─────────────────────────────────────────────────

declare -a FILES=()

if [[ $# -gt 0 ]]; then
    for f in "$@"; do
        [[ -f "$f" ]] || { log_warn "File not found, skipping: $f"; continue; }
        FILES+=("$f")
    done
else
    # Glob *.roff in current directory (non-recursive)
    shopt -s nullglob
    for f in *.roff; do
        FILES+=("$f")
    done
    shopt -u nullglob
fi

[[ ${#FILES[@]} -gt 0 ]] || die "No .roff files found. Provide files as arguments or run from a directory containing *.roff."

# ── Create output directory ─────────────────────────────────────────────

mkdir -p "$OUTDIR"
log_info "Target: $TARGET ($GROFF_DEV)  |  Macro: $MACRO  |  Output: $OUTDIR/"
log_info "Files to process: ${#FILES[@]}  |  Parallel jobs: $JOBS"

# ── Detect which preprocessors a file needs ──────────────────────────────
#    We scan for tbl (.TS/.TE), eqn (.EQ/.EN), pic (.PS/.PE) markers.

needs_preprocessor() {
    local file="$1" preproc="$2"
    case "$preproc" in
        tbl)    grep -qE '^\.(TS|TE)' "$file" 2>/dev/null ;;
        eqn)    grep -qE '^\.(EQ|EN)' "$file" 2>/dev/null ;;
        pic)    grep -qE '^\.(PS|PE)' "$file" 2>/dev/null ;;
        soelim) grep -qE '^\.so '     "$file" 2>/dev/null ;;
    esac
}

# ── Build a single file ─────────────────────────────────────────────────

build_one() {
    local infile="$1"
    local basename
    basename="$(basename "$infile" .roff)"
    local outfile="${OUTDIR}/${basename}.${EXT}"

    # Assemble pplua command
    local -a pplua_cmd=("$PPLUA")
    [[ "$SUPPRESS_LF" -eq 1 ]] && pplua_cmd+=("-n")
    [[ -n "$LUA_INCLUDE" ]]    && pplua_cmd+=("-I" "$LUA_INCLUDE")
    [[ -n "$LUA_PREAMBLE" ]]   && pplua_cmd+=("-l" "$LUA_PREAMBLE")
    [[ ${#PPLUA_DEFINES[@]} -gt 0 ]] && pplua_cmd+=("${PPLUA_DEFINES[@]}")
    pplua_cmd+=("$infile")

    # Assemble the pipeline: pplua | [soelim] | [tbl] | [eqn] | [pic] | groff
    local pipeline
    pipeline="$(printf '%q ' "${pplua_cmd[@]}")"

    if [[ "$HAS_SOELIM" -eq 1 ]] && needs_preprocessor "$infile" soelim; then
        pipeline+=" | soelim"
        log_verb "$basename: adding soelim"
    fi
    if [[ "$HAS_TBL" -eq 1 ]] && needs_preprocessor "$infile" tbl; then
        pipeline+=" | tbl"
        log_verb "$basename: adding tbl"
    fi
    if [[ "$HAS_EQN" -eq 1 ]] && needs_preprocessor "$infile" eqn; then
        pipeline+=" | eqn"
        log_verb "$basename: adding eqn"
    fi
    if [[ "$HAS_PIC" -eq 1 ]] && needs_preprocessor "$infile" pic; then
        pipeline+=" | pic"
        log_verb "$basename: adding pic"
    fi

    pipeline+=" | groff ${MACRO} ${GROFF_DEV}"

    # For binary targets, redirect to file; for text targets, also redirect
    pipeline+=" > $(printf '%q' "$outfile")"

    log_verb "Pipeline: $pipeline"

    # Execute
    local errlog
    errlog="${OUTDIR}/.${basename}.err"

    if eval "$pipeline" 2>"$errlog"; then
        local size
        size=$(wc -c < "$outfile" 2>/dev/null | tr -d ' ')
        log_ok "${basename}.${EXT}  (${size} bytes)"
        # Clean up error log if empty
        [[ -s "$errlog" ]] || rm -f "$errlog"
        return 0
    else
        log_err "Failed: $infile"
        if [[ -s "$errlog" ]]; then
            # Show first 10 lines of errors
            head -n 10 "$errlog" | while IFS= read -r line; do
                log_err "  $line"
            done
        fi
        return 1
    fi
}

# ── Main loop (with optional parallelism) ────────────────────────────────

TOTAL=${#FILES[@]}
SUCCESS=0
FAILURE=0
ELAPSED_START=$SECONDS

if [[ "$JOBS" -eq 1 ]]; then
    # Sequential
    for f in "${FILES[@]}"; do
        if build_one "$f"; then
            ((SUCCESS++))
        else
            ((FAILURE++))
        fi
    done
else
    # Parallel using background jobs and a simple semaphore
    declare -a PIDS=()
    declare -A PID_FILE=()

    sem_wait() {
        while [[ ${#PIDS[@]} -ge $JOBS ]]; do
            local -a new_pids=()
            for pid in "${PIDS[@]}"; do
                if kill -0 "$pid" 2>/dev/null; then
                    new_pids+=("$pid")
                else
                    wait "$pid" 2>/dev/null && ((SUCCESS++)) || ((FAILURE++))
                fi
            done
            PIDS=("${new_pids[@]}")
            [[ ${#PIDS[@]} -ge $JOBS ]] && sleep 0.1
        done
    }

    for f in "${FILES[@]}"; do
        sem_wait
        build_one "$f" &
        local pid=$!
        PIDS+=("$pid")
        PID_FILE[$pid]="$f"
    done

    # Wait for remaining
    for pid in "${PIDS[@]}"; do
        wait "$pid" 2>/dev/null && ((SUCCESS++)) || ((FAILURE++))
    done
fi

ELAPSED=$(( SECONDS - ELAPSED_START ))

# ── Summary ──────────────────────────────────────────────────────────────

echo ""
log_info "──────────────────────────────────────────"
log_info "Build complete in ${ELAPSED}s"
log_info "  Total:   $TOTAL"
log_ok   "  Success: $SUCCESS"
[[ "$FAILURE" -gt 0 ]] && log_err "  Failed:  $FAILURE"
log_info "  Output:  $OUTDIR/"
log_info "──────────────────────────────────────────"

[[ "$FAILURE" -eq 0 ]] && exit 0 || exit 1
