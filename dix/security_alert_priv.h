#ifndef SECURITY_ALERT_H
#define SECURITY_ALERT_H

#include "os/osdep.h"
#include <sys/types.h>

enum DialogCommand {
    DIALOG_CMD_NONE,
    DIALOG_CMD_ZENITY,
    DIALOG_CMD_DIALOG,
    DIALOG_CMD_WHIPTAIL,
    DIALOG_CMD_YAD,
    DIALOG_CMD_KDIALOG
};

Bool IsWhitelisted(const char *procname, int type);
void AddToWhitelist(const char *procname, int type);
void RemoveFromWhitelist(pid_t pid);
void GetProcessName(pid_t pid, char *buffer, size_t size);

void sanitize_string(char *str);

int ShowSecurityAlertDialog(ClientPtr client, pid_t client_pid, const char *client_name, pid_t window_pid, const char *window_name);
int ShowKeyboardSecurityAlertDialog(ClientPtr client, pid_t client_pid, const char *client_name);
int ShowSendEventSecurityAlertDialog(ClientPtr client, pid_t client_pid, const char *client_name, unsigned long window_id);

#endif // SECURITY_ALERT_H
