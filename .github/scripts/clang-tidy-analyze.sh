#!/bin/sh
set -eu

build_dir="build"
jobs=1
show_total=0
target_only=0
total_only=0
target=""
target_kind=""
target_file_abs=""

usage() {
    echo "Usage: $0 [-p BUILD_DIR] [-j N] [-t|--total] [--target-only|--total-only] <target>" >&2
    echo "  target: .c/.h file path or directory path to check." >&2
    echo "  -p DIR: compile database directory (default: $build_dir)." >&2
    echo "  -j N:  number of parallel clang-tidy jobs (positive integer, default: 1)." >&2
    echo "  -t:    print TOTAL warnings after per-file counts." >&2
    echo "  --target-only: for file targets, count warnings only in that file (exclude headers)." >&2
    echo "  --total-only: print only TOTAL count (no per-file breakdown)." >&2
    echo "Requires: clang-tidy, jq, and $build_dir/compile_commands.json." >&2
}

while [ "$#" -gt 0 ]; do
    case "$1" in
    -h | --help)
        usage
        exit 0
        ;;
    -t | --total)
        show_total=1
        shift
        ;;
    -p)
        if [ "$#" -lt 2 ]; then
            echo "Missing value for -p." >&2
            usage
            exit 2
        fi
        build_dir="$2"
        shift 2
        ;;
    --target-only)
        target_only=1
        shift
        ;;
    --total-only)
        total_only=1
        show_total=1
        shift
        ;;
    -j)
        if [ "$#" -lt 2 ]; then
            echo "Missing value for -j." >&2
            usage
            exit 2
        fi
        jobs="$2"
        case "$jobs" in
        '' | *[!0-9]* | 0)
            echo "Invalid -j value: $jobs (expected positive integer)." >&2
            usage
            exit 2
            ;;
        esac
        shift 2
        ;;
    --)
        shift
        break
        ;;
    -*)
        echo "Unknown option: $1" >&2
        usage
        exit 2
        ;;
    *) break ;;
    esac
done

if [ "$#" -eq 1 ]; then
    target="$1"
else
    if [ "$#" -eq 0 ]; then
        echo "Missing target." >&2
    else
        echo "Expected exactly one target argument." >&2
    fi
    usage
    exit 2
fi

if [ "$target_only" -eq 1 ] && [ "$total_only" -eq 1 ]; then
    echo "--target-only and --total-only are incompatible." >&2
    usage
    exit 2
fi

if [ ! -d "$build_dir" ]; then
    echo "Build directory '$build_dir' not found. Run meson setup first." >&2
    exit 1
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy not found in PATH." >&2
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "jq not found in PATH." >&2
    exit 1
fi

build_abs="$(cd "$build_dir" && pwd -P)"
tmp_outdir="$(mktemp -d)"
tmp_files="$tmp_outdir/files.txt"
tmp_parsed="$tmp_outdir/parsed.txt"
trap 'rm -rf "$tmp_outdir"' EXIT INT TERM HUP

# Read compile_commands.json to get every matching path
if [ -f "$target" ]; then
    target_kind="file"
    target_file_abs="$(realpath "$target")"
    case "$target" in
    *.c | *.h) printf '%s\n' "$target" >"$tmp_files" ;;
    *)
        echo "Not a .c or .h file: $target" >&2
        exit 2
        ;;
    esac
elif [ -d "$target" ]; then
    target_kind="dir"
    target_match="$target"
    compdb="$build_dir/compile_commands.json"
    if [ ! -f "$compdb" ]; then
        echo "Compilation database '$compdb' not found. Run meson setup first." >&2
        exit 1
    fi
    jq -r '.[] | [.directory, .file] | @tsv' "$compdb" |
        while IFS="$(printf '\t')" read -r dir f; do
            case "$f" in
            /*) canon="$(realpath "$f")" ;;
            *) canon="$(realpath "$dir/$f")" ;;
            esac
            printf '%s\n' "$canon" | grep -F -q "$target_match" && printf '%s\n' "$canon"
        done |
        awk '!seen[$0]++' >"$tmp_files"
else
    echo "Path not found: $target" >&2
    exit 2
fi

if [ "$target_only" -eq 1 ] && [ "$target_kind" != "file" ]; then
    echo "--target-only requires a file target." >&2
    exit 2
fi

# No matching source files found
if [ ! -s "$tmp_files" ]; then
    [ "$show_total" -eq 1 ] && printf "0\tTOTAL\n"
    exit 0
fi

emit_clang_output() {
    if command -v xargs >/dev/null 2>&1; then
        jobs_nul="$tmp_outdir/jobs.nul"
        : >"$jobs_nul"
        while IFS= read -r f; do
            sum="$(printf '%s' "$f" | cksum | cut -d ' ' -f 1)"
            out="$tmp_outdir/$sum.out"
            printf '%s\0%s\0' "$f" "$out" >>"$jobs_nul"
        done <"$tmp_files"
        # shellcheck disable=SC2016
        xargs -0 -n 2 -P "$jobs" sh -c '
            clang-tidy -p "$1" "$2" >"$3" 2>&1 || true
        ' sh "$build_abs" <"$jobs_nul"
        for out in "$tmp_outdir"/*.out; do
            [ -f "$out" ] && cat "$out"
        done
    else
        while IFS= read -r f; do
            clang-tidy -p "$build_abs" "$f" 2>&1 || true
        done <"$tmp_files"
    fi
}

# Print warnings per absolute file path
emit_clang_output |
    awk -F: \
        -v build="$build_abs" \
        -v with_total="$show_total" \
        -v target_only="$target_only" \
        -v target_file="$target_file_abs" '
        function canon(p,   abs, n, i, part, top, stack, out) {
            if (p ~ /^\//) {
                abs = p
            } else {
                abs = build "/" p
            }
            n = split(abs, stack, "/")
            top = 0
            for (i = 1; i <= n; i++) {
                part = stack[i]
                if (part == "" || part == ".") {
                    continue
                }
                if (part == "..") {
                    if (top > 0) {
                        top--
                    }
                    continue
                }
                top++
                stack[top] = part
            }
            out = ""
            for (i = 1; i <= top; i++) {
                out = out "/" stack[i]
            }
            return (out == "" ? "/" : out)
        }
        {
            # Strict diagnostic shape: path:line:col: warning: <message> [tidy-check]
            if ($0 !~ /^([^:[:space:]]+):([1-9][0-9]*):([1-9][0-9]*): warning: .+ \[[[:alnum:].-]+\]$/ \
                || (target_only == 1 && canon($1) != target_file)) {
                next
            }
            file = canon($1)
            total++
            count[file]++
        }
        END {
            for (f in count) {
                printf "%d\t%s\n", count[f], f
            }
            if (with_total == 1) {
                printf "__TOTAL__\t%d\n", total + 0
            }
        }
    ' >"$tmp_parsed"

# Display sorted output with optional total
if [ "$total_only" -eq 0 ]; then
    grep -v '^__TOTAL__' "$tmp_parsed" | sort -nr
fi
if [ "$show_total" -eq 1 ]; then
    total_count="$(grep '^__TOTAL__' "$tmp_parsed" | cut -f2)"
    [ -n "$total_count" ] && printf '%s\tTOTAL\n' "$total_count"
fi
