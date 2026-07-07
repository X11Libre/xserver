proto_to_docboox_xsl = files('xsl/protocol-to-docbook.xsl')
proto_iface_to_docboox_xsl = files('xsl/protocol-interfaces-to-docbook.xsl')
merge_mapcoords_xsl = files('xsl/merge-mapcoords.xsl')
to_publican_xsl = files('xsl/doxygen-to-publican.xsl')

publican_inputs = []
publican_image_maps = []

foreach src : files([
	'Wayland.xml', # must be first in publican_inputs
	'Wayland.ent',
	'Book_Info.xml',
	'Author_Group.xml',
	'Foreword.xml',
	'Client.xml',
	'Server.xml',
])
	name = fs.name(src)
	publican_inputs += fs.copyfile(name)
endforeach

publican_inputs += custom_target(
	'ProtocolSpec.xml',
	command: [ xsltproc, '-o', '@OUTPUT@', proto_to_docboox_xsl, '@INPUT@' ],
	input: wayland_protocol_xml,
	output: 'ProtocolSpec.xml'
)

publican_inputs += custom_target(
	'ProtocolInterfaces.xml',
	command: [ xsltproc, '-o', '@OUTPUT@', proto_iface_to_docboox_xsl, '@INPUT@' ],
	input: wayland_protocol_xml,
	output: 'ProtocolInterfaces.xml'
)

# Doxygen generates a myriad of files, and offers an XSLT
# to combine them all into one. This does the combining.
# The result is still Doxygen XML.
ClientAPI_combined = custom_target(
	'ClientAPI-combine-doxygen-files',
	command: [ xsltproc, '-o', '@OUTPUT@', '@INPUT@' ],
	input: [ doxygen_Client_combine_xslt, doxygen_Client_index_xml ],
	output: 'ClientAPI-combined.xml'
)

# This converts Doxygen XML to DocBook XML.
publican_inputs += custom_target(
	'ClientAPI-doxygen-to-docbook',
	command: [ xsltproc, '-o', '@OUTPUT@', '--stringparam', 'which', 'Client', to_publican_xsl, '@INPUT@' ],
	input: ClientAPI_combined,
	output: 'ClientAPI.xml'
)

ServerAPI_combined = custom_target(
	'ServerAPI-combine-doxygen-files',
	command: [ xsltproc, '-o', '@OUTPUT@', '@INPUT@' ],
	input: [ doxygen_Server_combine_xslt, doxygen_Server_index_xml ],
	output: 'ServerAPI-combined.xml'
)

publican_inputs += custom_target(
	'ServerAPI-doxygen-to-docbook',
	command: [ xsltproc, '-o', '@OUTPUT@', '--stringparam', 'which', 'Server', to_publican_xsl, '@INPUT@' ],
	input: ServerAPI_combined,
	output: 'ServerAPI.xml'
)

subdir('html/css')
subdir('html/images')

skip_validation = '--skip-validation'
if get_option('docbook_validation')
	skip_validation = []
endif

custom_target(
	'Wayland-docbook-html',
	command: [
		xmlto,
		skip_validation,
		'--stringparam', 'chunker.output.encoding=UTF-8',
		'--stringparam', 'chunk.section.depth=0',
		'--stringparam', 'toc.section.depth=1',
		'--stringparam', 'generate.consistent.ids=1',
		'--stringparam', 'html.stylesheet=css/default.css',
		'-o', '@OUTPUT@',
		'html',
		'@INPUT0@'
	],
	input: publican_inputs,
	output: 'html',
	build_by_default: true,
	install: true,
	install_dir: publican_install_prefix,
)
