#include "pch.h"
#include "WinUHidPS4.h"

#include <algorithm>

#include <wrl/wrappers/corewrappers.h>
using namespace Microsoft::WRL;

//
// This device emulates a wired Sony DualShock 4 gamepad
//

const BYTE k_PS4ReportDescriptor[] =
{
	0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
	0x09, 0x05,        // Usage (Game Pad)
	0xA1, 0x01,        // Collection (Application)
	0x85, 0x01,        //   Report ID (1)
	0x09, 0x30,        //   Usage (X)
	0x09, 0x31,        //   Usage (Y)
	0x09, 0x32,        //   Usage (Z)
	0x09, 0x35,        //   Usage (Rz)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x04,        //   Report Count (4)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0x39,        //   Usage (Hat switch)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x07,        //   Logical Maximum (7)
	0x35, 0x00,        //   Physical Minimum (0)
	0x46, 0x3B, 0x01,  //   Physical Maximum (315)
	0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
	0x75, 0x04,        //   Report Size (4)
	0x95, 0x01,        //   Report Count (1)
	0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
	0x65, 0x00,        //   Unit (None)
	0x05, 0x09,        //   Usage Page (Button)
	0x19, 0x01,        //   Usage Minimum (0x01)
	0x29, 0x0E,        //   Usage Maximum (0x0E)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x01,        //   Logical Maximum (1)
	0x75, 0x01,        //   Report Size (1)
	0x95, 0x0E,        //   Report Count (14)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
	0x09, 0x20,        //   Usage (0x20)
	0x75, 0x06,        //   Report Size (6)
	0x95, 0x01,        //   Report Count (1)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x7F,        //   Logical Maximum (127)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
	0x09, 0x33,        //   Usage (Rx)
	0x09, 0x34,        //   Usage (Ry)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x02,        //   Report Count (2)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
	0x09, 0x21,        //   Usage (0x21)
	0x95, 0x36,        //   Report Count (54)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x85, 0x05,        //   Report ID (5)
	0x09, 0x22,        //   Usage (0x22)
	0x95, 0x1F,        //   Report Count (31)
	0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x04,        //   Report ID (4)
	0x09, 0x23,        //   Usage (0x23)
	0x95, 0x24,        //   Report Count (36)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x02,        //   Report ID (2)
	0x09, 0x24,        //   Usage (0x24)
	0x95, 0x24,        //   Report Count (36)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x08,        //   Report ID (8)
	0x09, 0x25,        //   Usage (0x25)
	0x95, 0x03,        //   Report Count (3)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x10,        //   Report ID (16)
	0x09, 0x26,        //   Usage (0x26)
	0x95, 0x04,        //   Report Count (4)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x11,        //   Report ID (17)
	0x09, 0x27,        //   Usage (0x27)
	0x95, 0x02,        //   Report Count (2)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x12,        //   Report ID (18)
	0x06, 0x02, 0xFF,  //   Usage Page (Vendor Defined 0xFF02)
	0x09, 0x21,        //   Usage (0x21)
	0x95, 0x0F,        //   Report Count (15)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x13,        //   Report ID (19)
	0x09, 0x22,        //   Usage (0x22)
	0x95, 0x16,        //   Report Count (22)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x14,        //   Report ID (20)
	0x06, 0x05, 0xFF,  //   Usage Page (Vendor Defined 0xFF05)
	0x09, 0x20,        //   Usage (0x20)
	0x95, 0x10,        //   Report Count (16)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x15,        //   Report ID (21)
	0x09, 0x21,        //   Usage (0x21)
	0x95, 0x2C,        //   Report Count (44)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x06, 0x80, 0xFF,  //   Usage Page (Vendor Defined 0xFF80)
	0x85, 0x80,        //   Report ID (128)
	0x09, 0x20,        //   Usage (0x20)
	0x95, 0x06,        //   Report Count (6)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x81,        //   Report ID (129)
	0x09, 0x21,        //   Usage (0x21)
	0x95, 0x06,        //   Report Count (6)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x82,        //   Report ID (130)
	0x09, 0x22,        //   Usage (0x22)
	0x95, 0x05,        //   Report Count (5)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x83,        //   Report ID (131)
	0x09, 0x23,        //   Usage (0x23)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x84,        //   Report ID (132)
	0x09, 0x24,        //   Usage (0x24)
	0x95, 0x04,        //   Report Count (4)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x85,        //   Report ID (133)
	0x09, 0x25,        //   Usage (0x25)
	0x95, 0x06,        //   Report Count (6)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x86,        //   Report ID (134)
	0x09, 0x26,        //   Usage (0x26)
	0x95, 0x06,        //   Report Count (6)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x87,        //   Report ID (135)
	0x09, 0x27,        //   Usage (0x27)
	0x95, 0x23,        //   Report Count (35)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x88,        //   Report ID (136)
	0x09, 0x28,        //   Usage (0x28)
	0x95, 0x22,        //   Report Count (34)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x89,        //   Report ID (137)
	0x09, 0x29,        //   Usage (0x29)
	0x95, 0x02,        //   Report Count (2)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x90,        //   Report ID (144)
	0x09, 0x30,        //   Usage (0x30)
	0x95, 0x05,        //   Report Count (5)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x91,        //   Report ID (145)
	0x09, 0x31,        //   Usage (0x31)
	0x95, 0x03,        //   Report Count (3)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x92,        //   Report ID (146)
	0x09, 0x32,        //   Usage (0x32)
	0x95, 0x03,        //   Report Count (3)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0x93,        //   Report ID (147)
	0x09, 0x33,        //   Usage (0x33)
	0x95, 0x0C,        //   Report Count (12)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA0,        //   Report ID (160)
	0x09, 0x40,        //   Usage (0x40)
	0x95, 0x06,        //   Report Count (6)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA1,        //   Report ID (161)
	0x09, 0x41,        //   Usage (0x41)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA2,        //   Report ID (162)
	0x09, 0x42,        //   Usage (0x42)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA3,        //   Report ID (163)
	0x09, 0x43,        //   Usage (0x43)
	0x95, 0x30,        //   Report Count (48)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA4,        //   Report ID (164)
	0x09, 0x44,        //   Usage (0x44)
	0x95, 0x0D,        //   Report Count (13)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA5,        //   Report ID (165)
	0x09, 0x45,        //   Usage (0x45)
	0x95, 0x15,        //   Report Count (21)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA6,        //   Report ID (166)
	0x09, 0x46,        //   Usage (0x46)
	0x95, 0x15,        //   Report Count (21)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xF0,        //   Report ID (240)
	0x09, 0x47,        //   Usage (0x47)
	0x95, 0x3F,        //   Report Count (63)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xF1,        //   Report ID (241)
	0x09, 0x48,        //   Usage (0x48)
	0x95, 0x3F,        //   Report Count (63)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xF2,        //   Report ID (242)
	0x09, 0x49,        //   Usage (0x49)
	0x95, 0x0F,        //   Report Count (15)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA7,        //   Report ID (167)
	0x09, 0x4A,        //   Usage (0x4A)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA8,        //   Report ID (168)
	0x09, 0x4B,        //   Usage (0x4B)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xA9,        //   Report ID (169)
	0x09, 0x4C,        //   Usage (0x4C)
	0x95, 0x08,        //   Report Count (8)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xAA,        //   Report ID (170)
	0x09, 0x4E,        //   Usage (0x4E)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xAB,        //   Report ID (171)
	0x09, 0x4F,        //   Usage (0x4F)
	0x95, 0x39,        //   Report Count (57)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xAC,        //   Report ID (172)
	0x09, 0x50,        //   Usage (0x50)
	0x95, 0x39,        //   Report Count (57)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xAD,        //   Report ID (173)
	0x09, 0x51,        //   Usage (0x51)
	0x95, 0x0B,        //   Report Count (11)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xAE,        //   Report ID (174)
	0x09, 0x52,        //   Usage (0x52)
	0x95, 0x01,        //   Report Count (1)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xAF,        //   Report ID (175)
	0x09, 0x53,        //   Usage (0x53)
	0x95, 0x02,        //   Report Count (2)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0x85, 0xB0,        //   Report ID (176)
	0x09, 0x54,        //   Usage (0x54)
	0x95, 0x3F,        //   Report Count (63)
	0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0xC0,              // End Collection
};

const WINUHID_DEVICE_CONFIG k_PS4Config =
{
	(WINUHID_EVENT_TYPE)(WINUHID_EVENT_READ_REPORT | WINUHID_EVENT_WRITE_REPORT | WINUHID_EVENT_GET_FEATURE | WINUHID_EVENT_SET_FEATURE),
	0x054c, // Sony
	0x05c4, // DualShock 4
	0,
	sizeof(k_PS4ReportDescriptor),
	k_PS4ReportDescriptor,
	{},
	NULL,
	NULL,
	100000, // 100 ms input resend interval to prevent sensor timestamp overflow
};

#include <pshpack1.h>

typedef struct PS4_OUTPUT_REPORT {
	UCHAR RumbleValid : 1;
	UCHAR LedValid : 1;
	UCHAR BlinkValid : 1;
	UCHAR Reserved : 6;
	UCHAR Reserved2;

	UCHAR RumbleRight;
	UCHAR RumbleLeft;

	UCHAR LedRed;
	UCHAR LedGreen;
	UCHAR LedBlue;

	UCHAR BlinkOn;
	UCHAR BlinkOff;
} PS4_OUTPUT_REPORT, *PPS4_OUTPUT_REPORT;

#include <poppack.h>

typedef struct _PS4_LED_STATE {
	UCHAR LedRed;
	UCHAR LedGreen;
	UCHAR LedBlue;

	UCHAR BlinkOn;
	UCHAR BlinkOff;
} PS4_LED_STATE, *PPS4_LED_STATE;
#define PS4_BLINK_TIME_TO_MILLIS(x) ((x) * 10)

typedef struct _WINUHID_PS4_GAMEPAD {
	PWINUHID_DEVICE Device;
	BOOL Stopping;
	SRWLOCK Lock;

	PWINUHID_PS4_FF_CB RumbleCallback;
	PWINUHID_PS4_LED_CB LedCallback;
	PVOID CallbackContext;

	HANDLE LedThread;
	HANDLE LedUpdatedEvent;
	PS4_LED_STATE LedState;

	LARGE_INTEGER QpcFrequency;
	LARGE_INTEGER LastInputReportTime;
	WINUHID_PS4_INPUT_REPORT LastInputReport;
	USHORT Timestamp;

	UCHAR MacAddress[6];
} WINUHID_PS4_GAMEPAD, *PWINUHID_PS4_GAMEPAD;

#define DS4_FEATURE_REPORT_CALIBRATION		0x02
#define DS4_FEATURE_REPORT_FIRMWARE_INFO	0xa3
#define DS4_FEATURE_REPORT_PAIRING_INFO		0x12

#define DS4_OUTPUT_REPORT_EFFECTS           0x05

void PrepareInputReportForSubmission(PWINUHID_PS4_GAMEPAD Gamepad)
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	//
	// Compute the time between reports to determine the timestamp increment
	//
	LARGE_INTEGER deltaNs;
	deltaNs.QuadPart = now.QuadPart - Gamepad->LastInputReportTime.QuadPart;
	deltaNs.QuadPart *= 1000000000ULL;
	deltaNs.QuadPart /= Gamepad->QpcFrequency.QuadPart;
	Gamepad->Timestamp += (USHORT)(deltaNs.QuadPart / 5333); // Timestamp is in 5.333us units
	Gamepad->LastInputReportTime = now;

	//
	// Send the input report with the updated timestamp
	//
	Gamepad->LastInputReport.Timestamp = Gamepad->Timestamp;
}

VOID WinUHidPS4Callback(PVOID CallbackContext, PWINUHID_DEVICE Device, PCWINUHID_EVENT Event)
{
	auto gamepad = (PWINUHID_PS4_GAMEPAD)CallbackContext;

	if (Event->Type == WINUHID_EVENT_GET_FEATURE) {
		//
		// http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
		// https://github.com/nefarius/ViGEmBus/blob/master/sys/Ds4Pdo.cpp
		//
		switch (Event->ReportId)
		{
		case DS4_FEATURE_REPORT_CALIBRATION:
		{
			//
			// This neutral calibration is from inputino
			//
			static const UCHAR data[] =
			{
				0x02,
				0x00, 0x00, // gyro_pitch_bias
				0x00, 0x00, // gyro_yaw_bias
				0x00, 0x00, // gyro_roll_bias
				0x10, 0x27, // gyro_pitch_plus
				0xF0, 0xD8, // gyro_pitch_minus
				0x10, 0x27, // gyro_yaw_plus
				0xF0, 0xD8,  // gyro_yaw_minus
				0x10, 0x27, // gyro_roll_plus
				0xF0, 0xD8, // gyro_roll_minus
				0xF4, 0x01, // gyro_speed_plus
				0xF4, 0x01, // gyro_speed_minus
				0x10, 0x27, // acc_x_plus
				0xF0, 0xD8, // acc_x_minus
				0x10, 0x27, // acc_y_plus
				0xF0, 0xD8, // acc_y_minus
				0x10, 0x27, // acc_z_plus
				0xF0, 0xD8, // acc_z_minus
			};

			WinUHidCompleteReadEvent(Device, Event, &data, sizeof(data));
			break;
		}

		case DS4_FEATURE_REPORT_FIRMWARE_INFO:
		{
			static const UCHAR data[] =
			{
				0xA3, 0x41, 0x75, 0x67, 0x20, 0x20, 0x33, 0x20,
				0x32, 0x30, 0x31, 0x33, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x30, 0x37, 0x3A, 0x30, 0x31, 0x3A, 0x31,
				0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x01, 0x00, 0x31, 0x03, 0x00, 0x00,
				0x00, 0x49, 0x00, 0x05, 0x00, 0x00, 0x80, 0x03,
				0x00
			};

			WinUHidCompleteReadEvent(Device, Event, data, sizeof(data));
			break;
		}

		case DS4_FEATURE_REPORT_PAIRING_INFO:
		{
			UCHAR data[] =
			{
				0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};

			//
			// The MAC address is reported in reverse order
			//
			for (int i = 0; i < sizeof(gamepad->MacAddress); i++) {
				data[i + 1] = gamepad->MacAddress[sizeof(gamepad->MacAddress) - (i + 1)];
			}

			WinUHidCompleteReadEvent(Device, Event, data, sizeof(data));
			break;
		}

		default:
			//
			// Fail other feature reads that we don't implement
			//
			WinUHidCompleteReadEvent(Device, Event, NULL, 0);
			break;
		}
	}
	else if (Event->Type == WINUHID_EVENT_SET_FEATURE) {
		//
		// Just succeed all set feature events
		//
		WinUHidCompleteWriteEvent(Device, Event, TRUE);
	}
	else if (Event->Type == WINUHID_EVENT_READ_REPORT) {
		//
		// Resubmit the latest input report with updated timestamps
		//
		AcquireSRWLockExclusive(&gamepad->Lock);
		PrepareInputReportForSubmission(gamepad);
		ReleaseSRWLockExclusive(&gamepad->Lock);
		WinUHidCompleteReadEvent(Device, Event, &gamepad->LastInputReport, sizeof(gamepad->LastInputReport));
	}
	else {
		//
		// There's only one defined output report
		//
		if (Event->ReportId != DS4_OUTPUT_REPORT_EFFECTS || Event->Write.DataLength < 1 + sizeof(PS4_OUTPUT_REPORT)) {
			WinUHidCompleteWriteEvent(Device, Event, FALSE);
			return;
		}

		auto outputReport = (PPS4_OUTPUT_REPORT)&Event->Write.Data[1];

		//
		// If we have an LED thread and new LED state, pass the data over to it
		//
		if (gamepad->LedThread && (outputReport->LedValid || outputReport->BlinkValid)) {
			AcquireSRWLockExclusive(&gamepad->Lock);
			if (outputReport->LedValid) {
				gamepad->LedState.LedRed = outputReport->LedRed;
				gamepad->LedState.LedGreen = outputReport->LedGreen;
				gamepad->LedState.LedBlue = outputReport->LedBlue;
			}
			if (outputReport->BlinkValid) {
				gamepad->LedState.BlinkOn = outputReport->BlinkOn;
				gamepad->LedState.BlinkOff = outputReport->BlinkOff;
			}
			ReleaseSRWLockExclusive(&gamepad->Lock);
			SetEvent(gamepad->LedUpdatedEvent);
		}

		//
		// If we have new motor state, pass the data to the callback
		//
		if (outputReport->RumbleValid && gamepad->RumbleCallback) {
			gamepad->RumbleCallback(gamepad->CallbackContext, outputReport->RumbleLeft, outputReport->RumbleRight);
		}

		WinUHidCompleteWriteEvent(Device, Event, TRUE);
	}
}

DWORD WINAPI PS4LedThreadProc(LPVOID lpParameter)
{
	auto device = (PWINUHID_PS4_GAMEPAD)lpParameter;

	while (!device->Stopping) {
		//
		// Wait for a new LED state change
		//
		DWORD result = WaitForSingleObject(device->LedUpdatedEvent, INFINITE);
		if (device->Stopping) {
			break;
		}

		//
		// Load the most recent LED state
		//
		AcquireSRWLockShared(&device->Lock);
		PS4_LED_STATE ledState = device->LedState;
		ResetEvent(device->LedUpdatedEvent);
		ReleaseSRWLockShared(&device->Lock);

		//
		// If this is not a request to blink, turn on the LEDs and wait for the next request
		//
		if (ledState.BlinkOn == 0 && ledState.BlinkOff == 0) {
			device->LedCallback(device->CallbackContext, ledState.LedRed, ledState.LedGreen, ledState.LedBlue);
			continue;
		}

		//
		// Blink until we get another LED request
		//
		for (;;) {
			//
			// Turn the LEDs on
			//
			device->LedCallback(device->CallbackContext, ledState.LedRed, ledState.LedGreen, ledState.LedBlue);

			//
			// Wait for the blink on time
			//
			result = WaitForSingleObject(device->LedUpdatedEvent, PS4_BLINK_TIME_TO_MILLIS(ledState.BlinkOn));
			if (result == WAIT_OBJECT_0 || device->Stopping) {
				break;
			}

			//
			// Turn the LEDs off
			//
			device->LedCallback(device->CallbackContext, 0, 0, 0);

			//
			// Wait for the blink off time
			//
			result = WaitForSingleObject(device->LedUpdatedEvent, PS4_BLINK_TIME_TO_MILLIS(ledState.BlinkOff));
			if (result == WAIT_OBJECT_0 || device->Stopping) {
				break;
			}
		}
	}

	return GetLastError();
}

WINUHID_API PWINUHID_PS4_GAMEPAD WinUHidPS4Create(PCWINUHID_PS4_GAMEPAD_INFO Info, PWINUHID_PS4_FF_CB RumbleCallback, PWINUHID_PS4_LED_CB LedCallback, PVOID CallbackContext)
{
	WINUHID_DEVICE_CONFIG config = k_PS4Config;
	PopulateDeviceInfo(&config, Info ? Info->BasicInfo : NULL);

	if (config.VendorID == 0) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	PWINUHID_PS4_GAMEPAD gamepad = (PWINUHID_PS4_GAMEPAD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*gamepad));
	if (!gamepad) {
		SetLastError(ERROR_OUTOFMEMORY);
		return NULL;
	}

	QueryPerformanceFrequency(&gamepad->QpcFrequency);
	QueryPerformanceCounter(&gamepad->LastInputReportTime);

	InitializeSRWLock(&gamepad->Lock);
	gamepad->RumbleCallback = RumbleCallback;
	gamepad->LedCallback = LedCallback;
	gamepad->CallbackContext = CallbackContext;

	if (Info) {
		RtlCopyMemory(&gamepad->MacAddress[0], &Info->MacAddress[0], sizeof(gamepad->MacAddress));
	}

	WinUHidPS4InitializeInputReport(&gamepad->LastInputReport);

	//
	// If the caller wants LED callbacks, start the LED thread
	//
	if (gamepad->LedCallback) {
		gamepad->LedUpdatedEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
		if (!gamepad->LedUpdatedEvent) {
			WinUHidPS4Destroy(gamepad);
			return NULL;
		}

		gamepad->LedThread = CreateThread(NULL, 0, PS4LedThreadProc, gamepad, 0, NULL);
		if (!gamepad->LedThread) {
			WinUHidPS4Destroy(gamepad);
			return NULL;
		}

		SetThreadDescription(gamepad->LedThread, L"WinUHidDev PS4 LED Effects Thread");
	}

	gamepad->Device = WinUHidCreateDevice(&config);
	if (!gamepad->Device) {
		WinUHidPS4Destroy(gamepad);
		return NULL;
	}

	if (!WinUHidStartDevice(gamepad->Device, WinUHidPS4Callback, gamepad)) {
		WinUHidPS4Destroy(gamepad);
		return NULL;
	}

	//
	// Send an neutral input report
	//
	WINUHID_PS4_INPUT_REPORT inputReport;
	WinUHidPS4InitializeInputReport(&inputReport);
	if (!WinUHidPS4ReportInput(gamepad, &inputReport)) {
		WinUHidPS4Destroy(gamepad);
		return NULL;
	}

	return gamepad;
}

WINUHID_API BOOL WinUHidPS4ReportInput(PWINUHID_PS4_GAMEPAD Gamepad, PCWINUHID_PS4_INPUT_REPORT Report)
{
	AcquireSRWLockExclusive(&Gamepad->Lock);
	Gamepad->LastInputReport = *Report;
	PrepareInputReportForSubmission(Gamepad);
	ReleaseSRWLockExclusive(&Gamepad->Lock);

	BOOL ret = WinUHidSubmitInputReport(Gamepad->Device, &Gamepad->LastInputReport, sizeof(Gamepad->LastInputReport));

	//
	// Since we handle WINUHID_EVENT_READ_REPORT, WinUHidSubmitInputReport may fail with ERROR_NOT_READY
	// if a HID client hasn't asked for another report. This is fine because our callback function will
	// report this input once the caller is ready for it.
	//
	return ret || GetLastError() == ERROR_NOT_READY;
}

WINUHID_API VOID WinUHidPS4InitializeInputReport(PWINUHID_PS4_INPUT_REPORT Report)
{
	RtlZeroMemory(Report, sizeof(*Report));

	Report->ReportId = 0x01;
	Report->LeftStickX = 0x80;
	Report->LeftStickY = 0x80;
	Report->RightStickX = 0x80;
	Report->RightStickY = 0x80;
	Report->Hat = 0x8;

	WinUHidPS4SetTouchState(Report, 0, FALSE, 0, 0);
	WinUHidPS4SetTouchState(Report, 1, FALSE, 0, 0);

	WinUHidPS4SetBatteryState(Report, TRUE, 100);

	//
	// Initialize accelerometer in neutral upright position
	//
	WinUHidPS4SetAccelState(Report, 0, 9.80665f, 0);
}

WINUHID_API VOID WinUHidPS4SetTouchState(PWINUHID_PS4_INPUT_REPORT Report, UCHAR TouchIndex, BOOL TouchDown, USHORT TouchX, USHORT TouchY)
{
	Report->TouchReportCount = 1;
	if (TouchDown) {
		Report->TouchReports[0].TouchPoints[TouchIndex].ContactSeq++;
		Report->TouchReports[0].TouchPoints[TouchIndex].ContactSeq &= ~0x80;
	}
	else {
		Report->TouchReports[0].TouchPoints[TouchIndex].ContactSeq |= 0x80;
	}
	Report->TouchReports[0].TouchPoints[TouchIndex].XLowPart = TouchX & 0xFF;
	Report->TouchReports[0].TouchPoints[TouchIndex].XHighPart = (TouchX >> 8) & 0xF;
	Report->TouchReports[0].TouchPoints[TouchIndex].YLowPart = TouchY & 0xF;
	Report->TouchReports[0].TouchPoints[TouchIndex].YHighPart = (TouchY >> 4) & 0xFF;
	Report->TouchReports[0].Timestamp++;
}

WINUHID_API VOID WinUHidPS4SetAccelState(PWINUHID_PS4_INPUT_REPORT Report, float AccelX, float AccelY, float AccelZ)
{
	static const float k_AccelSensitivity = 0.000980664976f;

	Report->AccelX = std::clamp((int)(AccelX / k_AccelSensitivity), SHRT_MIN, SHRT_MAX);
	Report->AccelY = std::clamp((int)(AccelY / k_AccelSensitivity), SHRT_MIN, SHRT_MAX);
	Report->AccelZ = std::clamp((int)(AccelZ / k_AccelSensitivity), SHRT_MIN, SHRT_MAX);
}

WINUHID_API VOID WinUHidPS4SetGyroState(PWINUHID_PS4_INPUT_REPORT Report, float GyroX, float GyroY, float GyroZ)
{
	static const float k_GyroSensitivity = 0.000872664619f;

	Report->GyroX = std::clamp((int)(GyroX / k_GyroSensitivity), SHRT_MIN, SHRT_MAX);
	Report->GyroY = std::clamp((int)(GyroY / k_GyroSensitivity), SHRT_MIN, SHRT_MAX);
	Report->GyroZ = std::clamp((int)(GyroZ / k_GyroSensitivity), SHRT_MIN, SHRT_MAX);
}

WINUHID_API VOID WinUHidPS4SetBatteryState(PWINUHID_PS4_INPUT_REPORT Report, BOOL Wired, UCHAR Percentage)
{
	if (Percentage == 100 && Wired) {
		Report->BatteryLevel = 0xFF;
		Report->BatteryLevelSpecial = 0x1B;
	}
	else {
		Report->BatteryLevel = Percentage * 255U / 100U;
		Report->BatteryLevelSpecial = Percentage / 10;

		if (Wired) {
			Report->BatteryLevelSpecial |= 0x10;
		}
		else {
			Report->BatteryLevelSpecial &= ~0x10;
		}
	}
}

WINUHID_API VOID WinUHidPS4SetHatState(PWINUHID_PS4_INPUT_REPORT Report, INT HatX, INT HatY)
{
	if (HatX == 0 && HatY == 0) {
		Report->Hat = 0x8; // Neutral
	}
	else if (HatX < 0 && HatY < 0) {
		Report->Hat = 0x7; // Top-left
	}
	else if (HatX < 0 && HatY == 0) {
		Report->Hat = 0x6; // Left
	}
	else if (HatX < 0 && HatY > 0) {
		Report->Hat = 0x5; // Bottom-left
	}
	else if (HatX == 0 && HatY > 0) {
		Report->Hat = 0x4; // Bottom
	}
	else if (HatX > 0 && HatY > 0) {
		Report->Hat = 0x3; // Bottom-right
	}
	else if (HatX > 0 && HatY == 0) {
		Report->Hat = 0x2; // Right
	}
	else if (HatX > 0 && HatY < 0) {
		Report->Hat = 0x1; // Top-right
	}
	else {
		Report->Hat = 0x0; // Top
	}
}

WINUHID_API VOID WinUHidPS4Destroy(PWINUHID_PS4_GAMEPAD Gamepad)
{
	if (!Gamepad) {
		return;
	}

	Gamepad->Stopping = TRUE;

	if (Gamepad->LedThread) {
		SetEvent(Gamepad->LedUpdatedEvent);
		WaitForSingleObject(Gamepad->LedThread, INFINITE);
		CloseHandle(Gamepad->LedThread);
	}

	if (Gamepad->LedUpdatedEvent) {
		CloseHandle(Gamepad->LedUpdatedEvent);
	}

	if (Gamepad->Device) {
		WinUHidDestroyDevice(Gamepad->Device);
	}

	HeapFree(GetProcessHeap(), 0, Gamepad);
}