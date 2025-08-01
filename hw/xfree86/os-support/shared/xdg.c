#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_os_support.h"
#include "xf86_OSlib.h"

/*
 * GetXDGConfigPath: Gets the XDG user config directory
 * Puts $XDG_CONFIG_HOME, fallbacks to $HOME/.config/ into out_path
*/
void GetXDGConfigPath(char* out_path)
{
    struct passwd* pw = getpwuid(getuid());
    if ((!pw) || !pw->pw_dir || pw->pw_dir[0] == '\0') {
        out_path[0] = '\0';
        return;
    }
    char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");

    char tmpPath[PATH_MAX] = {0};
    strncpy(tmpPath, pw->pw_dir, PATH_MAX-1);
    tmpPath[PATH_MAX-1] = '\0';

    char* base_dir = strcat(tmpPath, "/.config/");
    if (XDG_CONFIG_HOME != NULL) {
        strncpy(out_path, XDG_CONFIG_HOME, PATH_MAX-1);
        out_path[PATH_MAX-1] = '\0';
        return;
    }
    strncpy(out_path, base_dir, PATH_MAX-1);
    out_path[PATH_MAX-1] = '\0';
}
