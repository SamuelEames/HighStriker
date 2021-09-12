// Code for High Striker game
// Authors: S.Eames, J.Crawford
// Date: September 2018



// TODO
/**


 * Indication that RPI is ready
 * Run with sensor - button / vibration / accelerometer 
 * Add randomness? 


**/

// Pixel LED Setup
// #define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#define NUM_LEDS 174
#define DATA_PIN 2
CRGB leds[NUM_LEDS];

// Length of audio files (milliseconds)
#define UP_TIME 	1375
#define DOWN_TIME 	1375
#define BELL_TIME	1000

#define MARK_WIDTH 	2 	// (pixels) number of LEDs at full brightness at top of lighted section
#define MARK_TAIL	8 	// (pixels) width of faded LEDs after MARK
#define DECAY		127 // (0-255) amount by which brightness drops across MARK_TAIL
#define DECAY_COL 	65  // 255 / MARK_TAIL



void clearLEDs()
{
	for (int i = 0; i < NUM_LEDS; ++i)
		leds[i] = CRGB::Black;

	// FastLED.show();
	return;
}


void pause_ms(unsigned int pauseTime)
{
	long startTime = millis();

	if (millis() < startTime) 
		startTime = millis(); // we wrapped around, lets just try again

	while ((startTime + pauseTime) > millis())
		; // do nothing
	
	return;
}


void animate(unsigned int strength)
{
	Serial.println('U'); 	// Trigger SFX
	// pause_ms(100); 			// wait for audio file to load

	// Variables
	int level = floor(NUM_LEDS * 0.01 * strength); 	// Pixel height to which marker travels

	long startTime = millis(); 						// Time at beginning of animation
	long timeStage = 0;								// Theoretical time since start of animation
	int refreshRate = floor(UP_TIME / level);		// Period between turning on successive LEDs 
	int pauseTime = 0;								// Time to wait between lighitng LEDs (accounting for processing time)

	Serial.print("startTime = ");
	Serial.println(startTime, DEC);

	Serial.print("refreshRate = ");
	Serial.println(refreshRate, DEC);
	
	Serial.print("level = ");
	Serial.println(level, DEC);

	clearLEDs();

	////////////////////////////////
	////////// ANIMATE UP //////////
	////////////////////////////////
	

	for (int i = 0; i < level; ++i)
	{
		// Light up some LEDs!
		leds[i] = CRGB::White;
		
		// Drop brightness of trailing LEDs
		if (i > MARK_WIDTH)
		{
			for (int j = (i - MARK_WIDTH); j > (i - MARK_TAIL); --j)
			{
				leds[j] -= CRGB( 0, DECAY_COL, DECAY_COL); // fade tail to red colour
				leds[j].fadeLightBy( DECAY );
			}	
		}

		FastLED.show();

		// Calculate time to wait between frames (to keep in sync with audio)
		timeStage += refreshRate;
		pauseTime = startTime + timeStage - millis(); 		

		if (pauseTime > 0)
			pause_ms(pauseTime);

		Serial.print("timeStage = ");
		Serial.println(timeStage, DEC);

		Serial.print("pauseTime = ");
		Serial.println(pauseTime, DEC);
	}

	Serial.println("we got here!"); 

	////////////////////////////////
	////////// ??WINNER?? //////////
	////////////////////////////////
	if (strength >= 100) // IF strength = 100, trigger bell animation
	{
		Serial.println('W'); 	// Trigger SFX
		// pause_ms(100); 			// wait for audio file to load

		startTime = millis(); 


		// // random colored speckles that blink in and fade smoothly
		int temp = (millis() - startTime);
		while (temp < BELL_TIME)
		{
			fadeToBlackBy( leds, NUM_LEDS, 10);
			int pos = random16(NUM_LEDS);
			leds[pos] += CHSV( 0 + random8(64), 200, 255);
			// pause_ms(5); 
			FastLED.show();
			temp = (millis() - startTime);
		}

		// Fade off remaining LEDs
		for (int i = 0; i < 255; ++i)
		{
			for (int j = 0; j < NUM_LEDS; ++j)
				leds[j] -= CRGB( 1, 1, 1);

			FastLED.show();
		}

	}


	////////////////////////////////
	///////// ANIMATE DOWN /////////
	////////////////////////////////
	else						// Don't animate down for winners
	{
		Serial.println('D'); // Trigger SFX
		// pause_ms(500); 			// wait for audio file to load	
		startTime = millis(); 
		timeStage = 0;	

		// Turn off LEDs from top to bottom of lighted section
		for (int i = level; i >= 0; --i)
		{
			leds[i] = CRGB::Black;
			
			// Draw MARK segment
			if ((i - MARK_WIDTH) >= 0)
				leds[i - MARK_WIDTH] = CRGB::White;

			FastLED.show();

			// Calculate time to wait between frames (to keep in sync with audio)
			timeStage += refreshRate;
			pauseTime = startTime + timeStage - millis(); 		

			if (pauseTime > 0)
				pause_ms(pauseTime);

/*			Serial.print("timeStage = ");
			Serial.println(timeStage, DEC);

			Serial.print("pauseTime = ");
			Serial.println(pauseTime, DEC);*/
		}

	}

/*	long animationTime = millis() - startTime;
	Serial.print("animationTime = ");
	Serial.println(animationTime, DEC);*/
	return;
}


void setup() 
{
	FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);	// setup LED array
	FastLED.setMaxPowerInVoltsAndMilliamps(5,5000); 			//limit power of LEDs
	Serial.begin(115200);

	clearLEDs();
	FastLED.show();
}

void loop() 
{
	// int strength = 0;
	// if there's any serial available, read it:
	while (Serial.available() > 0) 
	{
		// look for the next valid integer in the incoming serial stream:
		int strength = Serial.parseInt();

		// look for the newline. That's the end of your sentence:
		if (Serial.read() == '\n') 
		{
			// constrain the values to 0 - 255 and invert
			// if you're using a common-cathode LED, just use "constrain(color, 0, 255);"
			strength = constrain(strength, 0, 100);


			// print the three numbers in one string as hexadecimal:
			Serial.print("Strength = ");
			Serial.println(strength, DEC);
		}
		animate(strength);
	}


	


}

