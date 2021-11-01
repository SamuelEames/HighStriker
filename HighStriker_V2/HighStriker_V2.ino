// Code for High Striker game
// Authors: S.Eames, J.Crawford
// Date: September 2018
// Major overhaul by S.Eames October 2021


//////////////////// DEBUG SETUP ////////////////////
#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG
	#define DPRINT(...)   Serial.print(__VA_ARGS__)   //DPRINT is a macro, debug print
	#define DPRINTLN(...) Serial.println(__VA_ARGS__) //DPRINTLN is a macro, debug print with new line
#else
	#define DPRINT(...)                       //now defines a blank line
	#define DPRINTLN(...)                     //now defines a blank line
#endif

#define FASTLED_INTERNAL			// Stop the annoying messages during compile


//////////////////// LIBRARIES //////////////////////
#include <Adafruit_NeoPixel.h>
#include "HX711.h"
#include "Adafruit_Soundboard.h"


////////////////////// IO SETUP /////////////////////
// NOTE: SFX module connected to Serial1 port (pins 0-1)
#define HX711_DOUT_PIN 	2 		// Air pressure sensor - Data
#define HX711_SCK_PIN 	3 		// Air pressure sensor - Clock
#define LED_PIN 				6 		// Pixel string

#define PIN_SFX_RST 		4			// Reset line for SFX


//////////////////// PIXEL SETUP ////////////////////
#define NUM_LEDS    	300 		
#define BRIGHTNESS  	255 		// Range 0-255
#define REFRESH_INT		20 			// (ms) Refresh interval of pixel strip (because updating them takes FOREVER) (40 = 25Hz)

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

#define COL_BLACK 		0x00000000
#define COL_WHITE 		0xFF000000


#define MARK_WIDTH 		4 			// (pixels) number of LEDs at full brightness at top of lighted section
#define MARK_TAIL			8 			// (pixels) width of faded LEDs after MARK
#define DECAY					127 		// (0-255) amount by which brightness drops across MARK_TAIL
#define DECAY_COL 		65  		// 255 / MARK_TAIL

//////////////////// PRESSURE SENSOR SETUP ////////////////////
HX711 airPSensor;

#define NUM_READINGS 10								// Number of readings to take an average from 
int32_t airPReadings[NUM_READINGS]; 	// Holds air pressure readings

int32_t lastAPReading; 

/* NOTE:
			max value = 8388607
			min value = -8388608
			steady state seems to float between 4673000 - 4676000
*/ 

//////////////////// SOUND MODULE SETUP ////////////////////
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, PIN_SFX_RST);
// NOTE: Connect UG to ground to have the sound board boot into UART mode

// Audio filenames (format should be 8chars, no '.' and then extension)
char Audio_Beep[] = "T01NEXT0WAV";


// Length of audio files (milliseconds)
#define UP_TIME 	1750
#define DOWN_TIME 	1375
#define BELL_TIME	1000


//////////////////// MISC VARIABLES ////////////////////
// HAMMER SETTINGS

#define HIT_MIN	800 		// minimum force to trigger hit
#define HIT_MAX 3000 		// theoretical max hit
// #define WIN_RATE 5

int win_count = 0;
// int count_loss = 0;
int strength_offset = 0;



//////////////////////////////////////////////////////////////////////////////////



// void clearLEDs()
// {
// 	leds.fill(COL_BLACK, 0, NUM_LEDS);

// 	// FastLED.show();
// 	return;
// }


void pause_ms(uint32_t pauseTime)
{
	// Pauses for given time period 
	uint32_t startTime = millis();

	if (millis() < startTime) 
		startTime = millis(); // we wrapped around, lets just try again

	while ((startTime + pauseTime) > millis())
		; // do nothing
	
	return;
}


void animate(uint16_t strength)
{
	// strength equates to the % of LED strip to light up 
	#ifdef DEBUG
		if (strength == 100)
			Serial.print("#WIN\r\n");
		else
			Serial.print("#HIT\r\n");
	#endif


	// Variables
	uint16_t 	level = floor(NUM_LEDS * 0.01 * strength); 		// Pixel height to which marker travels

	uint32_t 	startTime = millis(); 												// Time at beginning of animation
	uint32_t 	timeStage = 0;																// Theoretical time since start of animation
	uint8_t 	refreshRate = floor(UP_TIME / level);					// Period between turning on successive LEDs 
	uint8_t 	pauseTime = 0;																// Time to wait between lighitng LEDs (accounting for processing time)


	leds.clear();

	////////////////////////////////
	////////// ANIMATE UP //////////
	////////////////////////////////
	

	for (uint16_t i = 0; i < level; ++i)
	{
		// Light up some LEDs!
		// leds[i] = COL_WHITE;
		leds.setPixelColor(i, 0xFFFF1100);
		
		// Drop brightness of trailing LEDs
		if (i > MARK_WIDTH)
		{
			for (uint16_t j = (i - MARK_WIDTH); j > (i - MARK_TAIL); --j)
			{
				leds.setPixelColor(j, (leds.getPixelColor(j) - 0x11000000));
				// leds[j] -= CRGB( 0, DECAY_COL, DECAY_COL); // fade tail to red colour
				// leds[j].fadeLightBy( DECAY );
			}	
		}

		leds.show();

		// Calculate time to wait between frames (to keep in sync with audio)
		timeStage += refreshRate;
		pauseTime = startTime + timeStage - millis(); 		

		if (pauseTime > 0)
			pause_ms(pauseTime);
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
		uint16_t temp = (millis() - startTime);
		while (temp < BELL_TIME)
		{
			// fadeToBlackBy( leds, NUM_LEDS, 10);
			uint16_t pos = random(NUM_LEDS);
			// leds[pos] += CHSV( 0 + random8(64), 200, 255);
			// pause_ms(5); 
			leds.show();
			temp = (millis() - startTime);
		}

		// Fade off remaining LEDs
		for (uint8_t i = 0; i < 255; ++i)
		{
			for (uint16_t j = 0; j < NUM_LEDS; ++j)
				// leds[j] -= CRGB( 1, 1, 1);
				leds.setPixelColor(j, (leds.getPixelColor(j) - 0x01010101)); // Fix this


			leds.show();
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
		for (uint16_t i = level; i > 0; --i)
		{
			// leds[i] = CRGB::Black;
			leds.setPixelColor(i, COL_BLACK);
			
			// Draw MARK segment
			if ((i - MARK_WIDTH) >= 0)
				// leds[i - MARK_WIDTH] = CRGB::White;
				leds.setPixelColor(i - MARK_WIDTH, COL_WHITE);

			leds.show();

			// Calculate time to wait between frames (to keep in sync with audio)
			timeStage += refreshRate;
			pauseTime = startTime + timeStage - millis(); 		

			if (pauseTime > 0)
				pause_ms(pauseTime);
		}

		Serial.println("Hey");
	}

	// Reset air pressure readings ready for next play
	for (uint8_t i = 0; i < NUM_READINGS; ++i)
		airPReadings[i] = 0;

	while (!airPSensor.is_ready());				// Wait until sensor ready
	lastAPReading = airPSensor.read();		// Take reading

	return;
}


void setup() 
{
	// LEDS
	// FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);	// setup LED array
	// FastLED.setMaxPowerInVoltsAndMilliamps(5,5000); 			//limit power of LEDs

	leds.begin(); 							// Initialise Pixels
	leds.show();								// Turn them all off
	leds.setBrightness(BRIGHTNESS);


	leds.fill(COL_WHITE, 0, NUM_LEDS);
	leds.show();


	Serial.begin(9600);

	// Instantiate air pressure sensor
	airPSensor.begin(HX711_DOUT_PIN, HX711_SCK_PIN);



	// // ACCELEROMETER 
	// if (! lis.begin(0x18)) 
	// {   // change this to 0x19 for alternative i2c address
	// 	Serial.println("Couldnt start");
	// 	while (1);
	// }
	// Serial.println("LIS3DH found!");

	// lis.setRange(LIS3DH_RANGE_16_G);   // 2, 4, 8 or 16 G!
	// // lis.read(); 
	// pause_ms(500);

	// Initialise Serial debug
	#ifdef DEBUG
		Serial.begin(115200);					// Open comms line
		while (!Serial) ; 					// Wait for serial port to be available

		Serial.println(F("Wassup?"));
	#endif

	// Fill table of air pressure readings
	// NOTE: 10 reading takes 861ms to do
	// uint8_t readingNum = 0;

	// while (readingNum <= NUM_READINGS)
	// {
	// 	if (airPSensor.is_ready())
	// 		airPReadings[readingNum++] = airPSensor.read();
	// 	delay(100);
	// }

	while (!airPSensor.is_ready());				// Wait until sensor ready
	lastAPReading = airPSensor.read();		// Take reading

}

void loop() 
{

	// Take air pressure reading
	static uint8_t readingNum = 0;

	int32_t airPReading_RAW;

	if (airPSensor.is_ready())
	{
		airPReading_RAW = airPSensor.read();

		airPReadings[readingNum++] = airPReading_RAW - lastAPReading;
		lastAPReading = airPReading_RAW;

		// DPRINT(F("Air Pressure = "));
		// DPRINT(airPReadings[readingNum -1]);



		// airPReadings[readingNum++] = airPSensor.read();
		// DPRINT(F("Air Pressure = "));
		// DPRINTLN(airPReadings[readingNum -1]);

		// if (readingNum >=2)
		// {
		// 	DPRINT(F("\t\tDifference = "));
		// 	DPRINTLN(airPReadings[readingNum - 1] - airPReadings[readingNum-2]);
		// 	/* code */
		// }
	}
	else
	{ DPRINTLN(F("Pressure sensor not found")); }

	if (readingNum >= NUM_READINGS)
		readingNum = 0;

	delay(100); // Can only take a reading roughly every 87ms

	// DPRINT(F(", "));
	DPRINTLN(getAirPAvg());
	



	// Take a reading from the air pressure sensor
	

	// sensors_event_t event;

	static uint32_t actualMax = HIT_MAX; // 
	// // int ZAcc = 0; 
	// // ceil(abs(event.acceleration.z - 9.8));
	
	// for (uint8_t i = 0; i < AVG_QTY; ++i)
	// {
	// 	// lis.getEvent(&event);
	// 	// ZAcc += ceil(abs(event.acceleration.z - 9.8));
	// 	// pause_ms(2);
	// }

	// Get smoothed air pressure value
	uint32_t airPSmooth = getAirPAvg();

	// If it's greater than our current recorded max value, update that
	if (airPSmooth > actualMax)
		actualMax = airPSmooth;

	uint8_t strength = 0;
	float strength_temp = 0.0;

	if (airPSmooth > HIT_MIN)
	{


		// ZAcc = ZAcc / AVG_QTY;
		Serial.print("airPSmooth = ");
		Serial.println(airPSmooth, DEC);


		strength_temp = (((float) airPSmooth - (float) HIT_MIN) / (float) actualMax) *100.0;// / (HIT_MAX * 100));
		Serial.print("Strength = ");
		Serial.println(strength_temp, DEC);

		strength = constrain(strength_temp + strength_offset, 15, 100);

		animate(strength);

		if (strength == 100) // if we win
			win_count -= 20;
		else
			win_count += 5;
		if (win_count >= 75)
			win_count = 75;
		else if (win_count <= 0)
			win_count = 0;

		strength_offset = random(win_count, 80);
	}
}



int32_t getAirPAvg()
{
	// Returns averaged air pressure
	uint32_t airPAverage = 0; //= airPReadings[0]; // set first 

	// Add up positive values - we don't need negative ones in this case
	for (uint8_t i = 0; i < NUM_READINGS; ++i)
	{
		if (airPReadings[i] > 0)
			airPAverage += airPReadings[i];
	}

	return (airPAverage/NUM_READINGS);
}

