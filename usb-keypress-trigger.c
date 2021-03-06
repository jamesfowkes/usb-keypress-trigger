/*
 * main.c
 *
 *  usb-keypress-trigger application
 *
 *  Uses Lightweight USB Framework on AVR (LUFA).
 *  Largely based on LUFA keyboard example code
 *  LUFA is distributed under MIT License
 *  	(see LUFA source or http://www.fourwalledcubicle.com/LUFA.php)
 *
 *  Created on: 27 Aug 2012
 *      Author: james
 *
 *		Target: AT90USB162 on Minimus USB V1 board
 */

/* Standard Library Includes */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/* Generic Library Includes */
#include "KeyboardSupport.h"

/* AVR Library Includes */
#include "avr/io.h"
#include "avr/wdt.h"
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

/* LUFA includes */
#include "Descriptors.h"

/* Minimus Includes */
#include <minimus.h>

/*
 * AVR Library Includes
 */

#include "Keyboard.h"

/*
 * Private Defines and Datatypes
 */

#define ARCADE_BUTTON_DDR (DDRB)
#define ARCADE_BUTTON_PIN (1)
#define ARCADE_BUTTON_PORT (PORTB)
#define ARCADE_BUTTON_PINS (PINB)

/*
 * Private Variables
 */

static bool s_hwb_button_pressed = false;
static bool s_keypress_trigger_flag = false;

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
{ .Config =
{
	.InterfaceNumber = 0,
	.ReportINEndpoint =
	{
		.Address = KEYBOARD_EPADDR,
		.Size = KEYBOARD_EPSIZE,
		.Banks = 1,
	},
	.PrevReportINBuffer = PrevKeyboardHIDReportBuffer,
	.PrevReportINBufferSize = sizeof(PrevKeyboardHIDReportBuffer),
}, };

/*
 * Private Functions
 */

static void setup_io()
{
	ARCADE_BUTTON_DDR &= ~_BV(ARCADE_BUTTON_PIN);
	ARCADE_BUTTON_PORT |= _BV(ARCADE_BUTTON_PIN);
}

static bool poll_arcade_button()
{
	static int8_t counter = 0;
	static bool already_pressed = false;

	bool pressed = false;

	counter += (ARCADE_BUTTON_PINS & _BV(ARCADE_BUTTON_PIN)) ? -1 : 1;

	if (counter > 20) { counter = 20; }
	if (counter < 0) { counter = 0; }

	if (counter == 0) { already_pressed = false; }

	if (!already_pressed)
	{
		pressed = counter == 20;
		already_pressed = pressed;
	}

	return pressed;	
}

static void handle_led(bool on)
{
	static uint8_t counter = 0;

	if (on) { counter = 200; }

	Minimus_LED_Control(LED2, counter > 0 ? LED_ON : LED_OFF);
                        
	if (counter > 0) { counter--; }
}

static void handle_buttons()
{
	bool external_button_pressed = poll_arcade_button();
	s_keypress_trigger_flag = external_button_pressed || s_hwb_button_pressed;
}

static void application_tick()
{
	handle_buttons();
	handle_led(s_keypress_trigger_flag);
}

/*
 * Public Functions
 */

void MinimusButtonCallback(MINIMUS_BUTTON_ENUM eButton, MINIMUS_BUTTONSTATE_ENUM eNewState)
{
	(void)eButton;
	s_hwb_button_pressed = eNewState == BUTTONDN;
}

int main(void)
{
	/* Disable watchdog: not required for simple keyboard application */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Allow Minimus to setup the microcontroller for itself */
	Minimus_Init(MinimusButtonCallback);

	setup_io();

	USB_Init();

	/* All processing interrupt based from here*/
	sei();

	while (true)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
		USB_USBTask();
	}

	return 0;
}

/*
 * USB event handlers
 */

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
	Minimus_USB_MsTick();

	application_tick();
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	(void) HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
	USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(
		USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
		uint8_t* const ReportID, const uint8_t ReportType, void* ReportData,
		uint16_t* const ReportSize)
{
	(void)HIDInterfaceInfo;
	(void)ReportID;
	(void)ReportType;

	USB_KeyboardReport_Data_t* KeyboardReport =
			(USB_KeyboardReport_Data_t*) ReportData;

	bool bSendReport = false;

	/* Clear the report contents, then set */
	memset(ReportData, 0, sizeof(USB_KeyboardReport_Data_t));

	if (s_keypress_trigger_flag)
	{
		KeyboardReport->KeyCode[0] = HID_KEYBOARD_SC_SPACE;
		bSendReport = true;
		s_keypress_trigger_flag = false;
	}

	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return bSendReport;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(
		USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
		const uint8_t ReportID, const uint8_t ReportType,
		const void* ReportData, const uint16_t ReportSize)
{
	(void) HIDInterfaceInfo;
	(void) ReportID;
	(void) ReportType;
	(void) ReportData;
	(void) ReportSize;
}

/* Unhandled USB library events */
void EVENT_USB_Device_Connect(void) {}
void EVENT_USB_Device_Disconnect(void) {}
void EVENT_USB_Device_Reset(void) {}

