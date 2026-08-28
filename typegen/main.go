package main

import (
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"gopkg.in/yaml.v3"
)

// typeInfo describes an X11 protocol scalar type used in reply structs.
type typeInfo struct {
	ctype string // emitted C type name (X11_*) as used in struct members
	base  string // underlying C type for the typedef (e.g. CARD32)
	arr   string // array suffix for the typedef (e.g. "[32]"), "" if scalar
	size  int    // size in bytes
	align int    // wire/C alignment requirement
	swap  string // X_REPLY_FIELD_* macro name for byteswap, "" if none
}

// typeTable maps YAML type names onto their X11_* C representation.
// Window/Atom/VisualID/Colormap are all CARD32 XIDs in the wire protocol.
var typeTable = map[string]typeInfo{
	"BYTE":          {ctype: "X11_BYTE", base: "CARD8", size: 1, align: 1},
	"CARD8":         {ctype: "X11_CARD8", base: "CARD8", size: 1, align: 1},
	"BOOL":          {ctype: "X11_BOOL", base: "CARD8", size: 1, align: 1},
	"CARD16":        {ctype: "X11_CARD16", base: "CARD16", size: 2, align: 2, swap: "X_REPLY_FIELD_CARD16"},
	"INT16":         {ctype: "X11_INT16", base: "INT16", size: 2, align: 2, swap: "X_REPLY_FIELD_CARD16"},
	"CARD32":        {ctype: "X11_CARD32", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"INT32":         {ctype: "X11_INT32", base: "INT32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"CARD64":        {ctype: "X11_CARD64", base: "CARD64", size: 8, align: 8, swap: "X_REPLY_FIELD_CARD64"},
	"VISUAL":        {ctype: "X11_VISUAL", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"COLORMAP":      {ctype: "X11_COLORMAP", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"ATOM":          {ctype: "X11_ATOM", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"WINDOW":        {ctype: "X11_WINDOW", base: "CARD32", size: 4, align: 4, swap: "X_REPLY_FIELD_CARD32"},
	"BYTE_ARRAY_32": {ctype: "X11_BYTE_ARRAY_32", base: "CARD8", arr: "[32]", size: 32, align: 1},
}

// cField is a single emitted C struct member.
type cField struct {
	name  string
	ctype string
	arr   int // array length, >0 for byte-array pads
	size  int
	swap  string // swap macro for this field (payload multi-byte fields only)
}

// replySpec is one parsed reply-struct entry.
type replySpec struct {
	name     string
	dataName string
	dataType string
	payload  [][2]string // ordered field name / type pairs from YAML
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

	var specs []replySpec
	for _, in := range inputs {
		s, err := parseFile(in)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s: %v\n", in, err)
			os.Exit(1)
		}
		specs = append(specs, s...)
	}

	out := renderHeader(specs, outPath)
	if outPath != "" {
		if err := os.WriteFile(outPath, []byte(out), 0o644); err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
	} else {
		fmt.Print(out)
	}
}

// parseFile parses one YAML file into reply specs (preserving order).
func parseFile(path string) ([]replySpec, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var doc yaml.Node
	if err := yaml.NewDecoder(f).Decode(&doc); err != nil {
		return nil, err
	}

	root := doc.Content
	if len(root) == 0 {
		return nil, fmt.Errorf("empty document")
	}
	rootMap := root[0]
	if rootMap.Kind != yaml.MappingNode {
		return nil, fmt.Errorf("document root is not a mapping")
	}

	var specs []replySpec
	pairs := keyValues(rootMap)
	for _, kv := range pairs {
		switch kv[0].Value { // accept both the spec default and the singular form actually used
		case "reply-structs", "reply-struct":
			sub, err := parseStructList(kv[1])
			if err != nil {
				return nil, err
			}
			specs = append(specs, sub...)
		}
	}
	return specs, nil
}

// parseStructList parses the value of `reply-struct(s)`: a mapping of struct
// name -> definition.
func parseStructList(n *yaml.Node) ([]replySpec, error) {
	if n.Kind != yaml.MappingNode {
		return nil, fmt.Errorf("reply-struct section is not a mapping")
	}
	var out []replySpec
	for _, kv := range keyValues(n) {
		spec, err := parseStruct(kv[0].Value, kv[1])
		if err != nil {
			return nil, err
		}
		out = append(out, spec)
	}
	return out, nil
}

func parseStruct(name string, n *yaml.Node) (replySpec, error) {
	if n.Kind != yaml.MappingNode {
		return replySpec{}, fmt.Errorf("struct %q: not a mapping", name)
	}
	spec := replySpec{name: name, dataType: "CARD8"}
	for _, kv := range keyValues(n) {
		switch kv[0].Value {
		case "data-name":
			spec.dataName = kv[1].Value
		case "data-type":
			spec.dataType = kv[1].Value
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

// layoutFields computes the full ordered C field list for a reply struct,
// including automatically added header fields and X11 padding.
// Returns total struct size.
func layoutFields(s *replySpec) ([]cField, int) {
	var fields []cField
	offset := 0

	addField := func(f cField) {
		fields = append(fields, f)
		offset += f.size
	}

	// standard protocol header
	addField(cField{name: "type", ctype: "X11_BYTE", size: 1})

	dname := s.dataName
	if dname == "" {
		dname = "data1"
	}
	dti, ok := typeTable[s.dataType]
	if !ok {
		dti = typeTable["CARD8"]
	}
	// header data byte is handled by __write_reply_hdr_* → never in swap macro
	addField(cField{name: dname, ctype: dti.ctype, size: dti.size})

	addField(cField{name: "sequenceNumber", ctype: "X11_CARD16", size: 2})
	addField(cField{name: "length", ctype: "X11_CARD32", size: 4})

	used := map[string]bool{"type": true, dname: true, "sequenceNumber": true, "length": true}
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
		addField(cField{name: fname, ctype: ti.ctype, size: ti.size, swap: swap})
	}

	// tail padding: at least 32 bytes, always a multiple of 4
	target := offset
	if target < 32 {
		target = 32
	}
	target = alignTo(target, 4)
	if target > offset {
		addPad(target - offset)
	}

	return fields, offset
}

// renderHeader produces the full generated C header.
func renderHeader(specs []replySpec, outPath string) string {
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
		fmt.Fprintf(&b, "typedef %s %s%s;\n", ti.base, ti.ctype, ti.arr)
	}
	fmt.Fprintf(&b, "\n")

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
		fmt.Fprintf(&b, "} X11_Reply_%s;   /* %d bytes */\n\n", s.name, size)

		// swap macro: header fields (type/data/sequenceNumber/length) are
		// handled by __write_reply_hdr_* / X_SEND_REPLY_WITH_RPCBUF, so only
		// payload multi-byte fields are swapped here (see dbe.c usage)
		var swaps []string
		for _, f := range fields {
			if f.swap != "" {
				swaps = append(swaps, fmt.Sprintf("        %s(%s);", f.swap, f.name))
			}
		}
		fmt.Fprintf(&b, "#define X11_REPLY_%s_SWAP() \\\n", s.name)
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
