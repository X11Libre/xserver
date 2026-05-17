#!/bin/sh
set -eu

build_dir="${1:-build}"
root_link="compile_commands.json"

case "$(uname -s 2>/dev/null || echo unknown)" in
MINGW* | MSYS* | CYGWIN*)
	echo "Skipping compile_commands.json symlink on non-Unix platform." >&2
	exit 0
	;;
esac

if [ ! -f "$build_dir/compile_commands.json" ]; then
	echo "Compilation database '$build_dir/compile_commands.json' not found. Run meson setup first." >&2
	exit 1
fi

if [ -e "$root_link" ] && [ ! -L "$root_link" ]; then
	echo "Refusing to overwrite non-symlink '$root_link'." >&2
	exit 1
fi

tmp_link="$root_link.tmp.$$"
rm -f "$tmp_link"
ln -s "$build_dir/compile_commands.json" "$tmp_link"
mv -f "$tmp_link" "$root_link"
