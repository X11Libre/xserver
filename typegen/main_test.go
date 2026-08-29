package main

import (
	"strings"
	"testing"
)

// expectedSizes maps reply struct names onto their X11 protocol size
// (verified against sz_* macros in X11/Xproto.h).
var expectedSizes = map[string]int{
	"GenericReply":           32,
	"GetWindowAttributes":    44,
	"GetGeometry":            32,
	"QueryTree":              32,
	"IntenAtom":              32,
	"GetAtomName":            32,
	"GetProperty":            32,
	"ListProperties":         32,
	"GetSelectionOwner":      32,
	"GrabPointer":            32,
	"GrabKeyboardReply":      32,
	"QueryPointer":           32,
	"GetMotionEvents":        32,
	"TranslateCoords":        32,
	"GetInputFocus":          32,
	"QueryKeymap":            40,
	"QueryFont":              60,
	"QueryTextExtents":       32,
	"ListFonts":              32,
	"ListFontsWithInfo":      60,
	"GetFontPath":            32,
	"GetImage":               32,
	"ListInstalledColormaps": 32,
	"AllocColor":             32,
	"AllocNamedColor":        32,
	"AllocColorCells":        32,
	"AllocColorPlanes":       32,
	"QueryColors":            32,
	"LookupColor":            32,
	"QueryBestSize":          32,
	"QueryExtension":         32,
	"ListExtensions":         32,
	"SetMapping":             32,
	"SetPointerMapping":      32,
	"SetModifierMapping":     32,
	"GetPointerMapping":      32,
	"GetKeyboardMapping":     32,
	"GetModifierMapping":     32,
	"GetKeyboardControl":     52,
	"GetPointerControl":      32,
	"GetScreenSaver":         32,
	"ListHosts":              32,
}

func TestLayoutSizes(t *testing.T) {
	specs, err := parseFile("core.yaml")
	if err != nil {
		t.Fatalf("parseFile: %v", err)
	}
	if len(specs) != len(expectedSizes) {
		t.Fatalf("parsed %d structs, want %d", len(specs), len(expectedSizes))
	}
	for _, s := range specs {
		want, ok := expectedSizes[s.name]
		if !ok {
			t.Errorf("unexpected struct %q", s.name)
			continue
		}
		fields, size := layoutFields(&s)
		if size != want {
			t.Errorf("%s: layout size = %d, want %d", s.name, size, want)
		}
		// byte-array pads must exactly cover their gap
		off := 0
		for _, f := range fields {
			if f.arr > 0 {
				if off+f.size != off+f.arr {
					t.Errorf("%s: pad %s size=%d arr=%d mismatch", s.name, f.name, f.size, f.arr)
				}
			}
			off += f.size
		}
	}
}

func TestRender(t *testing.T) {
	specs, err := parseFile("core.yaml")
	if err != nil {
		t.Fatalf("parseFile: %v", err)
	}
	out := renderHeader(specs, "x11-core-reply.h")
	for _, s := range specs {
		// struct + swap macro present
		if !strings.Contains(out, "} X11_Reply_"+s.name+";") {
			t.Errorf("%s: struct missing", s.name)
		}
		if !strings.Contains(out, "X11_REPLY_"+s.name+"_SWAP()") {
			t.Errorf("%s: swap macro missing", s.name)
		}
	}
	// typedefs emitted for every used type
	for _, want := range []string{
		"typedef CARD8 X11_BYTE;",
		"typedef CARD32 X11_WINDOW;",
		"typedef CARD8 X11_BYTE_ARRAY_32[32];",
	} {
		if !strings.Contains(out, want) {
			t.Errorf("missing typedef %q", want)
		}
	}
	// header fields always present
	for _, want := range []string{"sequenceNumber", "X11_CARD32 length;", "data1"} {
		if !strings.Contains(out, want) {
			t.Errorf("missing header element %q", want)
		}
	}
	// embedded compound type (xCharInfo) emitted once + used in the font replies
	if !strings.Contains(out, "typedef struct {\n    INT16 leftSideBearing;") {
		t.Errorf("X11_CharInfo compound typedef missing")
	}
	if !strings.Contains(out, "X11_CharInfo minBounds;") {
		t.Errorf("QueryFont minBounds member missing")
	}
	if !strings.Contains(out, "X_REPLY_FIELD_CARD16(minBounds.characterWidth);") {
		t.Errorf("compound member swap expansion missing")
	}
	// GetKeyboardControl: explicit CARD16 pad keeps the map at wire offset 20
	if !strings.Contains(out, "X11_CARD16 pad;\n    X11_BYTE_ARRAY_32 map;") {
		t.Errorf("GetKeyboardControl mid-pad + map layout wrong")
	}
	// byte-array typedef must not be aliased to itself
	if strings.Contains(out, "typedef CARD8 X11_BYTE_ARRAY_32 X11_BYTE_ARRAY_32") {
		t.Errorf("self-referential BYTE_ARRAY_32 typedef")
	}
}
