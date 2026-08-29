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
	"InternAtom":             32,
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

// expectedRequestSizes maps request struct names onto their X11 protocol
// size (verified against sz_x*Req macros in X11/Xproto.h).
var expectedRequestSizes = map[string]int{
	"CreateWindow":            32,
	"ChangeWindowAttributes":  12,
	"GetWindowAttributes":     8,
	"DestroyWindow":           8,
	"DestroySubwindows":       8,
	"ChangeSaveSet":           8,
	"ReparentWindow":          16,
	"MapWindow":               8,
	"MapSubwindows":           8,
	"UnmapWindow":             8,
	"UnmapSubwindows":         8,
	"ConfigureWindow":         12,
	"CirculateWindow":         8,
	"GetGeometry":             8,
	"QueryTree":               8,
	"InternAtom":              8,
	"GetAtomName":             8,
	"ChangeProperty":          24,
	"DeleteProperty":          12,
	"GetProperty":             24,
	"ListProperties":          8,
	"SetSelectionOwner":       16,
	"GetSelectionOwner":       8,
	"ConvertSelection":        24,
	"SendEvent":               44,
	"GrabPointer":             24,
	"UngrabPointer":           8,
	"GrabButton":              24,
	"UngrabButton":            12,
	"ChangeActivePointerGrab": 16,
	"GrabKeyboard":            16,
	"UngrabKeyboard":          8,
	"GrabKey":                 16,
	"UngrabKey":               12,
	"AllowEvents":             8,
	"GrabServer":              4,
	"UngrabServer":            4,
	"QueryPointer":            8,
	"GetMotionEvents":         16,
	"TranslateCoords":         16,
	"WarpPointer":             24,
	"SetInputFocus":           12,
	"GetInputFocus":           4,
	"QueryKeymap":             4,
	"OpenFont":                12,
	"CloseFont":               8,
	"QueryFont":               8,
	"QueryTextExtents":        8,
	"ListFonts":               8,
	"ListFontsWithInfo":       8,
	"SetFontPath":             8,
	"GetFontPath":             4,
	"CreatePixmap":            16,
	"FreePixmap":              8,
	"CreateGC":                16,
	"ChangeGC":                12,
	"CopyGC":                  16,
	"SetDashes":               12,
	"SetClipRectangles":       12,
	"FreeGC":                  8,
	"ClearArea":               16,
	"CopyArea":                28,
	"CopyPlane":               32,
	"PolyPoint":               12,
	"PolyLine":                12,
	"PolySegment":             12,
	"PolyRectangle":           12,
	"PolyArc":                 12,
	"FillPoly":                16,
	"PolyFillRectangle":       12,
	"PolyFillArc":             12,
	"PutImage":                24,
	"GetImage":                20,
	"PolyText8":               16,
	"PolyText16":              16,
	"ImageText8":              16,
	"ImageText16":             16,
	"CreateColormap":          16,
	"FreeColormap":            8,
	"CopyColormapAndFree":     12,
	"InstallColormap":         8,
	"UninstallColormap":       8,
	"ListInstalledColormaps":  8,
	"AllocColor":              16,
	"AllocNamedColor":         12,
	"AllocColorCells":         12,
	"AllocColorPlanes":        16,
	"FreeColors":              12,
	"StoreColors":             8,
	"StoreNamedColor":         16,
	"QueryColors":             8,
	"LookupColor":             12,
	"CreateCursor":            32,
	"CreateGlyphCursor":       32,
	"FreeCursor":              8,
	"RecolorCursor":           20,
	"QueryBestSize":           12,
	"QueryExtension":          8,
	"ListExtensions":          4,
	"ChangeKeyboardMapping":   8,
	"GetKeyboardMapping":      8,
	"ChangeKeyboardControl":   8,
	"GetKeyboardControl":      4,
	"Bell":                    4,
	"ChangePointerControl":    12,
	"GetPointerControl":       4,
	"SetScreenSaver":          12,
	"GetScreenSaver":          4,
	"ChangeHosts":             8,
	"ListHosts":               4,
	"SetAccessControl":        4,
	"SetCloseDownMode":        4,
	"KillClient":              8,
	"RotateProperties":        12,
	"ForceScreenSaver":        4,
	"SetPointerMapping":       4,
	"GetPointerMapping":       4,
	"SetModifierMapping":      4,
	"GetModifierMapping":      4,
	"NoOperation":             4,
}

// expectedEventSizes maps event struct names onto their X11 protocol size.
// All core events are fixed 32 bytes (the xEvent union size).
var expectedEventSizes = map[string]int{
	"KeyButtonPointer": 32,
	"EnterLeave":       32,
	"Focus":            32,
	"Expose":           32,
	"GraphicsExposure": 32,
	"NoExposure":       32,
	"Visibility":       32,
	"CreateNotify":     32,
	"DestroyNotify":    32,
	"UnmapNotify":      32,
	"MapNotify":        32,
	"MapRequest":       32,
	"Reparent":         32,
	"ConfigureNotify":  32,
	"ConfigureRequest": 32,
	"Gravity":          32,
	"ResizeRequest":    32,
	"Circulate":        32,
	"Property":         32,
	"SelectionClear":   32,
	"SelectionRequest": 32,
	"SelectionNotify":  32,
	"Colormap":         32,
	"MappingNotify":    32,
	"ClientMessage":    32,
	"Keymap":           32,
}

// expectedShapeSizes covers the SHAPE extension (shape.yaml): minor opcodes
// 0-8, sizes verified against sz_xShape* macros in
// X11/extensions/shapeproto.h. Reply/request/event names may collide (e.g.
// ShapeQueryVersion appears as both request and reply), so this is keyed
// per kind.
var expectedShapeSizes = map[string]map[string]int{
	kindReply: {
		"ShapeQueryVersion":  32,
		"ShapeQueryExtents":  32,
		"ShapeInputSelected": 32,
		"ShapeGetRectangles": 32,
	},
	kindRequest: {
		"ShapeQueryVersion":  4,
		"ShapeRectangles":    16,
		"ShapeMask":          20,
		"ShapeCombine":       20,
		"ShapeOffset":        16,
		"ShapeQueryExtents":  8,
		"ShapeSelectInput":   12,
		"ShapeInputSelected": 8,
		"ShapeGetRectangles": 12,
	},
	kindEvent: {
		"ShapeNotify": 32,
	},
}

func TestShapeSizes(t *testing.T) {
	specs, ops, err := parseFile("shape.yaml")
	if err != nil {
		t.Fatalf("parseFile: %v", err)
	}
	if len(ops) != 9 {
		t.Fatalf("parsed %d shape opcodes, want 9", len(ops))
	}
	wantTotal := 0
	for _, m := range expectedShapeSizes {
		wantTotal += len(m)
	}
	if len(specs) != wantTotal {
		t.Fatalf("parsed %d shape structs, want %d", len(specs), wantTotal)
	}
	for _, s := range specs {
		want, ok := expectedShapeSizes[s.kind][s.name]
		if !ok {
			t.Errorf("unexpected shape struct %q (%s)", s.name, s.kind)
			continue
		}
		_, size := layoutFields(&s)
		if size != want {
			t.Errorf("shape %s %s: layout size = %d, want %d", s.kind, s.name, size, want)
		}
	}
	for _, op := range ops {
		if !strings.HasPrefix(op.name, "SHAPE_") {
			t.Errorf("shape opcode %q missing SHAPE_ prefix", op.name)
		}
	}
}

func TestLayoutSizes(t *testing.T) {
	specs, _, err := parseFile("core.yaml")
	if err != nil {
		t.Fatalf("parseFile: %v", err)
	}
	if len(specs) != len(expectedSizes)+len(expectedRequestSizes)+len(expectedEventSizes) {
		t.Fatalf("parsed %d structs, want %d", len(specs), len(expectedSizes)+len(expectedRequestSizes)+len(expectedEventSizes))
	}
	for _, s := range specs {
		m := map[string]int{}
		switch s.kind {
		case kindRequest:
			m = expectedRequestSizes
		case kindEvent:
			m = expectedEventSizes
		default:
			m = expectedSizes
		}
		want, ok := m[s.name]
		if !ok {
			t.Errorf("unexpected %s struct %q", s.kind, s.name)
			continue
		}
		fields, size := layoutFields(&s)
		if size != want {
			t.Errorf("%s %s: layout size = %d, want %d", s.kind, s.name, size, want)
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
	specs, ops, err := parseFile("core.yaml")
	if err != nil {
		t.Fatalf("parseFile: %v", err)
	}
	out := renderHeader(specs, ops, "x11-core-reply.h")
	for _, s := range specs {
		// struct + swap macro present
		if !strings.Contains(out, "} "+structName(&s)+";") {
			t.Errorf("%s: struct missing", s.name)
		}
		if !strings.Contains(out, strings.TrimSuffix(swapMacro(&s), "()")) {
			t.Errorf("%s: swap macro missing", s.name)
		}
	}
	// typedefs emitted for every used type
	for _, want := range []string{
		"typedef CARD8 X11_BYTE;",
		"typedef CARD32 X11_WINDOW;",
		"typedef CARD8 X11_BYTE_ARRAY_32[32];",
		"typedef CARD16 X11_KEYBUTMASK;",
		"typedef CARD32 X11_TIME;",
		"typedef INT8 X11_INT8;",
		"typedef CARD8 X11_BYTE_ARRAY_31[31];",
	} {
		if !strings.Contains(out, want) {
			t.Errorf("missing typedef %q", want)
		}
	}
	// reply header fields always present
	for _, want := range []string{"sequenceNumber", "X11_CARD32 length;", "data1"} {
		if !strings.Contains(out, want) {
			t.Errorf("missing reply header element %q", want)
		}
	}
	// request header has a CARD16 length, no sequenceNumber
	if !strings.Contains(out, "X11_CARD16 length;") {
		t.Errorf("request CARD16 length missing")
	}
	if strings.Contains(out, "sequenceNumber\n") {
		t.Errorf("request structs must not emit sequenceNumber")
	}
	for _, want := range []string{
		"typedef struct {\n    X11_BYTE reqType;\n    X11_CARD8 data;\n    X11_CARD16 length;\n    X11_WINDOW window;",
		"} X11_Request_ChangeWindowAttributes;",
		"X11_REQUEST_ChangeWindowAttributes_SWAP()",
	} {
		if !strings.Contains(out, want) {
			t.Errorf("missing request element %q", want)
		}
	}
	// events have an event header (detail byte, sequenceNumber, no length)
	if !strings.Contains(out, "X11_CARD16 sequenceNumber;\n    X11_WINDOW window;") {
		t.Errorf("event header/payload layout wrong")
	}
	for _, want := range []string{
		"} X11_Event_KeyButtonPointer;",
		"X11_EVENT_KeyButtonPointer_SWAP()",
		"X11_EVENT_Keymap_SWAP()",
	} {
		if !strings.Contains(out, want) {
			t.Errorf("missing event element %q", want)
		}
	}
	// keymap event render skips the protocol header (type + 31-byte map only)
	if !strings.Contains(out, "} X11_Event_Keymap;   /* 32 bytes */") {
		t.Errorf("Keymap event missing")
	}
	if !strings.Contains(out, "X11_BYTE_ARRAY_31 map;") {
		t.Errorf("Keymap map array missing")
	}
	// opcodes emitted
	if !strings.Contains(out, "#define X11_OP_CREATE_WINDOW 1") {
		t.Errorf("opcode define missing")
	}
	if !strings.Contains(out, "#define X11_OP_NO_OPERATION 127") {
		t.Errorf("NO_OPERATION opcode missing")
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

func TestRenderShape(t *testing.T) {
	specs, ops, err := parseFile("shape.yaml")
	if err != nil {
		t.Fatalf("parseFile: %v", err)
	}
	out := renderHeader(specs, ops, "x11-shape.h")
	for _, want := range []string{
		"#define X11_OP_SHAPE_QUERY_VERSION 0",
		"#define X11_OP_SHAPE_GET_RECTANGLES 8",
		"} X11_Request_ShapeCombine;",
		"} X11_Event_ShapeNotify;",
		"X11_REQUEST_ShapeRectangles_SWAP()",
	} {
		if !strings.Contains(out, want) {
			t.Errorf("missing shape element %q", want)
		}
	}
	// SHAPE prefix must not leak into core opcode symbols
	if strings.Contains(out, "X11_OP_SHAPE_CREATE_WINDOW") {
		t.Errorf("shape prefix leaked into an unrelated opcode")
	}
}
