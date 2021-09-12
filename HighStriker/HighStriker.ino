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
#define DATA_PIN 4
CRGB leds[NUM_LEDS];

// Length of audio files (milliseconds)
#define UP_TIME 	1750
#define DOWN_TIME 	1375
#define BELL_TIME	1000

#define MARK_WIDTH 	4 	// (pixels) number of LEDs at full brightness at top of lighted section
#define MARK_TAIL	8 	// (pixels) width of faded LEDs after MARK
#define DECAY		127 // (0-255) amount by which brightness drops across MARK_TAIL
#define DECAY_COL 	65  // 255 / MARK_TAIL



// ACCELEROMETER SETUP
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>


// Used for software SPI
#define LIS3DH_CLK 13
#define LIS3DH_MISO 12
#define LIS3DH_MOSI 11
// Used for hardware & software SPI
#define LIS3DH_CS 10

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
   #define Serial SerialUSB
#endif



// HAMMER SETTINGS

#define HIT_MIN	40 // minimum force to trigger hit
#define HIT_MAX 50 // theoretical max hit
#define AVG_QTY 4	// Number of readings to take an average from 
// #define WIN_RATE 5

int win_count = 0;
// int count_loss = 0;
int strength_offset = 0;



//////////////////////////////////////////////////////////////////////////////////



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
	if (strength == 100)
	{
		Serial.print("#WIN\r\n");
	}
	else
	{
		Serial.print("#HIT\r\n");
	}
	 	// Trigger SFX
	// pause_ms(100); 			// wait for audio file to load

	// Variables
	int level = floor(NUM_LEDS * 0.01 * strength); 	// Pixel height to which marker travels

	long startTime = millis(); 						// Time at beginning of animation
	long timeStage = 0;								// Theoretical time since start of animation
	int refreshRate = floor(UP_TIME / level);		// Period between turning on successive LEDs 
	int pauseTime = 0;								// Time to wait between lighitng LEDs (accounting for processing time)

	// Serial.print("startTime = ");
	// Serial.println(startTime, DEC);

	// Serial.print("refreshRate = ");
	// Serial.println(refreshRate, DEC);
	
	// Serial.print("level = ");
	// Serial.println(level, DEC);

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

		// Serial.print("timeStage = ");
		// Serial.println(timeStage, DEC);

		// Serial.print("pauseTime = ");
		// Serial.println(pauseTime, DEC);
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
		pause_ms(BELL_TIME + 200);
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
	// LEDS
	FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);	// setup LED array
	FastLED.setMaxPowerInVoltsAndMilliamps(5,5000); 			//limit power of LEDs
	Serial.begin(9600);

	clearLEDs();
	FastLED.show();

	// ACCELEROMETER 
	if (! lis.begin(0x18)) 
	{   // change this to 0x19 for alternative i2c address
		Serial.println("Couldnt start");
		while (1);
	}
	Serial.println("LIS3DH found!");

	lis.setRange(LIS3DH_RANGE_16_G);   // 2, 4, 8 or 16 G!
	// lis.read(); 
	pause_ms(500);

}

void loop() 
{
	// while (Serial.available() > 0) 
	// {
	// 	// look for the next valid integer in the incoming serial stream:
	// 	int strength = Serial.parseInt();

	// 	// look for the newline. That's the end of your sentence:
	// 	if (Serial.read() == '\n') 
	// 	{
	// 		// constrain the values to 0 - 255 and invert
	// 		// if you're using a common-cathode LED, just use "constrain(color, 0, 255);"
	// 		strength = constrain(strength, 0, 100);


	// 		// print the three numbers in one string as hexadecimal:
	// 		Serial.print("Strength = ");
	// 		Serial.println(strength, DEC);
	// 	}
	// 	animate(strength);
	// }

	sensors_event_t event;

	static int max = 0;
	int ZAcc = 0; 
	ceil(abs(event.acceleration.z - 9.8));
	
	for (int i = 0; i < AVG_QTY; ++i)
	{
		lis.getEvent(&event);
		ZAcc += ceil(abs(event.acceleration.z - 9.8));
		pause_ms(2);
	}




	// lis.read();      // get X Y and Z data at once
	// Then print out the raw data
	// Serial.print("X:  "); Serial.print(lis.x); 
	// Serial.print("  \tY:  "); Serial.print(lis.y); 
	// Serial.print("  \tZ:  "); Serial.print(lis.z); 
	// Serial.print("  \tMAX:  "); Serial.println(max, DEC); 

	// if (lis.x > max)
	// 	max = lis.x;

	// if (lis.y > max)
	// 	max = lis.y;

	// if (lis.z > max)
	// 	max = lis.z;

	/* Or....get a new sensor event, normalized */ 
	// sensors_event_t event; 
	// lis.getEvent(&event);

	/* Display the results (acceleration is measured in m/s^2) */
	// float hit = 4;
	// float win = 100;

	// float xAcc = abs(event.acceleration.x);
	// float yAcc = abs(event.acceleration.y);
	
	if (ZAcc > max)
		max = ZAcc;

	// Serial.print("ZAcc = ");
	// Serial.print(ZAcc, DEC);

	// Serial.print("\tMax = ");
	// Serial.println(max, DEC);

	int strength = 0;
	float strength_temp = 0.0;

	if (ZAcc > (HIT_MIN * AVG_QTY))
	{
		ZAcc = ZAcc / AVG_QTY;
		Serial.print("ZAcc = ");
		Serial.println(ZAcc, DEC);


		strength_temp = (((float) ZAcc - (float) HIT_MIN) / (float) HIT_MAX) *100.0;// / (HIT_MAX * 100));
		Serial.print("Strength = ");
		Serial.println(strength_temp, DEC);

		strength = constrain(strength_temp + strength_offset, 15, 100);

		animate(strength);

		if (strength == 100) // if we win
		{	
			win_count -= 20;
		}

		else
		{
			win_count += 5;
		}

		if (win_count >= 75)
		{
			win_count = 75;
		}
		else if (win_count <= 0)
		{
			win_count = 0;
		}

		strength_offset = random(win_count, 80);
		


	}



	// float totalAcc = 0;
	// if (xAcc > yAcc)
	// {
	// 	totalAcc = max(xAcc, ZAcc);
	// }
	// else 
	// {
	// 	totalAcc = max(yAcc, ZAcc);
	// }


	  /* Display the results (acceleration is measured in m/s^2) */
	// Serial.print("\t\tX: "); Serial.print(event.acceleration.x);
	// // Serial.print(" \tY: "); Serial.print(event.acceleration.y); 
	// Serial.print(" \tZ: "); Serial.print(event.acceleration.z); 
	// Serial.println(" m/s^2 ");



}

