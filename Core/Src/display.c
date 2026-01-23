/**
  ******************************************************************************
  * @file    display.c
  * @brief   This file provides code for the
  *          display MAIN, AUX, ANNUNCIATORS & SPLASH.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "main.h"
#include "lcd.h"
#include "lt7680.h"
#include "display.h"
#include <string.h>  // For strchr, strncpy
#include <stdio.h>   // For debugging (optional)
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>		// required for float (soft FPU)


#define DURATION_MS 5000     // 5 seconds in milliseconds
#define TIMER_INTERVAL_MS 35 // The interval of your timed sub in milliseconds

// Display colours default
uint32_t MainColourFore = 0xFFFF00;		// Yellow
uint32_t AuxColourFore = 0xFFFFFF;		// White
uint32_t AnnunColourFore = 0x00FF00;	// Green
uint32_t ColourBackground = 0x000000;	// Black
uint32_t ColourBlackFore = 0x000000;	// Black

static void CheckDisplayStatus(void);

_Bool onethousandmVmodedetected;
char MaindisplayString[19] = "";              // String for G[1] to G[18]
_Bool displayBlank = false;
_Bool displayBlankPrevious = false;

// 6243 testing
volatile int aux_dollarCount = 0;
volatile uint16_t aux_dollarPositions[5] = { 0, 0, 0, 0, 0 };
volatile char aux_string_dbg[30] = { 0 };



//************************************************************************************************************************************************************


void DisplayMain() {

	// MAIN ROW - Print text to LCD
	SetTextColors(MainColourFore, ColourBackground); // Foreground, Background

	char MaindisplayStringStd[19] = "";					// String for G[1] to G[18]
	
	// Populate MaindisplayStringStd from G[1] to G[18]
	for (int i = 1; i <= 18; i++) {
		MaindisplayStringStd[i - 1] = G[i];
	}
	MaindisplayStringStd[18] = '\0';					// Null-terminate at the 19th position because array starts at 0

	// draw G[1]..G[18]
	ConfigureFontAndPosition(
		0b00,    // Internal CGROM
		0b10,    // Font size
		0b00,    // ISO 8859-1
		0,       // Full alignment enabled
		0,       // Chroma keying disabled
		1,       // Rotate 90 degrees counterclockwise
		0b10,    // Width multiplier
		0b10,    // Height multiplier
		1,       // Line spacing
		4,       // Character spacing
		Xpos_MAIN,
		0);

	DrawText(MaindisplayStringStd);

	CheckDisplayStatus();		

}


//******************************************************************************


// "DISPLAY OFF" logic
void CheckDisplayStatus() {

	// Check if "DISPLAY OFF" is present in MaindisplayString
	displayBlank = (strstr(MaindisplayString, "DISPLAY OFF") != NULL);

	// If the display status has changed
	if (displayBlank != displayBlankPrevious) {
		if (displayBlank) {
			// Changed to "DISPLAY OFF"
			ConfigurePWMAndSetBrightness(0);
			LCDConfigTurnOff_LT();
		}
		else {
			// Changed from "DISPLAY OFF" to something else, i.e. user has pressed a button to revive
			ConfigurePWMAndSetBrightness(BACKLIGHTFULL);  // Configure Timer-1 and PWM-1 for backlighting. Settable 0-100%
			LCDConfigTurnOn_LT();
		}
	}

	// Update the previous status
	displayBlankPrevious = displayBlank;

}


//******************************************************************************


void DisplayAux() {

	// AUX ROW text to LCD

	SetTextColors(AuxColourFore, ColourBackground); // Foreground, Background

	char AuxdisplayString[30] = "";						// String for G[19] to G[47]
	
	// Populate AuxdisplayString from G[19] to G[47]
	for (int i = 19; i <= 47; i++) {
		AuxdisplayString[i - 19] = G[i];
	}
	AuxdisplayString[29] = '\0';						// Null-terminate at the 30th position because array starts at 0

	ConfigureFontAndPosition(
		0b00,    // Internal CGROM
		0b01,    // Font size
		0b00,    // ISO 8859-1
		0,       // Full alignment enabled
		0,       // Chroma keying disabled
		1,       // Rotate 90 degrees counterclockwise
		0b01,    // Width multiplier
		0b01,    // Height multiplier
		5,       // Line spacing
		0,       // Character spacing
		Xpos_AUX,     // Cursor X
		Ypos_AUX      // Cursor Y
	);

	DrawText(AuxdisplayString);

}


//******************************************************************************

void DisplayAnnunciators() {

	// ANNUNCIATORS - Print or clear text on the LCD
	const char* AnnuncNames[19] = {
		"SMPL", "IDLE", "AUTO", "LOP", "NULL", "DFILT", "MATH", "AZERO",
		"ERR", "INFO", "FRONT", "REAR", "SLOT", "LO_G", "RMT", "TLK",
		"LTN", "SRQ"
	};


	// Set Y-position of the various annunciators
	int AnnuncYCoords[19] = {
		10,   // SMPL
		62,   // IDLE
		114,  // AUTO
		166,  // LOP
		218,  // NULL
		270,  // DFILT
		322,  // MATH
		374,  // AZERO
		426,  // ERR
		478,  // INFO
		530,  // FRONT
		582,  // REAR
		634,  // SLOT
		686,  // LO_G
		738,  // RMT
		790,  // TLK
		842,  // LTN
		900   // SRQ
	};


	for (int i = 0; i < 18; i++) {
		if (Annunc[i + 1] == 1) {  // Turn the annunciator ON
			SetTextColors(AnnunColourFore, ColourBackground); // Foreground: Green, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // 16-dot font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width X0
				0b01,    // Height X0
				5,       // Line spacing
				0,       // Character spacing
				Xpos_ANNUNC,  // Cursor X (fixed)
				AnnuncYCoords[i] // Cursor Y (from array)
			);
			DrawText(AnnuncNames[i]); // Print the corresponding name
		}
		else {  // Turn the annunciator OFF
			SetTextColors(ColourBlackFore, ColourBackground); // Foreground: Black, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // 16-dot font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width X0
				0b01,    // Height X0
				5,       // Line spacing
				0,       // Character spacing
				Xpos_ANNUNC,  // Cursor X (fixed)
				AnnuncYCoords[i] // Cursor Y (from array)
			);
			DrawText(AnnuncNames[i]); // Clear the text by drawing in black
		}
	}

}

//******************************************************************************

void DisplaySplash() {

	// Splash text to display
	static uint32_t cycle_count = 0; // Persistent counter for cycles
	static uint8_t timer_active = 1; // Flag to track timer status
	if (timer_active) {
		cycle_count++;
		// Check if the 5-second period has elapsed
		if (cycle_count >= (DURATION_MS / TIMER_INTERVAL_MS)) {
			// Runs once
			timer_active = 0; // Stop counting after 5 seconds
			SetTextColors(0x00FF00, ColourBackground); // Foreground: Yellow, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // Font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width multiplier
				0b00,    // Height multiplier
				1,       // Line spacing
				4,       // Character spacing
				Xpos_SPLASH,     // Cursor X
				Ypos_SPLASH      // Cursor Y
			);
			char text[] = "                                                           ";
			DrawText(text);

			SetTextColors(0xFF0000, ColourBackground); // Foreground: Yellow, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // Font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width multiplier
				0b00,    // Height multiplier
				1,       // Line spacing
				4,       // Character spacing
				Xpos_TIMINGS,     // Cursor X			// org 130
				Ypos_TIMINGS      // Cursor Y
			);
			char text2[] = "                        ";
			DrawText(text2);

		}
		else {
			// Perform operations within the 5-second window
			// Splash text
			SetTextColors(0x00FF00, ColourBackground); // Foreground: Yellow, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // Font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width multiplier
				0b00,    // Height multiplier
				1,       // Line spacing
				4,       // Character spacing
				Xpos_SPLASH,     // Cursor X
				Ypos_SPLASH      // Cursor Y
			);
			char text[] = "Serial decode by MickleT / TFT LCD by Ian Johnston";
			DrawText(text);

			SetTextColors(0x909090, ColourBackground); // Foreground: grey, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // Font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width multiplier
				0b00,    // Height multiplier
				1,       // Line spacing
				4,       // Character spacing
				Xpos_TIMINGS,     // Cursor X			// org 130
				Ypos_TIMINGS      // Cursor Y
			);
			char textsettings[128]; // Ensure the buffer is large enough
			snprintf(textsettings, sizeof(textsettings),
				"%d %d %d %d %d %d %d %s",
				LCD_VBPD,
				LCD_VFPD,
				LCD_VSPW,
				LCD_HBPD,
				LCD_HFPD,
				LCD_HSPW,
				REFRESH_RATE,
				ADA_BUY
			);
			DrawText(textsettings);
		}
	}

}
