#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Every server-internal header (*str.h, *Priv.h, *priv.h, *rst.h) that a
# driver's src/*.c file includes directly must be needed by that file:
# compiling the file without the include has to fail. Headers in src/*.h
# are not checked, because their server includes are also reached through
# other headers.
#
#   check-private-includes.py <meson-builddir> [<source-subdir>]
#
# <source-subdir> is the name of the directory holding the driver's .c
# files (default: src).
#
# Exit 0 pass, 1 an include is not needed, 77 skip (no compile database).

import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

INCLUDE = re.compile(
    r'^\s*#\s*include\s*[<"]([A-Za-z0-9_]+(?:str|Priv|priv|rst)\.h)[>"]')


def compile_args(entry, source, copy):
    args = shlex.split(entry['command'])
    out = []
    skip = False
    for i, arg in enumerate(args):
        if skip:
            skip = False
            continue
        if arg in ('-o', '-MQ', '-MF'):
            skip = True
            continue
        if arg in ('-MD', '-c'):
            continue
        if arg == entry['file']:
            continue
        out.append(arg)
    src_dir = os.path.dirname(source)
    return [out[0], '-I' + src_dir] + out[1:] + ['-fsyntax-only', copy]


def main():
    if len(sys.argv) not in (2, 3):
        print('usage: check-private-includes.py <meson-builddir> '
              '[<source-subdir>]')
        return 2
    subdir = sys.argv[2] if len(sys.argv) == 3 else 'src'
    db_path = os.path.join(sys.argv[1], 'compile_commands.json')
    if not os.path.exists(db_path):
        print('skip: no ' + db_path)
        return 77

    with open(db_path) as f:
        entries = json.load(f)

    failures = 0
    checked = 0
    for entry in entries:
        source = os.path.normpath(
            os.path.join(entry['directory'], entry['file']))
        src_dir = os.path.dirname(source)
        if not source.endswith('.c') or os.path.basename(src_dir) != subdir:
            continue
        local_headers = set(os.listdir(src_dir))
        with open(source) as f:
            lines = f.readlines()

        for n, line in enumerate(lines):
            m = INCLUDE.match(line)
            if not m or m.group(1) in local_headers:
                continue
            header = m.group(1)
            checked += 1
            with tempfile.TemporaryDirectory() as tmp:
                copy = os.path.join(tmp, os.path.basename(source))
                with open(copy, 'w') as f:
                    f.writelines(lines[:n] + lines[n + 1:])
                result = subprocess.run(
                    compile_args(entry, source, copy),
                    cwd=entry['directory'],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            name = os.path.basename(source)
            if result.returncode == 0:
                print('FAIL %s: %s is not needed (line %d)' %
                      (name, header, n + 1))
                failures += 1
            else:
                print('ok   %s: %s is needed' % (name, header))

    print('%d includes checked, %d not needed' % (checked, failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
