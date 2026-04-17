#!/bin/bash
#
# Helper script used by meson to ensure each test_foo.py file has
# a corresponding entry in the tests_pyxtest array

SOURCEDIR="${1:-@SOURCEDIR@}"
shift

TESTS="${*:-@TESTS@}"
FILES=$(find "$SOURCEDIR" -name "test_*.py" -printf "%f\n")

DIFF=$(diff -u <(echo "$TESTS" | tr " " "\n" | sort) <(echo "$FILES" | sort))
if [[ $? -ne 0 ]]; then
    echo "ERROR: Missing test file in meson tests list:" >&2
    echo "$DIFF" >&2
    exit 1
fi
