/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XI_HANDLERS_H
#define _XSERVER_XI_HANDLERS_H

#include "include/dix.h"

int ProcXAllowDeviceEvents(ClientPtr client);
int ProcXChangeDeviceControl(ClientPtr client);
int ProcXChangeDeviceDontPropagateList(ClientPtr client);
int ProcXChangeDeviceKeyMapping(ClientPtr client);
int ProcXChangeDeviceProperty(ClientPtr client);
int ProcXChangeFeedbackControl(ClientPtr client);
int ProcXChangeKeyboardDevice(ClientPtr client);
int ProcXChangePointerDevice(ClientPtr client);
int ProcXCloseDevice(ClientPtr client);
int ProcXDeleteDeviceProperty(ClientPtr client);
int ProcXDeviceBell(ClientPtr client);
int ProcXGetDeviceButtonMapping(ClientPtr client);
int ProcXGetDeviceControl(ClientPtr client);
int ProcXGetDeviceDontPropagateList(ClientPtr client);
int ProcXGetDeviceFocus(ClientPtr client);
int ProcXGetDeviceKeyMapping(ClientPtr client);
int ProcXGetDeviceModifierMapping(ClientPtr client);
int ProcXGetDeviceMotionEvents(ClientPtr client);
int ProcXGetDeviceProperty(ClientPtr client);
int ProcXGetExtensionVersion(ClientPtr client);
int ProcXGetFeedbackControl(ClientPtr client);
int ProcXGetSelectedExtensionEvents(ClientPtr client);
int ProcXGrabDeviceButton(ClientPtr client);
int ProcXGrabDevice(ClientPtr client);
int ProcXGrabDeviceKey(ClientPtr client);
int ProcXIAllowEvents(ClientPtr client);
int ProcXIBarrierReleasePointer(ClientPtr client);
int ProcXIChangeCursor(ClientPtr client);
int ProcXIChangeHierarchy(ClientPtr client);
int ProcXIChangeProperty(ClientPtr client);
int ProcXIDeleteProperty(ClientPtr client);
int ProcXIGetClientPointer(ClientPtr client);
int ProcXIGetFocus(ClientPtr client);
int ProcXIGetProperty(ClientPtr client);
int ProcXIGetSelectedEvents(ClientPtr client);
int ProcXIGrabDevice(ClientPtr client);
int ProcXIListProperties(ClientPtr client);
int ProcXIPassiveGrabDevice(ClientPtr client);
int ProcXIPassiveUngrabDevice(ClientPtr client);
int ProcXIQueryDevice(ClientPtr client);
int ProcXIQueryPointer(ClientPtr client);
int ProcXIQueryVersion(ClientPtr client);
int ProcXISelectEvents(ClientPtr client);
int ProcXISetClientPointer(ClientPtr client);
int ProcXISetFocus(ClientPtr client);
int ProcXIUngrabDevice(ClientPtr client);
int ProcXIWarpPointer(ClientPtr client);
int ProcXListDeviceProperties(ClientPtr client);
int ProcXListInputDevices(ClientPtr client);
int ProcXOpenDevice(ClientPtr client);
int ProcXQueryDeviceState(ClientPtr client);
int ProcXSelectExtensionEvent(ClientPtr client);
int ProcXSendExtensionEvent(ClientPtr client);
int ProcXSetDeviceButtonMapping(ClientPtr client);
int ProcXSetDeviceFocus(ClientPtr client);
int ProcXSetDeviceMode(ClientPtr client);
int ProcXSetDeviceModifierMapping(ClientPtr client);
int ProcXSetDeviceValuators(ClientPtr client);
int ProcXUngrabDeviceButton(ClientPtr client);
int ProcXUngrabDevice(ClientPtr client);
int ProcXUngrabDeviceKey(ClientPtr client);

int SProcXGetDeviceDontPropagateList(ClientPtr  client);
int SProcXGetDeviceMotionEvents(ClientPtr client);
int SProcXGetExtensionVersion(ClientPtr client);
int SProcXGetSelectedExtensionEvents(ClientPtr client);
int SProcXGrabDeviceButton(ClientPtr client);
int SProcXIAllowEvents(ClientPtr client);
int SProcXIBarrierReleasePointer(ClientPtr client);
int SProcXIGetClientPointer(ClientPtr client);
int SProcXIGetSelectedEvents(ClientPtr client);
int SProcXIPassiveGrabDevice(ClientPtr client);
int SProcXIPassiveUngrabDevice(ClientPtr client);
int SProcXIQueryPointer(ClientPtr client);
int SProcXIQueryVersion(ClientPtr client);
int SProcXISelectEvents(ClientPtr client);
int SProcXISetClientPointer(ClientPtr client);
int SProcXIWarpPointer(ClientPtr client);
int SProcXSetDeviceFocus(ClientPtr client);
int SProcXUngrabDeviceButton(ClientPtr client);
int SProcXUngrabDeviceKey(ClientPtr client);

#endif /* _XSERVER_XI_HANDLERS_H */
