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
uint32_t MainColourFore = 0xFFFF00; // Yellow
uint32_t AuxColourFore = 0xFFFFFF; // White
uint32_t AnnunColourFore = 0x00FF00; // Green

static void CheckDisplayStatus(void);

_Bool onethousandmVmodedetected;
char MaindisplayString[19] = "";              // String for G[1] to G[18]
_Bool displayBlank = false;
_Bool displayBlankPrevious = false;

//float test15 = 0;
//char test16[12];


//uint16_t dollarPositionAUX = 0xFFFF;


// 6243 testing
volatile int aux_dollarCount = 0;
volatile uint16_t aux_dollarPositions[5] = { 0, 0, 0, 0, 0 };
volatile char aux_string_dbg[30] = { 0 };



//************************************************************************************************************************************************************


void DisplayMain() {

	// This sub needs re-written in the same way the AUX line is now written - it's on the todo list.

	// MAIN ROW - Print text to LCD, detect if there is an OHM symbol ($) and if so split into 3 parts, before-OHM-after
	SetTextColors(MainColourFore, 0x000000); // Foreground, Background

	//char MaindisplayString[19] = "";              // String for G[1] to G[18]
	char BeforeDollar[19] = "";                   // To store characters before the $
	char AfterDollar[19] = "";                    // To store characters after the $
	uint16_t dollarPosition = 0xFFFF;            // Initialize to an invalid position

	// Populate MaindisplayString from G[1] to G[18]
	for (int i = 1; i <= 18; i++) {
		MaindisplayString[i - 1] = G[i];
	}
	MaindisplayString[18] = '\0';                 // Null-terminate MaindisplayString

	// Find the position of the '$' symbol
	for (int i = 0; i < 18; i++) {
		if (MaindisplayString[i] == '$') {
			dollarPosition = i;                   // Record the position of '$'
			break;                                // Exit loop once found
		}
	}

	// If in 2W or 4W Resistance measurement mode the display will contain the OHM symbol on the MAIN display.
	// If it appears then the dollarPositionAUX var will not be 0xFFFF
	if (dollarPosition != 0xFFFF) {

		// $ symbol found
		// Before
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
			Xpos_MAIN,      // Cursor X
			0        // Cursor Y
		);
		char MaindisplayStringBefore[19] = "";
		for (int i = 1; i <= 18; i++) {
			if (G[i] == '$') break;
			MaindisplayStringBefore[i - 1] = G[i];
		}
		DrawText(MaindisplayStringBefore);

		//HAL_Delay(5);
		Delay_NonBlocking(5);  // Wait ms in a non-blocking way

		ResetGraphicWritePosition_LT();
		Ohms16x32SymbolStoreUCG();			// This is called here rather than pre-defined because the 2nd UCG below is a different size
		Text_Mode();

		// Ohm Symbol
		// Calculate position of $ symbol
		//       yposohm = (dollarPosition * widthmultiplier * fontwidth) + (characterspacing * dollarPosition) + startoffset(6)
		uint16_t yposohm = (dollarPosition * 3 * 16) + (4 * dollarPosition);  // Y position of the OHM symbol
		// The actual OHM
		ConfigureFontAndPosition(
			0b10,    // User-Defined Font mode
			0b10,    // Font size
			0b00,    // ISO 8859-1
			0,       // Full alignment enabled
			0,       // Chroma keying disabled
			1,       // Rotate 90 degrees counterclockwise
			0b10,    // Width multiplier
			0b10,    // Height multiplier
			1,       // Line spacing
			4,       // Character spacing
			Xpos_MAIN,      // Cursor X 170
			yposohm  // Cursor Y 480	780
		);
		// Write the OHM symbol
		WriteRegister(0x04);
		WriteData(0x00);    // high byte
		WriteData(0x00);    // low byte

		//HAL_Delay(2);
		Delay_NonBlocking(2);  // Wait ms in a non-blocking way

		// After
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
			Xpos_MAIN,      // Cursor X
			(dollarPosition * 52) + 52        // Cursor Y	832
		);
		char MaindisplayStringAfter[19] = ""; // Adjust the size to match your max expected characters
		int j = 0; // Index for the new string
		for (int i = dollarPosition + 1; i <= 18; i++) { // Start after the $ and loop through the rest
			MaindisplayStringAfter[j++] = MaindisplayString[i]; // Copy characters to the new string
		}
		MaindisplayStringAfter[18] = '\0'; // Null-terminate the new string
		DrawText(MaindisplayStringAfter);

	} else {

		// Standard non-OHM mode (6243): just draw G[1]..G[18]
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

		char MaindisplayStringStd[19] = "";
		for (int i = 1; i <= 18; i++) {
			MaindisplayStringStd[i - 1] = G[i];
		}
		MaindisplayStringStd[18] = '\0';
		DrawText(MaindisplayStringStd);

		CheckDisplayStatus();		

	}

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

	SetTextColors(AuxColourFore, 0x000000); // Foreground, Background

	char AuxdisplayString[30] = "";               // String for G[19] to G[47]
	uint16_t dollarPositions[5] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }; // To store positions of up to 5 $
	int dollarCount = 0; // Count of $ symbols found

	// Customizable fudge factors for each $ symbol position
	//int fudgeFactors[5] = { 23, 23, 32, 104 }; // Adjust these values for each $ symbol's position

	// Populate AuxdisplayString from G[19] to G[47]
	for (int i = 19; i <= 47; i++) {
		AuxdisplayString[i - 19] = G[i];
	}
	AuxdisplayString[29] = '\0'; // Null-terminate at the 30th position because array starts at 0

	// Find positions of up to 5 $ symbols
	for (int i = 0; i < 30 && dollarCount < 5; i++) {
		if (AuxdisplayString[i] == '$') {
			dollarPositions[dollarCount++] = i; // Store position and increment count
		}
	}
	
	
	
	aux_dollarCount = dollarCount;
	for (int i = 0; i < 5; i++) aux_dollarPositions[i] = dollarPositions[i];
	for (int i = 0; i < 30; i++) aux_string_dbg[i] = AuxdisplayString[i]; // includes '\0' somewhere

	
	
	

	uint16_t yposohm = 0; // Initialize y-position for OHM symbol
	uint16_t yposoffset = 60; // Initialize y-position for OHM symbol

	if (dollarCount != 0) {

		// Process text and OHM symbols based on dollarCount
		for (int d = 0; d <= dollarCount; d++) {
			// Calculate start and end positions for text
			int start = (d == 0) ? 0 : dollarPositions[d - 1] + 1;
			int end = (d < dollarCount) ? dollarPositions[d] : 30;

			// Print text before or between $ symbols
			if (start < end) {
				char AuxdisplaySegment[30] = "";
				for (int i = start; i < end; i++) {
					AuxdisplaySegment[i - start] = AuxdisplayString[i];
				}
				AuxdisplaySegment[end - start] = '\0'; // Null-terminate

				ConfigureFontAndPosition(
					0b00,    // Internal CGROM
					0b01,    // Font size
					0b00,    // ISO 8859-1
					0,       // Full alignment enabled
					0,       // Chroma keying disabled
					1,       // Rotate 90 degrees counterclockwise
					0b01,    // Width multiplier
					0b01,    // Height multiplier
					1,       // Line spacing
					0,       // Character spacing
					Xpos_AUX,     // Cursor X
					yposohm = yposoffset + (start * 12 * 2) // Dynamically adjust position, offset + (position * 12 pixel width character * 2)
				);
				DrawText(AuxdisplaySegment);
			}

			//HAL_Delay(5);
			Delay_NonBlocking(5);  // Wait ms in a non-blocking way

			ResetGraphicWritePosition_LT();
			Ohms12x24SymbolStoreUCG(); // This is called here rather than pre-defined because the 1st UCG above is a different size
			Text_Mode();

			// Print OHM symbol if within dollarCount
			if (d < dollarCount) {
				uint16_t calculated_value = (dollarPositions[d] * 12 * 2);		// 12 pixel width character * 2
				yposohm = yposoffset + (uint16_t)calculated_value;

				ConfigureFontAndPosition(
					0b10,    // User-Defined Font mode
					0b01,    // Font size
					0b00,    // ISO 8859-1
					0,       // Full alignment enabled
					0,       // Chroma keying disabled
					1,       // Rotate 90 degrees counterclockwise
					0b01,    // Width multiplier
					0b01,    // Height multiplier
					1,       // Line spacing
					0,       // Character spacing
					Xpos_AUX,     // Cursor X
					yposohm  // Cursor Y
				);
				WriteRegister(0x04);
				WriteData(0x00);    // high byte
				WriteData(0x00);    // low byte
			}
		}

	}

	// If no $ symbols were found, print the entire string as-is	

	if (dollarCount == 0) {
		
		
		// 6243 testing
		//DrawText(AuxdisplayString);

		

		// Detect 1000mV Range
		//if ((strstr(MaindisplayString, "OVERLOAD") == NULL) &&			// does not contain
		//	(strpbrk(MaindisplayString, "0123456789") != NULL) &&		// does contain
		//	(strstr(AuxdisplayString, "1000mV Range") != NULL)) {		// does contain
		//	onethousandmVmodedetected = true;
		//} else {
		//	onethousandmVmodedetected = false;
		//	oneVoltmode = false;
		//}
		//if ((strstr(AuxdisplayString, "1000mV Range") != NULL)) {		// does contain
		//	onethousandmVmodedetected = true;
		//}
		//else {
		//	onethousandmVmodedetected = false;
			//oneVoltmode = false;
		//}

		// If in 1000mV range and user has enabled the new 1VDC mode
		//if (oneVoltmode && onethousandmVmodedetected) {
		//	yposohm = 60;
		//}
		//else {
		//	yposohm = 60;
		//}
		
		yposohm = 60;

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
			yposohm       // Cursor Y
		);

		// If in 1000mV range and user has enabled the new 1VDC mode
		//if (oneVoltmode && onethousandmVmodedetected) {
		//	DrawText("   1 V Range                 ");
		//}
		//else {
		DrawText(AuxdisplayString);
		//}

	}
}


//******************************************************************************

void DisplayAnnunciators() {

	// ANNUNCIATORS - Print or clear text on the LCD
	const char* AnnuncNames[19] = {
		"SMPL", "IDLE", "AUTO", "LOP", "NULL", "DFILT", "MATH", "AZERO",
		"ERR", "INFO", "FRONT", "REAR", "SLOT", "LO_G", "RMT", "TLK",
		"LTN", "SRQ"
	};


	// Set Y-position of the annunciators
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
			SetTextColors(AnnunColourFore, 0x000000); // Foreground: Green, Background: Black
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
			SetTextColors(0x000000, 0x000000); // Foreground: Black, Background: Black
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
			SetTextColors(0x00FF00, 0x000000); // Foreground: Yellow, Background: Black
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

			SetTextColors(0xFF0000, 0x000000); // Foreground: Yellow, Background: Black
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
				640      // Cursor Y
			);
			char text2[] = "                        ";
			DrawText(text2);

		}
		else {
			// Perform operations within the 5-second window
			// Splash text
			SetTextColors(0x00FF00, 0x000000); // Foreground: Yellow, Background: Black
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

			SetTextColors(0x909090, 0x000000); // Foreground: grey, Background: Black
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
				640      // Cursor Y
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
