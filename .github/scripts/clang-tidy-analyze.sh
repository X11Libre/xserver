#!/bin/sh
set -eu

default_build_dir="build"
build_dir=""
jobs=1
show_total=0
target_only=0
total_only=0
warnings_only=0
errors_only=0
target=""
target_kind="all"
target_file_abs=""

usage() {
	echo "Usage: $0 [-p BUILD_DIR] [-j N] [--warnings-only|--errors-only] [-t|--total] [--target-only] [--total-only] [target]" >&2
	echo "  target: optional .c/.h file path or directory path to check; omit for whole project." >&2
	echo "  -p DIR: compile database directory. Defaults to ./compile_commands.json, then $default_build_dir/compile_commands.json." >&2
	echo "  -j N:  number of parallel clang-tidy jobs (positive integer, default: 1)." >&2
	echo "  -t, --total: append TOTAL counts after per-file counts." >&2
	echo "  --warnings-only: print warning counts only." >&2
	echo "  --errors-only: print error counts only." >&2
	echo "  --target-only: for file targets, count diagnostics only in that file (exclude headers)." >&2
	echo "  --total-only: print only TOTAL counts (no per-file breakdown)." >&2
	echo "" >&2
	echo "Examples:" >&2
	echo "  $0 -j 8 --total" >&2
	echo "  $0 -j 8 hw/xfree86" >&2
	echo "  $0 --target-only hw/xfree86/drivers/video/modesetting/drmmode_display.c" >&2
	echo "" >&2
	echo "Requires: clang-tidy, jq, and a Meson compile_commands.json database." >&2
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
	--warnings-only)
		warnings_only=1
		shift
		;;
	--errors-only)
		errors_only=1
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

if [ "$#" -gt 1 ]; then
	echo "Expected at most one target argument." >&2
	usage
	exit 2
elif [ "$#" -eq 1 ]; then
	target="$1"
fi

if [ "$warnings_only" -eq 1 ] && [ "$errors_only" -eq 1 ]; then
	echo "--warnings-only and --errors-only are incompatible." >&2
	usage
	exit 2
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
	echo "clang-tidy not found in PATH." >&2
	exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
	echo "jq not found in PATH." >&2
	exit 1
fi

if [ -n "$build_dir" ]; then
	compdb_dir="$build_dir"
elif [ -f compile_commands.json ]; then
	compdb_dir="."
else
	compdb_dir="$default_build_dir"
fi

if [ ! -d "$compdb_dir" ]; then
	echo "Compile database directory '$compdb_dir' not found. Run meson setup first." >&2
	exit 1
fi

compdb="$compdb_dir/compile_commands.json"
if [ ! -f "$compdb" ]; then
	echo "Compilation database '$compdb' not found. Run meson setup first." >&2
	exit 1
fi

compdb_dir_abs="$(cd "$compdb_dir" && pwd -P)"
tmp_outdir="$(mktemp -d)"
tmp_files="$tmp_outdir/files.txt"
tmp_files_nul="$tmp_outdir/files.nul"
tmp_parsed="$tmp_outdir/parsed.txt"
trap 'rm -rf "$tmp_outdir"' EXIT INT TERM HUP

first_dir="$(jq -r '.[0].directory // empty' "$compdb")"
case "$first_dir" in
/*) diag_base="$(cd "$first_dir" && pwd -P)" ;;
"") diag_base="$compdb_dir_abs" ;;
*) diag_base="$(cd "$compdb_dir/$first_dir" && pwd -P)" ;;
esac

canonical_path() {
	if command -v realpath >/dev/null 2>&1; then
		realpath "$1"
	else
		dir=$(dirname "$1")
		base=$(basename "$1")
		printf '%s/%s\n' "$(cd "$dir" && pwd -P)" "$base"
	fi
}

emit_compdb_files() {
	jq -r '.[] | [.directory, .file] | @tsv' "$compdb" |
		while IFS="$(printf '\t')" read -r dir file; do
			case "$file" in
			/*) canonical_path "$file" ;;
			*) canonical_path "$dir/$file" ;;
			esac
		done |
		awk '!seen[$0]++'
}

if [ -n "$target" ] && [ -f "$target" ]; then
	target_kind="file"
	target_file_abs="$(canonical_path "$target")"
	case "$target" in
	*.c | *.h) printf '%s\n' "$target_file_abs" >"$tmp_files" ;;
	*)
		echo "Not a .c or .h file: $target" >&2
		exit 2
		;;
	esac
elif [ -n "$target" ] && [ -d "$target" ]; then
	target_kind="dir"
	target_dir_abs="$(canonical_path "$target")"
	emit_compdb_files |
		while IFS= read -r file; do
			case "$file" in
			"$target_dir_abs" | "$target_dir_abs"/*) printf '%s\n' "$file" ;;
			esac
		done >"$tmp_files"
elif [ -n "$target" ]; then
	echo "Path not found: $target" >&2
	exit 2
else
	emit_compdb_files >"$tmp_files"
fi

if [ "$target_only" -eq 1 ] && [ "$target_kind" != "file" ]; then
	echo "--target-only requires a file target." >&2
	exit 2
fi

if [ ! -s "$tmp_files" ]; then
	if [ "$show_total" -eq 1 ]; then
		if [ "$warnings_only" -eq 1 ] || [ "$errors_only" -eq 1 ]; then
			printf "0\tTOTAL\n"
		else
			printf "0\t0\tTOTAL\n"
		fi
	fi
	exit 0
fi

while IFS= read -r file; do
	printf '%s\0' "$file"
done <"$tmp_files" >"$tmp_files_nul"

emit_clang_output() {
	if command -v xargs >/dev/null 2>&1; then
		xargs -0 -n 1 -P "$jobs" clang-tidy -p "$compdb_dir_abs" <"$tmp_files_nul" 2>&1 || true
	else
		while IFS= read -r file; do
			clang-tidy -p "$compdb_dir_abs" "$file" 2>&1 || true
		done <"$tmp_files"
	fi
}

emit_clang_output |
	awk -F: \
		-v base="$diag_base" \
		-v target_only="$target_only" \
		-v target_file="$target_file_abs" '
        function canon(p,   abs, n, i, part, top, stack, out) {
            if (p ~ /^\//) {
                abs = p
            } else {
                abs = base "/" p
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
            if ($0 !~ /^([^:[:space:]]+):([1-9][0-9]*):([1-9][0-9]*): (warning|error): .+ \[[[:alnum:].,-]+\]$/) {
                next
            }

            file = canon($1)
            if (target_only == 1 && file != target_file) {
                next
            }

            if ($0 ~ /: warning: /) {
                warnings[file]++
                total_warnings++
            } else {
                errors[file]++
                total_errors++
            }
            seen[file] = 1
        }
        END {
            for (file in seen) {
                printf "%d\t%d\t%s\n", warnings[file] + 0, errors[file] + 0, file
            }
            printf "__TOTAL__\t%d\t%d\n", total_warnings + 0, total_errors + 0
        }
    ' >"$tmp_parsed"

print_counts() {
	while IFS="$(printf '\t')" read -r warnings errors file; do
		if [ "$warnings_only" -eq 1 ]; then
			printf '%s\t%s\n' "$warnings" "$file"
		elif [ "$errors_only" -eq 1 ]; then
			printf '%s\t%s\n' "$errors" "$file"
		else
			printf '%s\t%s\t%s\n' "$warnings" "$errors" "$file"
		fi
	done
}

if [ "$total_only" -eq 0 ]; then
	grep -v '^__TOTAL__' "$tmp_parsed" | sort -nr | print_counts
fi

if [ "$show_total" -eq 1 ]; then
	total_line="$(grep '^__TOTAL__' "$tmp_parsed")"
	total_warnings="$(printf '%s\n' "$total_line" | cut -f2)"
	total_errors="$(printf '%s\n' "$total_line" | cut -f3)"
	if [ "$warnings_only" -eq 1 ]; then
		printf '%s\tTOTAL\n' "$total_warnings"
	elif [ "$errors_only" -eq 1 ]; then
		printf '%s\tTOTAL\n' "$total_errors"
	else
		printf '%s\t%s\tTOTAL\n' "$total_warnings" "$total_errors"
	fi
fi
