package main

import (
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"gopkg.in/yaml.v3"
)

// compoundMember is one field of an embedded struct type (e.g. xCharInfo).
type compoundMember struct {
	name  string
	ctype string
	swap  string // swap macro for this member, "" if none
}

// typeInfo describes an X11 protocol scalar type used in generated structs.
type typeInfo struct {
	ctype string // emitted C type name (X11_*) as used in struct members
	base  string // underlying C type for the typedef (e.g. CARD32)
	arr   string // array suffix for the typedef (e.g. "[32]"), "" if scalar
	size  int    // size in bytes
	align int    // wire/C alignment requirement
	swap  string // X_REPLY_FIELD_* macro name for byteswap, "" if none
	// compound non-nil marks an embedded struct type; its members' swap
	// entries are expanded by SWAP macros using field.member paths
	compound []compoundMember
}

// Supported struct kinds; each has its own header layout, tail-padding and
// C naming scheme (see structName / layoutFields).
const (
	kindReply   = "reply"
	kindRequest = "request"
	kindEvent   = "event"
)

// typeTable maps YAML type names onto their X11_* C representation.
// Window/Atom/VisualID/Colormap/... are all CARD32 XIDs on the wire.
var typeTable = map[string]typeInfo{
	"BYTE":          {ctype: "X11_BYTE", base: "CARD8", size: 1, align: 1},
	"INT8":          {ctype: "X11_INT8", base: "INT8", size: 1, align: 1},
	"CARD8":         {ctype: "X11_CARD8", base: "CARD8", size: 1, align: 1},
	"KEYCODE":       {ctype: "X11_KEYCODE", base: "CARD8", size: 1, align: 1},
	"BOOL":          {ctype: "X11_BOOL", base: "CARD8", size: 1, align: 1},
	"CARD16":        {ctype: "X11_CARD16", base: "CARD16", size: 2, align: 2, swap: "X_REPLY_FIELD_CARD16"},
	"INT16":         {ctype: "X11_INT16", base: "INT16", size: 2, align: 2, swap: "X_REPLY_FIELD_CARD16"},
	"KEYBUTMASK":    {ctype: "X11_KEYBUTMASK", base: "CARD16", size: 2, align: 2, swap: "X_REPLY_FIELD_CARD16"},
	"CARD32":        {ctype: "X11_CARD32", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"INT32":         {ctype: "X11_INT32", base: "INT32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"CARD64":        {ctype: "X11_CARD64", base: "CARD64", size: 8, align: 8, swap: "X_REPLY_FIELD_CARD64"},
	"VISUAL":        {ctype: "X11_VISUAL", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"COLORMAP":      {ctype: "X11_COLORMAP", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"ATOM":          {ctype: "X11_ATOM", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"WINDOW":        {ctype: "X11_WINDOW", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"TIME":          {ctype: "X11_TIME", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"FONT":          {ctype: "X11_FONT", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"CURSOR":        {ctype: "X11_CURSOR", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"GCONTEXT":      {ctype: "X11_GCONTEXT", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"PIXMAP":        {ctype: "X11_PIXMAP", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"DRAWABLE":      {ctype: "X11_DRAWABLE", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"BYTE_ARRAY_31": {ctype: "X11_BYTE_ARRAY_31", base: "CARD8", arr: "[31]", size: 31, align: 1},
	"BYTE_ARRAY_32": {ctype: "X11_BYTE_ARRAY_32", base: "CARD8", arr: "[32]", size: 32, align: 1},
	"CHAR_INFO": {
		ctype: "X11_CharInfo", size: 12, align: 2,
		compound: []compoundMember{
			{name: "leftSideBearing", ctype: "INT16", swap: "X_REPLY_FIELD_CARD16"},
			{name: "rightSideBearing", ctype: "INT16", swap: "X_REPLY_FIELD_CARD16"},
			{name: "characterWidth", ctype: "INT16", swap: "X_REPLY_FIELD_CARD16"},
			{name: "ascent", ctype: "INT16", swap: "X_REPLY_FIELD_CARD16"},
			{name: "descent", ctype: "INT16", swap: "X_REPLY_FIELD_CARD16"},
			{name: "attributes", ctype: "CARD16", swap: "X_REPLY_FIELD_CARD16"},
		},
	},
}

// cField is a single emitted C struct member.
type cField struct {
	name     string
	ctype    string
	arr      int // array length, >0 for byte-array pads
	size     int
	swap     string           // swap macro for this field (payload multi-byte fields only)
	compound []compoundMember // non-nil for embedded struct fields (e.g. xCharInfo)
}

// structSpec is one parsed YAML struct definition (reply, request or event).
type structSpec struct {
	kind     string
	name     string
	dataName string
	dataType string
	noHeader bool
	payload  [][2]string // ordered field name / type pairs from YAML
}

// opcode is one parsed YAML opcode entry. name already carries any
// per-file opcode-prefix, value is the protocol opcode number.
type opcode struct {
	name  string
	value int
}

// structName returns the emitted C typedef name for a struct.
func structName(s *structSpec) string {
	switch s.kind {
	case kindRequest:
		return "X11_Request_" + s.name
	case kindEvent:
		return "X11_Event_" + s.name
	default:
		return "X11_Reply_" + s.name
	}
}

// swapMacro returns the emitted C byteswap macro name for a struct.
func swapMacro(s *structSpec) string {
	switch s.kind {
	case kindRequest:
		return "X11_REQUEST_" + s.name + "_SWAP()"
	case kindEvent:
		return "X11_EVENT_" + s.name + "_SWAP()"
	default:
		return "X11_REPLY_" + s.name + "_SWAP()"
	}
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "usage: x11-typegen [-o out.h] <yaml>...")
		os.Exit(2)
	}
	var outPath string
	inputs := []string{}
	args := os.Args[1:]
	for i := 0; i < len(args); i++ {
		if args[i] == "-o" && i+1 < len(args) {
			outPath = args[i+1]
			i++
		} else {
			inputs = append(inputs, args[i])
		}
	}

	var specs []structSpec
	var ops []opcode
	for _, in := range inputs {
		ss, oo, err := parseFile(in)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s: %v\n", in, err)
			os.Exit(1)
		}
		specs = append(specs, ss...)
		ops = append(ops, oo...)
	}

	out := renderHeader(specs, ops, outPath)
	if outPath != "" {
		if err := os.WriteFile(outPath, []byte(out), 0o644); err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
	} else {
		fmt.Print(out)
	}
}

// parseFile parses one YAML file into struct specs (preserving order) and
// opcode entries.
func parseFile(path string) ([]structSpec, []opcode, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, nil, err
	}
	defer f.Close()

	var doc yaml.Node
	if err := yaml.NewDecoder(f).Decode(&doc); err != nil {
		return nil, nil, err
	}

	root := doc.Content
	if len(root) == 0 {
		return nil, nil, fmt.Errorf("empty document")
	}
	rootMap := root[0]
	if rootMap.Kind != yaml.MappingNode {
		return nil, nil, fmt.Errorf("document root is not a mapping")
	}

	var specs []structSpec
	var ops []opcode
	prefix := ""
	pairs := keyValues(rootMap)
	for _, kv := range pairs {
		switch kv[0].Value { // accept both the spec default and the singular form
		case "reply-structs", "reply-struct":
			sub, err := parseStructList(kindReply, kv[1])
			if err != nil {
				return nil, nil, err
			}
			specs = append(specs, sub...)
		case "request-structs", "request-struct":
			sub, err := parseStructList(kindRequest, kv[1])
			if err != nil {
				return nil, nil, err
			}
			specs = append(specs, sub...)
		case "events", "event":
			sub, err := parseStructList(kindEvent, kv[1])
			if err != nil {
				return nil, nil, err
			}
			specs = append(specs, sub...)
		case "opcode-prefix":
			prefix = kv[1].Value
		case "opcodes", "opcode":
			oo, err := parseOpcodes(prefix, kv[1])
			if err != nil {
				return nil, nil, err
			}
			ops = append(ops, oo...)
		}
	}
	return specs, ops, nil
}

// parseStructList parses the value of a struct section: a mapping of struct
// name -> definition.
func parseStructList(kind string, n *yaml.Node) ([]structSpec, error) {
	if n.Kind != yaml.MappingNode {
		return nil, fmt.Errorf("struct section is not a mapping")
	}
	var out []structSpec
	for _, kv := range keyValues(n) {
		spec, err := parseStruct(kind, kv[0].Value, kv[1])
		if err != nil {
			return nil, err
		}
		out = append(out, spec)
	}
	return out, nil
}

func parseStruct(kind, name string, n *yaml.Node) (structSpec, error) {
	if n.Kind != yaml.MappingNode {
		return structSpec{}, fmt.Errorf("struct %q: not a mapping", name)
	}
	spec := structSpec{kind: kind, name: name, dataType: "CARD8"}
	for _, kv := range keyValues(n) {
		switch kv[0].Value {
		case "data-name":
			spec.dataName = kv[1].Value
		case "data-type":
			spec.dataType = kv[1].Value
		case "header":
			spec.noHeader = kv[1].Value == "false" || kv[1].Value == "no" || kv[1].Value == "0"
		case "payload":
			payload, err := parsePayload(kv[1])
			if err != nil {
				return spec, fmt.Errorf("struct %q: %v", name, err)
			}
			spec.payload = payload
		}
	}
	return spec, nil
}

func parsePayload(n *yaml.Node) ([][2]string, error) {
	if n.Kind != yaml.MappingNode {
		return nil, fmt.Errorf("payload is not a mapping")
	}
	var out [][2]string
	for _, kv := range keyValues(n) {
		t := kv[1].Value
		if _, ok := typeTable[t]; !ok {
			return nil, fmt.Errorf("unknown field type %q", t)
		}
		out = append(out, [2]string{kv[0].Value, t})
	}
	return out, nil
}

func parseOpcodes(prefix string, n *yaml.Node) ([]opcode, error) {
	if n.Kind != yaml.MappingNode {
		return nil, fmt.Errorf("opcode section is not a mapping")
	}
	var out []opcode
	for _, kv := range keyValues(n) {
		v, err := strconv.Atoi(kv[1].Value)
		if err != nil {
			return nil, fmt.Errorf("opcode %q: invalid value %q", kv[0].Value, kv[1].Value)
		}
		name := kv[0].Value
		if prefix != "" {
			name = prefix + "_" + name
		}
		out = append(out, opcode{name: name, value: v})
	}
	return out, nil
}

// keyValues returns ordered key/value pairs of a mapping node.
func keyValues(n *yaml.Node) [][2]*yaml.Node {
	var out [][2]*yaml.Node
	for i := 0; i+1 < len(n.Content); i += 2 {
		out = append(out, [2]*yaml.Node{n.Content[i], n.Content[i+1]})
	}
	return out
}

// alignTo rounds offset up to a multiple of a.
func alignTo(offset, a int) int {
	if a <= 1 {
		return offset
	}
	rem := offset % a
	if rem == 0 {
		return offset
	}
	return offset + (a - rem)
}

// defaultDataName returns the header data-byte field name per kind.
func defaultDataName(kind string) string {
	switch kind {
	case kindRequest:
		return "data"
	case kindEvent:
		return "detail"
	default:
		return "data1"
	}
}

// layoutFields computes the full ordered C field list for a struct,
// including the protocol header and X11 padding. Returns total struct size.
func layoutFields(s *structSpec) ([]cField, int) {
	var fields []cField
	offset := 0

	addField := func(f cField) {
		fields = append(fields, f)
		offset += f.size
	}

	used := map[string]bool{}
	if !s.noHeader {
		// protocol header. The data byte is handled by the generic write
		// helpers, never by the SWAP macro.
		//
		// The request byte 0 is named reqType (as in libX11's xChangePropertyReq
		// and friends) because the wire protocol re-uses "type" for the ATOM
		// member of ChangeProperty. Events/replies keep "type".
		byte0 := "type"
		if s.kind == kindRequest {
			byte0 = "reqType"
		}
		addField(cField{name: byte0, ctype: "X11_BYTE", size: 1})
		used[byte0] = true

		dname := s.dataName
		if dname == "" {
			dname = defaultDataName(s.kind)
		}
		dti, ok := typeTable[s.dataType]
		if !ok {
			dti = typeTable["CARD8"]
		}
		addField(cField{name: dname, ctype: dti.ctype, size: dti.size})
		used[dname] = true

		switch s.kind {
		case kindRequest:
			// request header: [opcode, data, length] — no sequence number
			addField(cField{name: "length", ctype: "X11_CARD16", size: 2})
			used["length"] = true
		case kindEvent:
			// event header: [type, detail, sequenceNumber] — no length
			addField(cField{name: "sequenceNumber", ctype: "X11_CARD16", size: 2})
			used["sequenceNumber"] = true
		default: // reply
			addField(cField{name: "sequenceNumber", ctype: "X11_CARD16", size: 2})
			addField(cField{name: "length", ctype: "X11_CARD32", size: 4})
			used["sequenceNumber"] = true
			used["length"] = true
		}
	}

	padNo := 1
	addPad := func(gap int) {
		// name pads pad1, pad2, ... avoiding payload field names
		name := fmt.Sprintf("pad%d", padNo)
		for used[name] {
			padNo++
			name = fmt.Sprintf("pad%d", padNo)
		}
		padNo++
		fields = append(fields, cField{name: name, ctype: "X11_CARD8", arr: gap, size: gap})
		offset += gap
	}

	// payload fields, aligned to natural boundaries, pads inserted in between
	for _, kv := range s.payload {
		fname, ftype := kv[0], kv[1]
		ti := typeTable[ftype]
		used[fname] = true
		if a := alignTo(offset, ti.align); a > offset {
			addPad(a - offset)
		}
		swap := ti.swap
		addField(cField{name: fname, ctype: ti.ctype, size: ti.size, swap: swap, compound: ti.compound})
	}

	// tail padding per kind:
	//   reply/event: at least 32 bytes (fixed-size), multiple of 4
	//   request:     requests are 4-byte multiples, no 32-byte minimum
	target := offset
	if s.kind == kindRequest {
		target = alignTo(target, 4)
	} else {
		if target < 32 {
			target = 32
		}
		target = alignTo(target, 4)
	}
	if target > offset {
		addPad(target - offset)
	}

	return fields, offset
}

// renderHeader produces the full generated C header.
func renderHeader(specs []structSpec, ops []opcode, outPath string) string {
	var b strings.Builder

	guard := "X11_CORE_REPLY_H"
	if outPath != "" {
		base := strings.ToUpper(filepath.Base(outPath))
		base = strings.Map(func(r rune) rune {
			if (r >= 'A' && r <= 'Z') || (r >= '0' && r <= '9') || r == '_' {
				return r
			}
			return '_'
		}, base)
		if base != "" {
			guard = base
		}
	}

	fmt.Fprintf(&b, "/* generated by x11-typegen - do not edit */\n\n")
	fmt.Fprintf(&b, "#ifndef %s\n#define %s\n\n", guard, guard)
	fmt.Fprintf(&b, "#include <X11/Xmd.h>\n")
	fmt.Fprintf(&b, "#include <X11/Xdefs.h>\n")
	fmt.Fprintf(&b, "#include <X11/X.h>\n\n")

	// collect used types, emit typedef block in stable order
	// header fields always need BYTE/CARD16/CARD32
	used := map[string]bool{"BYTE": true, "CARD16": true, "CARD32": true}
	for _, s := range specs {
		if _, ok := typeTable[s.dataType]; ok {
			used[s.dataType] = true
		}
		for _, kv := range s.payload {
			used[kv[1]] = true
		}
	}
	fmt.Fprintf(&b, "/* X11_* base typedefs */\n")
	names := make([]string, 0, len(used))
	for t := range used {
		names = append(names, t)
	}
	sort.Strings(names)
	for _, t := range names {
		ti := typeTable[t]
		if ti.compound != nil {
			// embedded struct type (e.g. X11_CharInfo)
			fmt.Fprintf(&b, "typedef struct {\n")
			for _, m := range ti.compound {
				fmt.Fprintf(&b, "    %s %s;\n", m.ctype, m.name)
			}
			fmt.Fprintf(&b, "} %s;\n", ti.ctype)
		} else {
			fmt.Fprintf(&b, "typedef %s %s%s;\n", ti.base, ti.ctype, ti.arr)
		}
	}
	fmt.Fprintf(&b, "\n")

	// opcodes (prefix-resolved symbols), emitted sorted for determinism
	if len(ops) > 0 {
		sorted := make([]opcode, len(ops))
		copy(sorted, ops)
		sort.Slice(sorted, func(i, j int) bool { return sorted[i].name < sorted[j].name })
		fmt.Fprintf(&b, "/* opcodes */\n")
		for _, op := range sorted {
			fmt.Fprintf(&b, "#define X11_OP_%s %d\n", op.name, op.value)
		}
		fmt.Fprintf(&b, "\n")
	}

	// emit the structs in declaration order (as given by the yaml file)
	for _, s := range specs {
		fields, size := layoutFields(&s)
		fmt.Fprintf(&b, "typedef struct {\n")
		for _, f := range fields {
			if f.arr > 0 {
				fmt.Fprintf(&b, "    %s %s[%d];\n", f.ctype, f.name, f.arr)
			} else {
				fmt.Fprintf(&b, "    %s %s;\n", f.ctype, f.name)
			}
		}
		fmt.Fprintf(&b, "} %s;   /* %d bytes */\n\n", structName(&s), size)

		// swap macro: header fields (type/data/sequenceNumber/length) are
		// handled by the generic write helpers; only payload multi-byte
		// fields are swapped here (see dbe.c usage)
		var swaps []string
		for _, f := range fields {
			if f.compound != nil {
				// embedded struct: swap each multi-byte member via field.member
				for _, m := range f.compound {
					if m.swap != "" {
						swaps = append(swaps, fmt.Sprintf("        %s(%s.%s);", m.swap, f.name, m.name))
					}
				}
			} else if f.swap != "" {
				swaps = append(swaps, fmt.Sprintf("        %s(%s);", f.swap, f.name))
			}
		}
		fmt.Fprintf(&b, "#define %s \\\n", swapMacro(&s))
		if len(swaps) == 0 {
			fmt.Fprintf(&b, "    do { } while (0)\n\n")
		} else {
			fmt.Fprintf(&b, "    do { \\\n")
			// every line of the macro body must be continued, incl. the last one
			fmt.Fprintf(&b, "%s \\\n", strings.Join(swaps, " \\\n"))
			fmt.Fprintf(&b, "    } while (0)\n\n")
		}
	}

	fmt.Fprintf(&b, "#endif /* %s */\n", guard)
	return b.String()
}
