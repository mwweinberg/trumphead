#include "SPI.h"
#include <Adafruit_VS1053.h>
#include <SD.h>

#include <Adafruit_NeoPixel.h>


/*****************************************************************************
Sketch to play music when fluid is detected and change LED colors
Based on Adafruit examples and amazing support.  Thanks!
adapted from https://forums.adafruit.com/viewtopic.php?f=31&t=79525&p=402768&hilit=+music+maker+music+maker+trigger#p402768
for the sensor
-Connect the red wire to +5V, 
-the black wire to common ground 
-and the yellow sensor wire to pin #2
connect the neopixel to pin #5
*****************************************************************************/

#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS    10      // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
#define SHIELD_RESET   -1      // VS1053 reset pin (unused!)
#define SHIELD_CS       7      // VS1053 chip select pin (output)
#define SHIELD_DCS      6      // VS1053 Data/command select pin (output)
#define CARDCS          4      // Card chip select pin
#define DREQ            3      // VS1053 Data request, ideally an Interrupt pin

//++++++++led stuff+++++++++++

#define PIN = 5 //this is the neopixel data pin
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, 5, NEO_RGB + NEO_KHZ800); //  initializes neopixels



//&&&&&&&&&this is sensor stuff&&&&&&&&&&&

uint8_t sensorVal = 0;   // placeholder for the rate calculation

#define FLOWSENSORPIN 2


// count how many pulses!
volatile uint16_t pulses = 0;
// track the state of the pulse pin
volatile uint8_t lastflowpinstate;
// you can try to keep time of how long it is between pulses
volatile uint32_t lastflowratetimer = 0;
// and use that to calculate a flow rate
volatile float flowrate;

SIGNAL(TIMER1_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  
  flowrate /= lastflowratetimer;  // in hertz
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK1 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK1 &= ~_BV(OCIE0A);
  }
}


//&&&&end sensor stuff&&&&&&&&&&&&&&

//+++++++++++led stuff+++++++++++++++
uint8_t ledPin = A4;

//music stuff
char MP3;
char LAST_MP3;

//Initializes the music shield
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);



void setup() {
  Serial.begin(9600);

  //+++++++++++LED stuff +++++++++++++   
  //initialize neopixels
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  //++++++++++++++++++++++++++
  
  Serial.println("Adafruit VS1053 Library Test");

  // initialise the music player
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));

  
 
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");
  
  // list files
  printDirectory(SD.open("/"), 0);
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(50,50);

  if (! musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT))
    Serial.println(F("DREQ pin is not an interrupt pin"));

  //&&&&&&&&&sensor stuff&&&&&&&&&&&&&&&&&&&
  pinMode(FLOWSENSORPIN, INPUT);
  digitalWrite(FLOWSENSORPIN, HIGH);
  lastflowpinstate = digitalRead(FLOWSENSORPIN);
  useInterrupt(true);
  //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

  
  
}


void loop() {

//&&&&&&&&this reads the sensor&&&&&&&&&&&
sensorVal = (flowrate/7.5);
Serial.print("start of loop, sensorVal = ");
Serial.println(sensorVal);
Serial.println();
//&&&&&&&&&&&&&&&&&&&&&&&&&&&




//************************************************
//**************NO FLUID********************
//************************************************
// going to want to make this untriggered state 
if (sensorVal < 7) { //no fluid detected
  Serial.print("No fluid detected.  SensorVal is: ");   
  Serial.println(sensorVal);

  //+++++++++ set pixels to normal color+++++++++++++++
  strip.setPixelColor(0, 255, 69, 0); // turn the leds the color you want. pixel, r, g, b
  strip.show();
  //sensorVal = analogRead(sensorPin);
  //&&&&&&&this delay may be too short&&&&&&&&
  
  delay(500);
} //end of while "no fluid" loop
//************************************************



//************************************************
//**************FLUID DETECTED**********************
//************************************************

if (sensorVal > 6) { //fluid is detected
Serial.print("Fluid detected!  SensorVal is: "); Serial.println(sensorVal);

  //This starts playing a random song
  File path = SD.open("/");
  File results;
  char* MP3 = selectRandomFileFrom( path, results );
  delay(500);
  musicPlayer.startPlayingFile(MP3);
  //++++++++++this changes the LED lights to active
  digitalWrite(ledPin, HIGH);
  Serial.print("Now playing song: "); Serial.println(MP3);


while (musicPlayer.playingMusic) {
  
  strip.setPixelColor(0, 255, 0, 0); // turn the leds the color you want. pixel, r, g, b
  strip.show();
} //end of while playing music loop

Serial.print("Done playing: "); Serial.println(MP3); Serial.println();

} //end of if "door is open" loop
//************************************************


} //end of Void Loop



//****************************************************
/* Helper functions */
//****************************************************


/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}

// Function to select random mp3
char* selectRandomFileFrom( File dir, File result ) {
  File entry;
  int count = 0;

  dir.rewindDirectory();

  while ( entry = dir.openNextFile() ) {
    if ( random( count ) == 0 ) {
      result = entry;
    }
    entry.close();
    count++;
  }
  return result.name();   // returns the randomly selected file name
}



