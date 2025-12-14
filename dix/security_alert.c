#include "security_alert_priv.h"
#include "dix.h"
#include "os/osdep.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_WHITELIST_ENTRIES 1024

typedef struct {
    pid_t client_pid;
    pid_t target_pid;
} WhitelistEntry;

static WhitelistEntry whitelist[MAX_WHITELIST_ENTRIES];
static int whitelist_count = 0;

#define WHITELIST_FILE "/etc/X11/whitelist_actions.conf"

Bool IsWhitelisted(const char *procname, int type)
{
    FILE *f = fopen(WHITELIST_FILE, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            char *last_space = strrchr(line, ' ');
            if (last_space) {
                *last_space = '\0';
                int w_type;
                if (sscanf(last_space + 1, "%d", &w_type) == 1) {
                    if (w_type == type && strcmp(line, procname) == 0) {
                        fclose(f);
                        return TRUE;
                    }
                }
            }
        }
        fclose(f);
    }
    return FALSE;
}

void AddToWhitelist(const char *procname, int type)
{
    FILE *f = fopen(WHITELIST_FILE, "a");
    if (f) {
        fprintf(f, "%s %d\n", procname, type);
        fclose(f);
    }
}

void RemoveFromWhitelist(pid_t pid)
{
    int i = 0;
    while (i < whitelist_count) {
        if (whitelist[i].client_pid == pid || whitelist[i].target_pid == pid) {
            whitelist[i] = whitelist[whitelist_count - 1];
            whitelist_count--;
        } else {
            i++;
        }
    }
}

void GetProcessName(pid_t pid, char *buffer, size_t size)
{
    char command[256];
    FILE *fp;

    snprintf(command, sizeof(command), "ps -p %d -o comm=", pid);
    fp = popen(command, "r");
    if (fp) {
        if (fgets(buffer, size, fp)) {
            // Remove trailing newline
            buffer[strcspn(buffer, "\n")] = 0;
        } else {
            strncpy(buffer, "unknown", size);
        }
        pclose(fp);
    } else {
        strncpy(buffer, "unknown", size);
    }
}


void
sanitize_string(char *str)
{
    if (!str) return;
    char *p = str;
    while (*p) {
        if (strchr(";'|&`()\\\"!<>", *p)) {
            *p = '_';
        }
        p++;
    }
}

static int
command_exists(const char *command)
{
    char *path = getenv("PATH");
    if (!path)
        return 0;

    char *path_copy = strdup(path);
    if (!path_copy)
        return 0;

    int found = 0;
    char *saveptr;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return found;
}

static enum DialogCommand
GetDialogCommand(void)
{
    // env for user and DE select dialog app
    const char *env_dialog = getenv("XLIBRE_SECURITY_ALERT_DIALOG");
    if (env_dialog) {
        if (strcmp(env_dialog, "kdialog") == 0) return DIALOG_CMD_KDIALOG;
        if (strcmp(env_dialog, "zenity") == 0) return DIALOG_CMD_ZENITY;
        if (strcmp(env_dialog, "dialog") == 0) return DIALOG_CMD_DIALOG;
        if (strcmp(env_dialog, "whiptail") == 0) return DIALOG_CMD_WHIPTAIL;
        if (strcmp(env_dialog, "yad") == 0) return DIALOG_CMD_YAD;
    }

    if (command_exists("kdialog")) {
        return DIALOG_CMD_KDIALOG;
    }
    if (command_exists("zenity")) {
        return DIALOG_CMD_ZENITY;
    }
    if (command_exists("dialog")) {
        return DIALOG_CMD_DIALOG;
    }
    if (command_exists("whiptail")) {
        return DIALOG_CMD_WHIPTAIL;
    }
    if (command_exists("yad")) {
        return DIALOG_CMD_YAD;
    }
    return DIALOG_CMD_NONE;
}

// Alert about window image scrapering
int
ShowSecurityAlertDialog(ClientPtr client, pid_t client_pid, const char *client_name, pid_t window_pid, const char *window_name)
{
    enum DialogCommand dialog_cmd = GetDialogCommand();
    if (dialog_cmd == DIALOG_CMD_NONE) {
        /* No dialog tool found, deny access */
        return BadAccess;
    }

    char text[1024];
    char sanitized_client_name[256];
    char sanitized_window_name[256];

    strncpy(sanitized_client_name, client_name, sizeof(sanitized_client_name) - 1);
    sanitized_client_name[sizeof(sanitized_client_name) - 1] = '\0';
    sanitize_string(sanitized_client_name);

    strncpy(sanitized_window_name, window_name, sizeof(sanitized_window_name) - 1);
    sanitized_window_name[sizeof(sanitized_window_name) - 1] = '\0';
    sanitize_string(sanitized_window_name);

    snprintf(text, sizeof(text),
             "Process \\\"%s\\\" (PID: %d) is trying to get an image of a window belonging to process \\\"%s\\\" (PID: %d). This could be screen sharing or a malicious application. Allow this interaction?",
             sanitized_client_name, client_pid, sanitized_window_name, window_pid);

    pid_t pid = fork();
    if (pid == -1) {
        return BadAlloc;
    } else if (pid == 0) { /* child */
        switch (dialog_cmd) {
            case DIALOG_CMD_ZENITY: {
                char *args[] = {"zenity", "--question", "--title=XLibre Security Alert", "--text", text, "--ok-label=Allow", "--cancel-label=Deny", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_DIALOG: {
                char *args[] = {"dialog", "--yesno", text, "10", "70", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_WHIPTAIL: {
                char *args[] = {"whiptail", "--yesno", text, "10", "70", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_YAD: {
                char *args[] = {"yad", "--question", "--title=XLibre Security Alert", "--text", text, "--button=Allow:0", "--button=Deny:1", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_KDIALOG: {
                char *args[] = {"kdialog", "--yesno", text, "--title", "XLibre Security Alert", NULL};
                execvp(args[0], args);
                break;
            }
        default: break;
        }
        _exit(127); /* execvp failed */
    } else { /* parent */
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            AddToWhitelist(sanitized_client_name, window_pid);
            return Success;
        } else {
            return BadAccess;
        }
    }
    return BadAccess; // Should not be reached
}

// Alert about keyboard event listening
int
ShowKeyboardSecurityAlertDialog(ClientPtr client, pid_t client_pid, const char *client_name)
{
    enum DialogCommand dialog_cmd = GetDialogCommand();
    if (dialog_cmd == DIALOG_CMD_NONE) {
        return BadAccess;
    }

    char text[1024];
    char sanitized_client_name[256];

    strncpy(sanitized_client_name, client_name, sizeof(sanitized_client_name) - 1);
    sanitized_client_name[sizeof(sanitized_client_name) - 1] = '\0';
    sanitize_string(sanitized_client_name);

    snprintf(text, sizeof(text),
             "Process \\\"%s\\\" (PID: %d) is attempting to listen to all keyboard events. This is a common behavior for keyloggers. Allow this action?",
             sanitized_client_name, client_pid);

    pid_t pid = fork();
    if (pid == -1) {
        return BadAlloc;
    } else if (pid == 0) { /* child */
        switch (dialog_cmd) {
            case DIALOG_CMD_ZENITY: {
                char *args[] = {"zenity", "--question", "--title=XLibre Security Alert", "--text", text, "--ok-label=Allow", "--cancel-label=Deny", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_DIALOG: {
                char *args[] = {"dialog", "--yesno", text, "10", "70", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_WHIPTAIL: {
                char *args[] = {"whiptail", "--yesno", text, "10", "70", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_YAD: {
                char *args[] = {"yad", "--question", "--title=XLibre Security Alert", "--text", text, "--button=Allow:0", "--button=Deny:1", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_KDIALOG: {
                char *args[] = {"kdialog", "--yesno", text, "--title", "XLibre Security Alert", NULL};
                execvp(args[0], args);
                break;
            }
        default: break;
        }
        _exit(127);
    } else { /* parent */
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            AddToWhitelist(sanitized_client_name, 0); // 0 for keyboard events, not tied to a specific window PID
            return Success;
        } else {
            return BadAccess;
        }
    }
    return BadAccess;
}

// Alert about synthetic event sending
int
ShowSendEventSecurityAlertDialog(ClientPtr client, pid_t client_pid, const char *client_name, unsigned long window_id)
{
    enum DialogCommand dialog_cmd = GetDialogCommand();
    if (dialog_cmd == DIALOG_CMD_NONE) {
        return BadAccess;
    }

    char text[1024];
    char sanitized_client_name[256];

    strncpy(sanitized_client_name, client_name, sizeof(sanitized_client_name) - 1);
    sanitized_client_name[sizeof(sanitized_client_name) - 1] = '\0';
    sanitize_string(sanitized_client_name);

    snprintf(text, sizeof(text),
             "Process \\\"%s\\\" (PID: %d) is attempting to send a synthetic event to window 0x%lx. This may be used to spoof input. Allow this action?",
             sanitized_client_name, client_pid, window_id);

    pid_t pid = fork();
    if (pid == -1) {
        return BadAlloc;
    } else if (pid == 0) { /* child */
        switch (dialog_cmd) {
            case DIALOG_CMD_ZENITY: {
                char *args[] = {"zenity", "--question", "--title=XLibre Security Alert", "--text", text, "--ok-label=Allow", "--cancel-label=Deny", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_DIALOG: {
                char *args[] = {"dialog", "--yesno", text, "10", "70", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_WHIPTAIL: {
                char *args[] = {"whiptail", "--yesno", text, "10", "70", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_YAD: {
                char *args[] = {"yad", "--question", "--title=XLibre Security Alert", "--text", text, "--button=Allow:0", "--button=Deny:1", NULL};
                execvp(args[0], args);
                break;
            }
            case DIALOG_CMD_KDIALOG: {
                char *args[] = {"kdialog", "--yesno", text, "--title", "XLibre Security Alert", NULL};
                execvp(args[0], args);
                break;
            }
        default: break;
        }
        _exit(127);
    } else { /* parent */
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            AddToWhitelist(sanitized_client_name, window_id);
            return Success;
        } else {
            return BadAccess;
        }
    }
    return BadAccess;
}