#pragma once

#include "WinUHidDevs.h"

typedef struct _WINUHID_MOUSE_DEVICE *PWINUHID_MOUSE_DEVICE;

//
// Creates a new mouse. Device info can optionally be provided to override a subset
// of identification information of the created device.
//
// To destroy the device, call WinUHidMouseDestroy().
//
// On failure, the function will return NULL. Call GetLastError() to the error code.
//
WINUHID_API PWINUHID_MOUSE_DEVICE WinUHidMouseCreate(PCWINUHID_PRESET_DEVICE_INFO Info);

//
// Submits a pointer motion report to the device.
//
// On failure, the function will return FALSE. Call GetLastError() to the error code.
//
WINUHID_API BOOL WinUHidMouseReportMotion(PWINUHID_MOUSE_DEVICE Mouse, SHORT DeltaX, SHORT DeltaY);

//
// Submits a pointer button report to the device.
//
// On failure, the function will return FALSE. Call GetLastError() to the error code.
//
#define WUHM_BUTTON_LEFT   0x01
#define WUHM_BUTTON_RIGHT  0x02
#define WUHM_BUTTON_MIDDLE 0x03
#define WUHM_BUTTON_X1     0x04
#define WUHM_BUTTON_X2     0x05
WINUHID_API BOOL WinUHidMouseReportButton(PWINUHID_MOUSE_DEVICE Mouse, UCHAR ButtonIndex, BOOL Down);

//
// Submits a mouse wheel report to the device.
//
// NOTE: The scroll value should be reported in units of 1/120th of a whole detent.
//
// On failure, the function will return FALSE. Call GetLastError() to the error code.
//
WINUHID_API BOOL WinUHidMouseReportScroll(PWINUHID_MOUSE_DEVICE Mouse, SHORT ScrollValue, BOOL Horizontal);

//
// Destroys the mouse device.
//
// This function never fails as long as the provided argument is valid.
//
WINUHID_API VOID WinUHidMouseDestroy(PWINUHID_MOUSE_DEVICE Mouse);