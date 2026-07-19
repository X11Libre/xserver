#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

static const TokenTable TopLevelTab[] = {
    {DISABLE_EXTENSION, "disable_extension"},
    {ENABLE_EXTENSION, "enable_extension"},
    {DISABLE_MODULE, "disable_module"},
    {ENABLE_MODULE, "enable_module"},
    {-1, ""},
};

// Modules and Extensions
// Enable: 0 for disabled 1 for enabled
// Type: 0 for module and 1 for extension
typedef struct
{
	const char *name;
	unsigned char enable;
	unsigned char type;
} ME;

typedef struct
{
	ME me[16];
} Config;

// Error messages
#define INVALID_KEY "%s is not a valid keyword."

// old
#define BAD_OPTION_MSG "The Option keyword requires 1 or 2 quoted strings to follow it."
#define INVALID_KEYWORD_MSG "\"%s\" is not a valid keyword in this section."
#define INVALID_SECTION_MSG "\"%s\" is not a valid section name."
#define UNEXPECTED_EOF_MSG "Unexpected EOF. Missing EndSection keyword?"
#define QUOTE_MSG "The %s keyword requires a quoted string to follow it."
#define NUMBER_MSG "The %s keyword requires a number to follow it."
#define POSITIVE_INT_MSG "The %s keyword requires a positive integer to follow it."
#define BOOL_MSG "The %s keyword requires a boolean to follow it."
#define ZAXISMAPPING_MSG "The ZAxisMapping keyword requires 2 positive numbers or X or Y to follow it."
#define DACSPEED_MSG "The DacSpeed keyword must be followed by a list of up to %d numbers."
#define DISPLAYSIZE_MSG "The DisplaySize keyword must be followed by the width and height in mm."
#define HORIZSYNC_MSG "The HorizSync keyword must be followed by a list of numbers or ranges."
#define VERTREFRESH_MSG "The VertRefresh keyword must be followed by a list of numbers or ranges."
#define VIEWPORT_MSG "The Viewport keyword must be followed by an X and Y value."
#define VIRTUAL_MSG "The Virtual keyword must be followed by a width and height value."
#define WEIGHT_MSG "The Weight keyword must be followed by red, green and blue values."
#define BLACK_MSG "The Black keyword must be followed by red, green and blue values."
#define WHITE_MSG "The White keyword must be followed by red, green and blue values."
#define SCREEN_MSG "The Screen keyword must be followed by an optional number, a screen name\n" "\tin quotes, and optional position/layout information."
#define INVALID_SCR_MSG "Invalid Screen line."
#define INPUTDEV_MSG "The InputDevice keyword must be followed by an input device name in quotes."
#define INACTIVE_MSG "The Inactive keyword must be followed by a Device name in quotes."
#define UNDEFINED_SCREEN_MSG "Undefined Screen \"%s\" referenced by ServerLayout \"%s\"."
#define UNDEFINED_MODES_MSG "Undefined Modes Section \"%s\" referenced by Monitor \"%s\"."
#define UNDEFINED_DEVICE_MSG "Undefined Device \"%s\" referenced by Screen \"%s\"."
#define UNDEFINED_ADAPTOR_MSG "Undefined VideoAdaptor \"%s\" referenced by Screen \"%s\"."
#define ADAPTOR_REF_TWICE_MSG "VideoAdaptor \"%s\" already referenced by Screen \"%s\"."
#define UNDEFINED_DEVICE_LAY_MSG "Undefined Device \"%s\" referenced by ServerLayout \"%s\"."
#define UNDEFINED_INPUT_MSG "Undefined InputDevice \"%s\" referenced by ServerLayout \"%s\"."
#define NO_IDENT_MSG "This section must have an Identifier line."
#define ONLY_ONE_MSG "This section must have only one of either %s line."
#define UNDEFINED_INPUTDRIVER_MSG "InputDevice section \"%s\" must have a Driver line."
#define INVALID_GAMMA_MSG "gamma correction value(s) expected\n either one value or three r/g/b values."
#define GROUP_MSG "The Group keyword must be followed by either a group name in quotes or\n" "\ta numerical group id."
#define MULTIPLE_MSG "Multiple \"%s\" lines."
#define MUST_BE_OCTAL_MSG "The number \"%d\" given in this section must be in octal (0xxx) format."
#define GPU_DEVICE_TOO_MANY "More than %d GPU devices defined."
#define CLOCKS_TOO_MANY "More than %d Clocks defined."
