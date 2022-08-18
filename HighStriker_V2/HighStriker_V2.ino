// Code for High Striker game
// Authors: S.Eames, J.Crawford
// Date: September 2018
// Major overhaul by S.Eames October 2021


//////////////////// DEBUG SETUP ////////////////////
// #define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG
  #define DPRINT(...)   Serial.print(__VA_ARGS__)   //DPRINT is a macro, debug print
  #define DPRINTLN(...) Serial.println(__VA_ARGS__) //DPRINTLN is a macro, debug print with new line
#else
  #define DPRINT(...)                       //now defines a blank line
  #define DPRINTLN(...)                     //now defines a blank line
#endif

#define FASTLED_INTERNAL      // Stop the annoying messages during compile


//////////////////// LIBRARIES //////////////////////
#include <Adafruit_NeoPixel.h>
#include "HX711.h"
#include "Adafruit_Soundboard.h"


////////////////////// IO SETUP /////////////////////
// NOTE: SFX module connected to Serial1 port (pins 0-1)
#define HX711_DOUT_PIN  2     // Air pressure sensor - Data
#define HX711_SCK_PIN   3     // Air pressure sensor - Clock
#define LED_PIN         6     // Pixel string

#define PIN_SFX_RST     4     // Reset line for SFX


//////////////////// PIXEL SETUP ////////////////////
#define NUM_LEDS      300     
#define BRIGHTNESS    255     // Range 0-255
#define REFRESH_INT   20      // (ms) Refresh interval of pixel strip (because updating them takes FOREVER) (40 = 25Hz)

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

#define COL_BLACK     0x00000000
#define COL_WHITE     0xFF000000
#define COL_DEFAULT   0x00000011
#define COL_RED       0x11FF0000


#define MARK_WIDTH    4       // (pixels) number of LEDs at full brightness at top of lighted section
#define MARK_TAIL     8       // (pixels) width of faded LEDs after MARK
#define DECAY         127     // (0-255) amount by which brightness drops across MARK_TAIL
#define DECAY_COL     65      // 255 / MARK_TAIL

//////////////////// PRESSURE SENSOR SETUP ////////////////////
HX711 airPSensor;

#define NUM_READINGS 10               // Number of readings to take an average from 
int32_t airPReadings[NUM_READINGS];   // Holds air pressure readings

int32_t lastAPReading; 

/* NOTE:
      max value = 8388607
      min value = -8388608
      steady state seems to float between 4673000 - 4676000
*/ 

#define RAW_HIT_THRES 4688000     // Minimum RAW value to trigger a hit
#define RAW_MAX_HIT   4700000
uint32_t raw_maxHit = RAW_MAX_HIT;    // Theoretical max hit value

//////////////////// SOUND MODULE SETUP ////////////////////
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, PIN_SFX_RST);
// NOTE: Connect UG to ground to have the sound board boot into UART mode

// Audio filenames (format should be 8chars, no '.' and then extension)
char Audio_UP[]   = "T01UP000OGG";
char Audio_DOWN[] = "T02DOWN0OGG";
char Audio_BELL[] = "T03BELL0OGG";


// Length of audio files (milliseconds)
#define UP_TIME     250
#define DOWN_TIME   320
#define BELL_TIME   500
#define LAG_TIME    200


//////////////////// MISC VARIABLES ////////////////////
// HAMMER SETTINGS

#define HIT_MIN     800     // minimum force to trigger hit
#define HIT_MAX     3000    // theoretical max hit
// #define WIN_RATE 5

int win_count = 0;
// int count_loss = 0;
int strength_offset = 0;


#define READ_DELAY  500     // Delay from last max reading to trigger action
uint32_t lastMaxTime = 0;   // Time at which last max value was read
uint32_t lastMaxValue = 0;  // Current max reading

uint16_t count_wins = 0;
uint16_t count_hits = 0;



//////////////////////////////////////////////////////////////////////////////////





void setup() 
{
  // LEDS
  // FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // setup LED array
  // FastLED.setMaxPowerInVoltsAndMilliamps(5,5000);      //limit power of LEDs

  leds.begin();               // Initialise Pixels
  leds.show();                // Turn them all off
  leds.setBrightness(BRIGHTNESS);


  leds.fill(COL_DEFAULT, 0, NUM_LEDS);
  leds.show();


  // Setup sound board module
  Serial1.begin(9600);                  // Open serial connection

  if (!sfx.reset())                     // Rest & check it's there
    DPRINTLN("SFX module not found");
  else
    DPRINTLN("SFX module found!");

  // delay(1000);



  // Instantiate air pressure sensor
  airPSensor.begin(HX711_DOUT_PIN, HX711_SCK_PIN);


  // Initialise Serial debug
  #ifdef DEBUG
    Serial.begin(115200);         // Open comms line
    while (!Serial) ;           // Wait for serial port to be available

    Serial.println(F("Wassup?"));
  #endif

  // Fill table of air pressure readings
  // NOTE: 10 reading takes 861ms to do
  // uint8_t readingNum = 0;

  // while (readingNum <= NUM_READINGS)
  // {
  //  if (airPSensor.is_ready())
  //    airPReadings[readingNum++] = airPSensor.read();
  //  delay(100);
  // }

  while (!airPSensor.is_ready());       // Wait until sensor ready
  lastAPReading = airPSensor.read();    // Take reading

}

void loop() 
{

  int32_t airPReading_RAW;              // RAW reading from pressure sensor
  float strength_temp = 0.0;            // Raw strength value
  uint8_t strength = 0;                 // Constrained % Strength

  if (airPSensor.is_ready())
  {
    airPReading_RAW = airPSensor.read();        // Get sensor reading

    if (abs(airPReading_RAW) > 8000000)         // Sensor reads solid high if bumped sometimes
    {                          
      DPRINTLN(F("ERROR - RIDICULOUSLY HIGH READING"));
      return;
    }

    if (airPReading_RAW > lastMaxValue)     // If it's larger than our last reading, save it and reset the timer
    {
      lastMaxValue = airPReading_RAW;
      lastMaxTime = millis();

      if (lastMaxValue > raw_maxHit)        // Update our recorded max value if achieved
        raw_maxHit = lastMaxValue;
    }
    
    if (lastMaxValue > RAW_HIT_THRES)       // Detect hit (If reading is above hit pressure threshold)
    {
      DPRINT(F("Max Air Pressure = "));
      DPRINT(lastMaxValue);
      DPRINT(F("\t Current Air Pressure = "));
      DPRINT(airPReading_RAW);
      DPRINTLN(F("\t HIT!"));
    }

    if (lastMaxTime + READ_DELAY <= millis()) // Find peak pressure of hit by waiting READ_DELAY for new max, then showing the win
    {
      if (lastMaxValue > RAW_HIT_THRES)       // Detect hit (If reading is above hit pressure threshold)
      {

        // Do fun maths to determine how high player achieved
        strength_temp = ((float) (lastMaxValue - RAW_HIT_THRES) / (float) (raw_maxHit - RAW_HIT_THRES)) *100.0;
        DPRINT(F("Strength = "));
        DPRINT(strength_temp, DEC);
        DPRINT(F("\tOffset = "));
        DPRINTLN(strength_offset);


        // Constrain number
        strength = constrain(strength_temp + strength_offset, 15, 100);

        if (strength == 100)
          count_wins++;
        count_hits++;

        // Reset max hit value (it may have gone out of range) if little successes achieved
        if (count_hits > 20)
        {
          if (count_wins <= 1)
          {
            DPRINTLN(F("Resetting maxhit value"));
            raw_maxHit = RAW_MAX_HIT;
            count_wins = 0;
            count_hits = 0;
          }
        }


        // Show the things!
        animate(strength);

        // Throw in some randomness and sneaky maths to ensure people win someties but not too much
        if (strength == 100)    // if we win
          win_count -= 20;      // Make it harder to win
        else                    // if we don't
          win_count += 5;       // Help a little
        if (win_count > 100)    // (value went negative and wrapped around)
          win_count = 0;
        if (win_count >= 70)    // But don't help too much
          win_count = 70;

        strength_offset = random(win_count, win_count + 10);


        // Reset variables
        lastMaxValue = 0;
        lastMaxTime = millis();
      }
    }
  }
}


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
  // #ifdef DEBUG
  //  if (strength == 100)
  //    Serial.print("#WIN\r\n");
  //  else
  //    Serial.print("#HIT\r\n");
  // #endif

  // strength = 100;


  // Variables
  uint16_t  level = floor(NUM_LEDS * 0.01 * strength);    // Pixel height to which marker travels

  uint32_t  startTime;                        // Time at beginning of animation
  uint32_t  timeStage = 0;                                // Theoretical time since start of animation
  uint8_t   refreshRate = floor(UP_TIME / level);         // Period between turning on successive LEDs 
  uint8_t   pauseTime = 0;                                // Time to wait between lighitng LEDs (accounting for processing time)

  uint16_t  currentHeight = 0;                            // Holds number of LEDs to light up for each stage

  // leds.clear();        // Black the things to begin

  ////////////////////////////////
  ////////// ANIMATE UP //////////
  ////////////////////////////////

  /*
    It takes longer to light up LEDs one by one than it does to play file, so let's assume we'll need to skip some frames

    Number of LEDs to light up = % though SFX
    Level * (elapsedTime/total time)
  */ 

  if (! sfx.playTrack(Audio_UP) )
  {
    DPRINTLN("Failed to play track?");
  }

  startTime = millis();
  while ((startTime + UP_TIME) > millis() )
  {
    // Calculate number of LEDs to light up
    currentHeight = level * ((millis() - startTime) / (float) UP_TIME);

    // DPRINT("TempVal = ");
    // DPRINTLN(tempVal);

    for (uint16_t i = 0; i < currentHeight; ++i)
    {
      if (currentHeight - i < MARK_WIDTH)
        leds.setPixelColor(i, COL_WHITE);
      else
        leds.setPixelColor(i, COL_RED);
    }

    leds.show();
  }


  ////////////////////////////////
  ////////// ??WINNER?? //////////
  ////////////////////////////////
  if (strength >= 100) // IF strength = 100, trigger bell animation
  {
    // Serial.println('W');   // Trigger SFX
    if (!sfx.reset()) 
    {
        DPRINTLN("Reset failed");
    }
    if (! sfx.playTrack(Audio_BELL) )
    {
      DPRINTLN("Failed to play track?");
    }
    // pause_ms(100);       // wait for audio file to load

    startTime = millis(); 


    while ((startTime + BELL_TIME) > millis() )
    {
      // Calculate number of LEDs to light up

      // float tempVal = (millis() - startTime) / (float) UP_TIME;
      currentHeight = level * ((millis() - startTime) / (float) BELL_TIME);

      // DPRINT("TempVal = ");
      // DPRINTLN(tempVal);
      for (uint8_t i = 0; i < 10; ++i)
      {
        /* code */
        leds.fill(random(0x00FFFFFF), random(NUM_LEDS-9), 8);
      }

      leds.show();
    }

    leds.fill(COL_DEFAULT);
    leds.show();
  }


  ////////////////////////////////
  ///////// ANIMATE DOWN /////////
  ////////////////////////////////
  else            // Don't animate down for winners
  {
    // pause_ms(BELL_TIME + 200);
    // Serial.println('D'); // Trigger SFX

    if (!sfx.reset()) 
    {
        DPRINTLN("Reset failed");
    }
    if (! sfx.playTrack(Audio_DOWN) )
    {
      DPRINTLN("Failed to play track?");
    }
    // pause_ms(500);       // wait for audio file to load  
    startTime = millis(); 
    timeStage = 0;  


    while ((startTime + DOWN_TIME) > millis() )
    {
      // Calculate number of LEDs to light up

      // float tempVal = (millis() - startTime) / (float) UP_TIME;
      currentHeight = level * (1.0 - (millis() - startTime) / (float) DOWN_TIME);

      // DPRINT("currentHeight = ");
      // DPRINTLN(currentHeight);


      leds.fill(COL_DEFAULT, currentHeight, NUM_LEDS - currentHeight);

    for (uint16_t i = 0; i < currentHeight; ++i)
    {

      if (currentHeight - i < MARK_WIDTH)
        leds.setPixelColor(i, COL_WHITE);
      // else
      //  leds.setPixelColor(i, COL_RED);
    }     


      // for (uint16_t i = 0; i < currentHeight; ++i)
      // {
      //  /* code */
      // }

      leds.show();
    }


    pause_ms(LAG_TIME);


    // // Turn off LEDs from top to bottom of lighted section
    // for (uint16_t i = level; i > 0; --i)
    // {
    //  // leds[i] = CRGB::Black;
    //  leds.setPixelColor(i, COL_BLACK);
      
    //  // Draw MARK segment
    //  if ((i - MARK_WIDTH) >= 0)
    //    // leds[i - MARK_WIDTH] = CRGB::White;
    //    leds.setPixelColor(i - MARK_WIDTH, COL_WHITE);

    //  leds.show();

    //  // Calculate time to wait between frames (to keep in sync with audio)
    //  timeStage += refreshRate;
    //  pauseTime = startTime + timeStage - millis();     

    //  if (pauseTime > 0)
    //    pause_ms(pauseTime);
    // }

    // Serial.println("Hey");
  }

  // Reset air pressure readings ready for next play
  for (uint8_t i = 0; i < NUM_READINGS; ++i)
    airPReadings[i] = 0;

  while (!airPSensor.is_ready());       // Wait until sensor ready
  lastAPReading = airPSensor.read();    // Take reading


  if (!sfx.reset()) 
  {
      DPRINTLN("Reset failed");
  }

  return;
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

