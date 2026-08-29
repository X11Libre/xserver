# x11-typegen: generates X11 reply-struct C headers from per-extension YAML.
#
# The generator is a small Go program (go >= 1.22). If ``go`` is not
# available, generation is skipped with a warning — the build then simply
# consumes the already-generated header if one exists.

go = find_program('go', required: false)

if not go.found()
    warning('go not found — skipping x11-typegen generation')
    subdir_done()
endif

typegen_dir = meson.current_source_dir()
gen_script = find_program(join_paths(typegen_dir, 'gen.sh'))

x11_core_reply_h = custom_target('x11-core-reply',
    output: 'x11-core-reply.h',
    input: [
        'core.yaml',
        'shape.yaml',
    ],
    command: [
        gen_script, '-o', '@OUTPUT@', '@INPUT@',
    ],
    build_by_default: true,
)