/*
1. Put Clock into Wifi Configuration mode 

In Wifi Configuration mode the ESP32 is setup to become a Wifi Access Point temporarily so you can upload your Wifi Credentials using a mobile device. The Clock is in Wifi Configuration mode when the blue led on the ESP32 flashes quickly. The procedure for putting the Clock into Wifi Configuration mode is to  

- Attach the USB cable to the Clock for 5 seconds
- Disconnect the USB cable for 2 seconds
- Re-attach the USB cable 

The blue led on the back of the PCB should now flash quickly. If not repeat the procedure.

2. Connect your mobile device to the Clock over Wifi

With the blue led flashing quickly (indicating that the WiFi configuration mode is active) scan the QR code  to connect your phone to the Clock.  

SSID: XXX's WordClock

password: 12345678

3. Enter your home Wifi credentials into the Mobile Device

Then open a web browser and type in http://192.168.1.1. to display the WiFi configuration page. (This configuration page may also popup automagically.)

On the configuration page enter your Wifi Credentials for your local wifi access router. Take care to get the syntax correct.

Press the configure button to save this to the Clock.

4. Return the clock into Clock Mode

When in Clock mode The blue led will flash slowly. The procedure to put the clock into Clock mode is as follows

- Disconnect the USB cable for 12 seconds 
- Attach the USB cable  
- After approx 20 seconds, the Clock should now start and display the correct time.

The Clock should see the power-up sequence results in successfully displaying the time and playing the "Jedi" startup audio clip. 
If there are issues please go back through each of the previous steps and validate each has been followed carefully.

5. Button Menu Overview 

Clock mode:

- Short press the Lower Button to change the color of the foreground.
- Short press the Upper Button to change the color of the background.
- To exit Clock Mode and access Alarm Mode Long Press Both Buttons Simultaneously

Alarm Mode:

- To arm or disarm the Alarm, long-press the Lower button. You will see the word "ON" appear on the Alarm Mode Screen indicating the Alarm is armed.
- To set the Alarm Time you can Increment alarm hour and minutes using Upper and Lower buttons. Note the AM/PM notation on the screen to ensure you get the correct Alarm time.
- Once you are happy with the Alarm time you can exit Alarm mode with a long press of both buttons which will return you to Clock Mode.


*/

//Libraries

#include <WiFi.h>
#include <TimeLib.h>  
#include <pgmspace.h>
#include <EEPROM.h>
#include "ESPAutoWiFiConfig.h"

// WS2182 LED Driver Library Setup
#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 169 // Note: First LED is address 0
#define DATA_PIN 12 // Note: D12 used to control LED chain


// Define the array of leds
CRGB leds[NUM_LEDS];

// Define Led Matrix
// Array with following locations: {  0   ,   1 , 2  3            }
// Array with following Structure: {Value ,   X , Y, LED Colour # }
byte ledmatrix[169][4];

// Populate the LED Matrix with X,Y Coordinates
byte tempctr = 0; // Points to each LED location

byte color = 4 ; // Used to cycle through color effects
byte bkcolor = 2; // Used to set the background colour
byte effectmode = 0; // This denotes the type of refresh effect selected by the user 

byte displaymode = 0; // Display mode used to detemine where in the menu heirarchy 0 = Time, 1 = Set Alarm, 

float legacybrightness = 10; // Used to track the current baseline of output from the LDR
const float alpha = 0.9; // alpha is the % baseline included in changes of ambient light per cycle

/*
color == 0:Green;
color == 1:Red;    
color == 2:Blue;
color == 3:Yellow;                                 
color == 4:White;                  
color == 5:Violet; 
color == 6:Aqua; 
*/

int a1 = 100; // Duration in microseconds that LEDs on during callibration
int tdelay = 1;// LED Powerup Sequence delay

const char* TZ_INFO = "	SGT-8";
const char* ntpServer = "uk.pool.ntp.org";

// ***** CUSTOM VARIABLES below used to personalise the Clock *********

byte birthday = 24; // Birthday "Day" of individual owning the clock Note: [1,31]
byte birthmonth = 11; // Birthday "Month" of individual owning the clock Note: [0,11]

// ***** CUSTOM VARIABLES above used to personalise the Clock *********


byte clockhour; // Holds current hour value
byte clockminute; // Holds current minute value

byte clockday; // Holds current day value // day of month [1,31]
byte clockmonth; // Holds current month value // month of year [0,11]

byte alarmhour = 6; // Holds Alarm hour value
byte alarmminute = 0; // Holds Alarm minute value

boolean alarmstatus = false; // Start with Alarm Disabled  

unsigned long runTime = 0;

// Test routine LED display

 int testroutine[169] = {12  , 13  , 38  , 39  , 64  , 65  , 90  , 91  , 116 , 117 , 142 , 143 , 168 , 167 , 166 , 165 , 164 , 163 , 162 , 161 , 160 , 159 , 158 , 157 , 156 ,   
155 , 130 , 129 , 104 , 103 , 78  , 77  , 52  , 51  , 26  , 25  , 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 14  , 37  , 40  , 63  , 66  , 89  , 92  , 115 , 118 , 141 , 144 ,       
145 , 146 , 147 , 148 , 149 , 150 , 151 , 152 , 153 , 154 , 131 , 128 , 105 , 102 , 79  , 76  , 53  , 50  , 27  , 24 , 23  , 22  , 21  , 20  , 19  , 18  , 17  , 16  , 15  ,               
36  , 41  , 62  , 67  , 88  , 93  , 114 , 119 , 140 , 139 , 138 , 137 , 136 , 135 , 134 , 133 , 132 , 127 , 106 , 101 , 80  , 75  , 54  , 49 , 28 , 29  , 30  , 31  , 32 , 33  , 34  , 35  ,                       
42  , 61  , 68  , 87  , 94  , 113 , 120 , 121 , 122 , 123 , 124 , 125 , 126 , 107 , 100 , 81 , 74 , 55 , 48 , 47  , 46  , 45  , 44  , 43 , 60  , 69  , 86  , 95  , 112 ,                               
111 , 110 , 109 , 108 , 99  , 82 , 73 , 56 , 57 , 58 , 59 , 70  , 85  , 96  , 97  , 98  , 72  , 83 , 71 , 84 };

// Time Refresh counter 
int rfcvalue = 90000; // Refresh the time approximately once every 60 seconds
int rfc = 89000; // Force a redraw of screen at the beginning of startup then refresh every 60 seconds

int ledPin = 2; // for  ESP32 DOIT Dev Kit 1 - onboard Led connected to GPIO2
bool highForLedOn = true; // need to make output high to turn GPIO2 Led ON
size_t eepromOffset = 0; // if you use EEPROM.begin(size) in your code add the size here so AutoWiFi data is written after your data


void setup() {

Serial.begin(115200); // Setupserial interface for test data outputs

// Setup LDR Sensor pin D13 connected to LDR in series with 10k resiter to earth

pinMode(32, OUTPUT); // This pin used to mute the audio when not in use via pin 5 of PAM8403
pinMode(25, OUTPUT ); // Audio output pin

digitalWrite(32,LOW);   // MUTE sound to speaker via pin 5 of PAM8403

  analogReadResolution(10);

// WS2182 LED Driver Setup
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS); // Default if RGB for this however may vary dependent on LED manufacturer
  LEDS.setBrightness(20); //Set brightness of LEDs here
  // limit my draw to 1A at 5v of power draw
  FastLED.setMaxPowerInVoltsAndMilliamps(5,500); // Limits to 200mA to avoid overcurrent on PCB tracks

  FastLED.setDither(0); // Turns off Auto Dithering function to remove flicker



// Setup Buttons

 pinMode(26, INPUT_PULLUP); // Upper Button
 pinMode(27, INPUT_PULLUP); // Lower Button

// Pre-load the Matrix with colour and coordinate values
      
      for (byte x = 1; x < 14; x++) {

         for (byte y = 1; y < 14; y++) {

              if ( x == 1 ) { // First Column
                tempctr = (14 - y);                
              } else 
              if ( x == 2 ) { // First Column
                tempctr = (13 + y);               
              } else              
              if ( x == 3 ) { // First Column
                tempctr = (40 - y);               
              } else               
              if ( x == 4 ) { // First Column
                tempctr = (39 + y);                
              } else  
              if ( x == 5 ) { // First Column
                tempctr = (66 - y);                
              } else 
              if ( x == 6 ) { // First Column
                tempctr = (65 + y);               
              } else              
              if ( x == 7 ) { // First Column
                tempctr = (92 - y);               
              } else               
              if ( x == 8 ) { // First Column
                tempctr = (91 + y);                
              } else
              if ( x == 9 ) { // First Column
                tempctr = (118 - y);                
              } else 
              if ( x == 10 ) { // First Column
                tempctr = (117 + y);               
              } else              
              if ( x == 11 ) { // First Column
                tempctr = (144 - y);               
              } else               
              if ( x == 12 ) { // First Column
                tempctr = (143 + y);                
              } else
              if ( x == 13 ) { // First Column
                tempctr = (170 - y);               
              }
            // Poke the coordinates into the array for each LED in the Matrix

            ledmatrix[tempctr][0] = 0;
            ledmatrix[tempctr][1] = x;
            ledmatrix[tempctr][2] = y;                        
            ledmatrix[tempctr][3] = 0;          // Load Green as first colour  

            /*
                Serial.print(tempctr);
                Serial.print(" x: ");
                Serial.print(x);
                Serial.print(" y: ");
                Serial.println(y);
            */                      
         }
        
      }
  
/*
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
 */

  if (ESPAutoWiFiConfigSetup(ledPin, highForLedOn, eepromOffset)) { 
    return; // in config mode so skip rest of setup
  }


  
  //init and get the time
  configTime(0, 0, ntpServer);
  setenv("TZ", TZ_INFO, 1);
  tzset();
  printLocalTime();

 Serial.print(clockhour);
 Serial.print(" : "); 
 Serial.println(clockminute);

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

 StartupLedTest();



}

// ******************************************************************************

void loop() {

// Check Wifi Status
  if (ESPAutoWiFiConfigLoop()) {  // handle WiFi config webpages
    return;  // skip the rest of the loop until config finished
  }

  // testdisplay(); // Display the test display
  
  if (displaymode == 0) { // ************************** Clock Display Mode *************************
    refreshtime(); // Refresh the clock time variable from RTC
    changelettercolor(); // Check if upper button pushed to change letter color
    changebkcolor(); // Check if lower button pushed to change background color
    checkforalarmmenu(); // Check to see if the Alarm Mode is selected

    // Check Alarm Status
    if (alarmstatus == true) {
      if ((alarmminute == clockminute) && (alarmhour == clockhour)) {
        // Call the Alarm Routine
        alarmroutine(); // Plays the Alarm for 60 Seconds the resets or stops if a button is pressed
      }
    }

  }  
  
  if (displaymode == 1) { // ************************** Set Alarm Mode *************************

    // Enter the Set Alarm Menu by a long press on both buttons
    UpdateAlarmDisplay(); // Clears the Array and sets up and displays the Alarm Mode Screen

    /*** In setup Mode check for Wifi Credential Changes
    if (ESPAutoWiFiConfigSetup(ledPin, highForLedOn, eepromOffset)) { 
      return; // in config mode so skip rest of setup
    }
    //***/
    
    incrementalarmhour(); // Push Upper Button to increment Alarm hour
    incrementalarmminute(); // Short Press of Lower Button to increment Alarm minute, followed by Long Press to Arm or DisArm the Alarm   
    checkforalarmmenu();    // Check for both buttons long press to exit the Alarm Mode
  }

}

// ******************************************************************************

void checkforalarmmenu() { 
    //Upper and Lower Button Pushed for 2 seconds Toggle Alarm set mode - 0 = Clock Mode, 1 = Alarm Mode
  if((digitalRead(26) == LOW)&&(digitalRead(27) == LOW)){
      delay(2000); // Debounce by waiting then checking again
      if((digitalRead(26) == LOW)&&(digitalRead(27) == LOW)){
      // Toggle Alarm Set Mode
      if (displaymode == 0){  
          displaymode = 1;
          Serial.print("displaymode = ");
          Serial.println(displaymode);
          AL();
          alarmon();         
          UpdateAlarmDisplay(); // Clears the Array and sets up and displays the Alarm Mode Screen
      } else 
      if (displaymode == 1){ 
          displaymode = 0;            
          Serial.print("displaymode = ");
          Serial.println(displaymode); 
          dAL();
          dAM();
          dPM();
          dalarmon();             
          color = 4 ; 
          bkcolor = 2;   
          refreshtime(); // Refresh the clock time variable from RTC

      }
    }
  delay(500);
  }
}

void incrementalarmhour() { // Upper Button will increment the Alarm Hour
    //Upper Button Pushed
  if((digitalRead(27) == LOW)&&(digitalRead(26) == HIGH)){
      delay(200); // Debounce by waiting then checking again
      if((digitalRead(27) == LOW)&&(digitalRead(26) == HIGH)){

      // Increment the color variable
      alarmhour++;
      if(alarmhour == 24){
        alarmhour = 0;                
      }
      UpdateAlarmDisplay();
      Serial.print("Alarm Hour = ");
      Serial.println(alarmhour);
      Serial.print("Alarm Minute = ");
      Serial.println(alarmminute);
      Serial.print("Alarm Status = ");
      Serial.println(alarmstatus);
   }

  delay(500);
  }
}

void incrementalarmminute() { // Upper Button will increment the Alarm Hour
    //Lower Button Pushed
  if((digitalRead(26) == LOW)&&(digitalRead(27) == HIGH)){
      delay(200); // Debounce by waiting then checking again
    if((digitalRead(26) == LOW)&&(digitalRead(27) == HIGH)){
      // Increment the color variable
      alarmminute = alarmminute + 5; // Increment in 5min blocks
      if(alarmminute == 60){
        alarmminute = 0;  
      }
          delay(200); 
          if((digitalRead(26) == LOW)&&(digitalRead(27) == HIGH)){ 
            delay(1000);
            if((digitalRead(26) == LOW)&&(digitalRead(27) == HIGH)){   
                // Toggle the Alarm Arm/Disarm variable if LOWER Button held for 1 second
                 alarmstatus = !alarmstatus; // Toggle the Alarm Arm/Disarm variable  
                 color = 4 ; 
                 bkcolor = 2;        
                 UpdateAlarmDisplay();
             }
           }


      delay(200); 
      UpdateAlarmDisplay();
      Serial.print("Alarm Minute = ");
      Serial.println(alarmminute);
      Serial.print("Alarm Hour = ");
      Serial.println(alarmhour);
      Serial.print("Alarm Status = ");
      Serial.println(alarmstatus);
      
    }

  delay(500);
  }
}


void changelettercolor() { 
    //Lower Button Pushed
  if((digitalRead(26) == LOW)&&(digitalRead(27) == HIGH)){
      delay(200); // Debounce by waiting then checking again
      if((digitalRead(26) == LOW)&&(digitalRead(27) == HIGH)){
           // Increment the color variable
           color++; 
           if (color > 6) {
             color = 0;
            }     
        UpdateDisplay0();
      }
  delay(500);
  }
}

void changebkcolor() {
    //Upper Button Pushed
  if((digitalRead(27) == LOW)&&(digitalRead(26) == HIGH)){
      delay(200); // Debounce by waiting then checking again
      if((digitalRead(27) == LOW)&&(digitalRead(26) == HIGH)){

      // Increment the color variable
      bkcolor++; 
      if (bkcolor > 6) {
          bkcolor = 0;
        }            
      UpdateDisplay0();
      }

  delay(500);
  }
}


void UpdateAlarmDisplay() { // Step through each location in the Matrix Display Array Data in LED Matrix

// Clear the display

// Blank display
fill_solid( leds, 169, CRGB(0,0,0)); 

// firstly clear Array
   for (int z = 0; z < 169; z++) {

        ledmatrix[z][0] = 0;
   }
   
   clockname(); // Always Display Clock Name

   AL(); // Display the Alarm Mode Letters at all time

   // Display wether Alarm time is AM or PM

   if (alarmhour >= 12) {
    dAM(); // Delete AM display
    PM(); // Show PM display
   } else {
    dPM(); // Delete PM display
    AM(); // Show AM display    
   }

   UpdateAlarm(); // Update the display array with the current Alarm Time - alamminute, alarmhour 

   if (alarmstatus == false) {
      dalarmon(); // Indicates wether Alarm is not set   
   } else {
      alarmon(); // Indicates wether Alarm is set
   }
   

   for (int l = 0; l < 169; l++) {

        if ( ledmatrix[l][0] == 1) {
           leds[l] = CRGB::DarkSlateGray;     
        } else {
           leds[l] = CRGB::Black;        
        }


   }  

   FastLED.show();
}

void UpdateDisplay0() { // Step through each location in the Matrix Display Array Data in LED Matrix

   clockname(); // Always Display Clock Name

   for (int l = 0; l < 169; l++) {

        if ( ledmatrix[l][0] == 1) {

                if ( color == 0) {

                  leds[l] = CRGB::FairyLightNCC; 
              
                } else if ( color == 1) {

                  leds[l] = CRGB::Red;    

                 } else if ( color == 2) {

                  leds[l] = CRGB::Green; 

                 } else if ( color == 3) {

                  leds[l] = CRGB::Yellow;                                 

                } else if ( color == 4) {

                  leds[l] = CRGB::White; 

                 } else if ( color == 5) {

                  leds[l] = CRGB::Violet; 
                  
                 }else if ( color == 6) {

                  leds[l] = CRGB::Blue; 
                  
                 }               
        } else {
                if ( bkcolor == 0) {

                  leds[l] = CRGB::DarkViolet; 
              
                } else if ( bkcolor == 1) {

                  leds[l] = CRGB::DarkGray;    

                 } else if ( bkcolor == 2) {

                  leds[l] = CRGB::DarkBlue; 

                 } else if ( bkcolor == 3) {

                  leds[l] = CRGB::DarkGreen;                                 

                } else if ( bkcolor == 4) {

                  leds[l] = CRGB::DarkOrange; 

                 } else if ( bkcolor == 5) {

                  leds[l] = CRGB::DarkSlateGray; 
                  
                 }else if ( bkcolor == 6) {

                  leds[l] = CRGB::Black; 
                  
                 }    



      
      }        
//   changelettercolor(); // Check if upper button pushed to change letter color
//   changebkcolor(); // Check if lower button pushed to change background color

//  dimdisplay(); // Read and set the brightness of the LEDs 
   }  
   dimdisplay(); // Read and set the brightness of the LEDs
   FastLED.show();

/*
 * HTMLColorCode {
  AliceBlue =0xF0F8FF, Amethyst =0x9966CC, AntiqueWhite =0xFAEBD7, Aqua =0x00FFFF,
  Aquamarine =0x7FFFD4, Azure =0xF0FFFF, Beige =0xF5F5DC, Bisque =0xFFE4C4,
  Black =0x000000, BlanchedAlmond =0xFFEBCD, Blue =0x0000FF, BlueViolet =0x8A2BE2,
  Brown =0xA52A2A, BurlyWood =0xDEB887, CadetBlue =0x5F9EA0, Chartreuse =0x7FFF00,
  Chocolate =0xD2691E, Coral =0xFF7F50, CornflowerBlue =0x6495ED, Cornsilk =0xFFF8DC,
  Crimson =0xDC143C, Cyan =0x00FFFF, DarkBlue =0x00008B, DarkCyan =0x008B8B,
  DarkGoldenrod =0xB8860B, DarkGray =0xA9A9A9, DarkGrey =0xA9A9A9, DarkGreen =0x006400,
  DarkKhaki =0xBDB76B, DarkMagenta =0x8B008B, DarkOliveGreen =0x556B2F, DarkOrange =0xFF8C00,
  DarkOrchid =0x9932CC, DarkRed =0x8B0000, DarkSalmon =0xE9967A, DarkSeaGreen =0x8FBC8F,
  DarkSlateBlue =0x483D8B, DarkSlateGray =0x2F4F4F, DarkSlateGrey =0x2F4F4F, DarkTurquoise =0x00CED1,
  DarkViolet =0x9400D3, DeepPink =0xFF1493, DeepSkyBlue =0x00BFFF, DimGray =0x696969,
  DimGrey =0x696969, DodgerBlue =0x1E90FF, FireBrick =0xB22222, FloralWhite =0xFFFAF0,
  ForestGreen =0x228B22, Fuchsia =0xFF00FF, Gainsboro =0xDCDCDC, GhostWhite =0xF8F8FF,
  Gold =0xFFD700, Goldenrod =0xDAA520, Gray =0x808080, Grey =0x808080,
  Green =0x008000, GreenYellow =0xADFF2F, Honeydew =0xF0FFF0, HotPink =0xFF69B4,
  IndianRed =0xCD5C5C, Indigo =0x4B0082, Ivory =0xFFFFF0, Khaki =0xF0E68C,
  Lavender =0xE6E6FA, LavenderBlush =0xFFF0F5, LawnGreen =0x7CFC00, LemonChiffon =0xFFFACD,
  LightBlue =0xADD8E6, LightCoral =0xF08080, LightCyan =0xE0FFFF, LightGoldenrodYellow =0xFAFAD2,
  LightGreen =0x90EE90, LightGrey =0xD3D3D3, LightPink =0xFFB6C1, LightSalmon =0xFFA07A,
  LightSeaGreen =0x20B2AA, LightSkyBlue =0x87CEFA, LightSlateGray =0x778899, LightSlateGrey =0x778899,
  LightSteelBlue =0xB0C4DE, LightYellow =0xFFFFE0, Lime =0x00FF00, LimeGreen =0x32CD32,
  Linen =0xFAF0E6, Magenta =0xFF00FF, Maroon =0x800000, MediumAquamarine =0x66CDAA,
  MediumBlue =0x0000CD, MediumOrchid =0xBA55D3, MediumPurple =0x9370DB, MediumSeaGreen =0x3CB371,
  MediumSlateBlue =0x7B68EE, MediumSpringGreen =0x00FA9A, MediumTurquoise =0x48D1CC, MediumVioletRed =0xC71585,
  MidnightBlue =0x191970, MintCream =0xF5FFFA, MistyRose =0xFFE4E1, Moccasin =0xFFE4B5,
  NavajoWhite =0xFFDEAD, Navy =0x000080, OldLace =0xFDF5E6, Olive =0x808000,
  OliveDrab =0x6B8E23, Orange =0xFFA500, OrangeRed =0xFF4500, Orchid =0xDA70D6,
  PaleGoldenrod =0xEEE8AA, PaleGreen =0x98FB98, PaleTurquoise =0xAFEEEE, PaleVioletRed =0xDB7093,
  PapayaWhip =0xFFEFD5, PeachPuff =0xFFDAB9, Peru =0xCD853F, Pink =0xFFC0CB,
  Plaid =0xCC5533, Plum =0xDDA0DD, PowderBlue =0xB0E0E6, Purple =0x800080,
  Red =0xFF0000, RosyBrown =0xBC8F8F, RoyalBlue =0x4169E1, SaddleBrown =0x8B4513,
  Salmon =0xFA8072, SandyBrown =0xF4A460, SeaGreen =0x2E8B57, Seashell =0xFFF5EE,
  Sienna =0xA0522D, Silver =0xC0C0C0, SkyBlue =0x87CEEB, SlateBlue =0x6A5ACD,
  SlateGray =0x708090, SlateGrey =0x708090, Snow =0xFFFAFA, SpringGreen =0x00FF7F,
  SteelBlue =0x4682B4, Tan =0xD2B48C, Teal =0x008080, Thistle =0xD8BFD8,
  Tomato =0xFF6347, Turquoise =0x40E0D0, Violet =0xEE82EE, Wheat =0xF5DEB3,
  White =0xFFFFFF, WhiteSmoke =0xF5F5F5, Yellow =0xFFFF00, YellowGreen =0x9ACD32,
  FairyLight =0xFFE42D, FairyLightNCC =0xFF9D2A
}
 */

}


void clearclockscreen(){

// Blank display
fill_solid( leds, 169, CRGB(0,0,0));
clockname(); // Always Display Clock Name


FastLED.show(); 
}




void ledtestBLUE() { // Powerup sequence for unit

  // Flash LED to show calibrate done

      for (int x = 168; x > -1; x--) {
        leds[x] = CRGB::Blue;
        FastLED.show();
        delay(tdelay); 

        leds[x] = CRGB::Black;
        FastLED.show();
        delay(tdelay);
                    
      }
}

void StartupLedTest() { // Powerup sequence for unit

  // Flash LED to show calibrate done

fill_solid( leds, 169, CRGB::Blue);
FastLED.show(); 


      for (int x = 0; x < 169; x++) {
       
        
        leds[testroutine[x]] = CRGB::White;
         if(x<168){
          leds[testroutine[x+1]] = CRGB::White;
         }        

        FastLED.show();
        delay(8); 

        leds[testroutine[x]] = CRGB::Black;
        FastLED.show();
        delay(10);
                    
      }

}

void ledtestGREEN() { // Powerup sequence for unit

  // Flash LED to show calibrate done

      for (int x = 168; x > -1; x--) {
        leds[x] = CRGB::Green;
        FastLED.show();
        delay(tdelay); 

        leds[x] = CRGB::Black;
        FastLED.show();
        delay(tdelay);
                    
      }

}

void ledtestWHITE() { // Powerup sequence for unit

  // Flash LED to show calibrate done

      for (byte x = 1; x < 14; x++) {

         for (byte y = 1; y < 14; y++) {

              if ( x == 1 ) { // First Column
                tempctr = (14 - y);                
              } else 
              if ( x == 2 ) { // First Column
                tempctr = (13 + y);               
              } else              
              if ( x == 3 ) { // First Column
                tempctr = (40 - y);               
              } else               
              if ( x == 4 ) { // First Column
                tempctr = (39 + y);                
              } else  
              if ( x == 5 ) { // First Column
                tempctr = (66 - y);                
              } else 
              if ( x == 6 ) { // First Column
                tempctr = (65 + y);               
              } else              
              if ( x == 7 ) { // First Column
                tempctr = (92 - y);               
              } else               
              if ( x == 8 ) { // First Column
                tempctr = (91 + y);                
              } else
              if ( x == 9 ) { // First Column
                tempctr = (118 - y);                
              } else 
              if ( x == 10 ) { // First Column
                tempctr = (117 + y);               
              } else              
              if ( x == 11 ) { // First Column
                tempctr = (144 - y);               
              } else               
              if ( x == 12 ) { // First Column
                tempctr = (143 + y);                
              } else
              if ( x == 13 ) { // First Column
                tempctr = (170 - y);               
              }
            // Poke the coordinates into the array for each LED in the Matrix


        Serial.println(tempctr);
        leds[tempctr] = CRGB::White;
        FastLED.show();
        delay(200); 
        leds[tempctr] = CRGB::Black;
        FastLED.show();                     
        delay(200); 
         }
        
      }


/*


      for (int x = 0; x < 169; x++) {
        leds[x] = CRGB::White;
        FastLED.show();
        delay(tdelay); 

        leds[x] = CRGB::Black;
        FastLED.show();
        delay(tdelay);
                    
      }
*/
}


/*
      
      for (int x = 0; x < 169; x++) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(tdelay); 
      }

      for (int x = 8; x > -1; x--) {
        leds[x] = CRGB::Green;
        FastLED.show();
        delay(tdelay);     
      }
      for (int x = 8; x > -1; x--) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(tdelay); 
      }
      for (int x = 0; x < 9; x++) {
        leds[x] = CRGB::Red;
        FastLED.show();
        delay(tdelay);     
      }
      for (int x = 0; x < 9; x++) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(tdelay); 
      }
*/

// DISPLAY ROUTINES TO TURN ON LEDS

void dAL() { //120, 121
  // "AL" Alarm time

    ledmatrix[120][0] = 0; 
    ledmatrix[121][0] = 0; 
}

void AL() { //120, 121
  // "AL" Alarm time

    ledmatrix[120][0] = 1; 
    ledmatrix[121][0] = 1; 
}


void dAM() { //120, 121
  // "AL" Alarm time

    ledmatrix[15][0] = 0; 
    ledmatrix[35][0] = 0; 
}

void AM() { //120, 121
  // "AL" Alarm time

    ledmatrix[15][0] = 1; 
    ledmatrix[35][0] = 1; 
}



void dPM() { //120, 121
  // "AL" Alarm time

    ledmatrix[35][0] = 0; 
    ledmatrix[41][0] = 0; 
}

void PM() { //120, 121
  // "AM" Alarm time

    ledmatrix[35][0] = 1; 
    ledmatrix[41][0] = 1; 
}

void dalarmon() { //87 94
  // "on"

    ledmatrix[87][0] = 0; 
    ledmatrix[94][0] = 0; 
}

void alarmon() { //87 94
  // "on"

    ledmatrix[87][0] = 1; 
    ledmatrix[94][0] = 1; 
}

void doclock() { //103 104 129 130 155 156
  // "oclock"

    ledmatrix[103][0] = 0; 
    ledmatrix[104][0] = 0; 
    ledmatrix[129][0] = 0; 
    ledmatrix[130][0] = 0; 
    ledmatrix[155][0] = 0; 
    ledmatrix[156][0] = 0; 
        
/*   
    leds[50] = CRGB::Black;
    leds[60] = CRGB::Black;
    leds[70] = CRGB::Black;
    leds[80] = CRGB::Black; 
    leds[90] = CRGB::Black; 
    leds[100] = CRGB::Black;        
      FastLED.show();
*/

}

void oclock() { //103  104 129 130 155 156
  // "oclock"
    ledmatrix[103][0] = 1; 
    ledmatrix[104][0] = 1; 
    ledmatrix[129][0] = 1; 
    ledmatrix[130][0] = 1; 
    ledmatrix[155][0] = 1; 
    ledmatrix[156][0] = 1;  
}


void dtwelve() { // 0  25  26  51  52  77
  // "twelve"

    ledmatrix[0][0] = 0; 
    ledmatrix[25][0] = 0; 
    ledmatrix[26][0] = 0; 
    ledmatrix[51][0] = 0; 
    ledmatrix[52][0] = 0; 
    ledmatrix[77][0] = 0; 
        
}

void twelve() { // 0  25  26  51  52  77
  // "twelve"
    ledmatrix[0][0] = 1; 
    ledmatrix[25][0] = 1; 
    ledmatrix[26][0] = 1; 
    ledmatrix[51][0] = 1; 
    ledmatrix[52][0] = 1; 
    ledmatrix[77][0] = 1; 
}

void deleven() { // 102  105 128 131 154 157
  // "eleven"
    ledmatrix[102][0] = 0; 
    ledmatrix[105][0] = 0; 
    ledmatrix[128][0] = 0; 
    ledmatrix[131][0] = 0; 
    ledmatrix[154][0] = 0; 
    ledmatrix[157][0] = 0; 
}

void eleven() { // 102  105 128 131 154 157
  // "eleven"
    ledmatrix[102][0] = 1; 
    ledmatrix[105][0] = 1; 
    ledmatrix[128][0] = 1; 
    ledmatrix[131][0] = 1; 
    ledmatrix[154][0] = 1; 
    ledmatrix[157][0] = 1; 
}

void dten() { // 53 76  79
  // "ten"

    ledmatrix[53][0] = 0; 
    ledmatrix[76][0] = 0; 
    ledmatrix[79][0] = 0; 
    
}

void ten() { // 53 76  79
  // "ten"
    ledmatrix[53][0] = 1; 
    ledmatrix[76][0] = 1; 
    ledmatrix[79][0] = 1;  
}

void dnine() { // 1  24  27  50
  // "nine"
    ledmatrix[1][0] = 0; 
    ledmatrix[24][0] = 0; 
    ledmatrix[27][0] = 0; 
    ledmatrix[50][0] = 0;     
}

void nine() { // 1  24  27  50
  // "eleven"
    ledmatrix[1][0] = 1; 
    ledmatrix[24][0] = 1; 
    ledmatrix[27][0] = 1; 
    ledmatrix[50][0] = 1; 
}


void deight() { // 106 127 132 153 158
  // "eight"
    ledmatrix[106][0] = 0; 
    ledmatrix[127][0] = 0; 
    ledmatrix[132][0] = 0; 
    ledmatrix[153][0] = 0; 
    ledmatrix[158][0] = 0;  
}

void eight() { // 106  127 132 153 158
  // "eight"
    ledmatrix[106][0] = 1; 
    ledmatrix[127][0] = 1; 
    ledmatrix[132][0] = 1; 
    ledmatrix[153][0] = 1; 
    ledmatrix[158][0] = 1; 
}


void dseven() { // 49 54  75  80  101
  // "seven"
    ledmatrix[49][0] = 0; 
    ledmatrix[54][0] = 0; 
    ledmatrix[75][0] = 0; 
    ledmatrix[80][0] = 0;  
    ledmatrix[101][0] = 0; 
}
void seven() { // 49 54  75  80  101
  // "seven"
    ledmatrix[49][0] = 1; 
    ledmatrix[54][0] = 1; 
    ledmatrix[75][0] = 1; 
    ledmatrix[80][0] = 1;  
    ledmatrix[101][0] = 1; 
}

void dsix() { // 2  23  28
  // "six"
    ledmatrix[2][0] = 0; 
    ledmatrix[23][0] = 0; 
    ledmatrix[28][0] = 0; 
}

void six() { // 2  23  28
  // "six"
    ledmatrix[2][0] = 1; 
    ledmatrix[23][0] = 1; 
    ledmatrix[28][0] = 1; 
}

void dfive() { // 100 107 126 133
  // "five"
    ledmatrix[100][0] = 0; 
    ledmatrix[107][0] = 0; 
    ledmatrix[126][0] = 0; 
    ledmatrix[133][0] = 0; 
}

void five() { // 100  107 126 133
  // "five"
    ledmatrix[100][0] = 1; 
    ledmatrix[107][0] = 1; 
    ledmatrix[126][0] = 1; 
    ledmatrix[133][0] = 1;
}

void dfour() { // 48  55  74  81
  // "four"
    ledmatrix[48][0] = 0; 
    ledmatrix[55][0] = 0; 
    ledmatrix[74][0] = 0; 
    ledmatrix[81][0] = 0;
}

void four() { // 48  55  74  81
  // "four"
    ledmatrix[48][0] = 1; 
    ledmatrix[55][0] = 1; 
    ledmatrix[74][0] = 1; 
    ledmatrix[81][0] = 1;
}

void dthree() { // 108 125 134 151 160
  // "three"
    ledmatrix[108][0] = 0; 
    ledmatrix[125][0] = 0; 
    ledmatrix[134][0] = 0; 
    ledmatrix[151][0] = 0;  
    ledmatrix[160][0] = 0; 
}

void three() {
  // "three"
    ledmatrix[108][0] = 1; 
    ledmatrix[125][0] = 1; 
    ledmatrix[134][0] = 1; 
    ledmatrix[151][0] = 1;  
    ledmatrix[160][0] = 1;  
}

void dtwo() { // 3 22  29
  // "two"
    ledmatrix[3][0] = 0; 
    ledmatrix[22][0] = 0; 
    ledmatrix[29][0] = 0; 
}

void two() { // 3  22  29
  // "two"
    ledmatrix[3][0] = 1; 
    ledmatrix[22][0] = 1; 
    ledmatrix[29][0] = 1; 
}

void done() { // 73 82  99
  // "one"
    ledmatrix[73][0] = 0; 
    ledmatrix[82][0] = 0; 
    ledmatrix[99][0] = 0; 
}

void one() { // 73  82  99
  // "one"
    ledmatrix[73][0] = 1; 
    ledmatrix[82][0] = 1; 
    ledmatrix[99][0] = 1; 
}

void dpast() { // 4  21  30  47
  // "past"
    ledmatrix[4][0] = 0; 
    ledmatrix[21][0] = 0; 
    ledmatrix[30][0] = 0; 
    ledmatrix[47][0] = 0; 
}

void past() { // 4 21  30  47
  // "past"
    ledmatrix[4][0] = 1; 
    ledmatrix[21][0] = 1; 
    ledmatrix[30][0] = 1; 
    ledmatrix[47][0] = 1; 
}

void dTo() { // 72  83
  // "To"
    ledmatrix[72][0] = 0; 
    ledmatrix[83][0] = 0; 
}

void To() { // 72 83
  // "To"
    ledmatrix[72][0] = 1; 
    ledmatrix[83][0] = 1; 
}

void dfivemins() { // 5  20  31  46
  // "five"
    ledmatrix[5][0] = 0; 
    ledmatrix[20][0] = 0; 
    ledmatrix[31][0] = 0; 
    ledmatrix[46][0] = 0; 
}

void fivemins() { // 5 20  31  46
  // "five"
    ledmatrix[5][0] = 1; 
    ledmatrix[20][0] = 1; 
    ledmatrix[31][0] = 1; 
    ledmatrix[46][0] = 1; 
}

void dtwenty() { // 97 110 123 136 149 162
  // "twenty"
    ledmatrix[97][0] = 0; 
    ledmatrix[110][0] = 0; 
    ledmatrix[123][0] = 0; 
    ledmatrix[136][0] = 0;  
    ledmatrix[149][0] = 0; 
    ledmatrix[162][0] = 0; 
}

void twenty() { // 97 110 123 136 149 162
  // "twenty"
    ledmatrix[97][0] = 1; 
    ledmatrix[110][0] = 1; 
    ledmatrix[123][0] = 1; 
    ledmatrix[136][0] = 1;  
    ledmatrix[149][0] = 1; 
    ledmatrix[162][0] = 1;  
}

void dtwentyfive() {
  // "twenty"
    ledmatrix[97][0] = 0; 
    ledmatrix[110][0] = 0; 
    ledmatrix[123][0] = 0; 
    ledmatrix[136][0] = 0;  
    ledmatrix[149][0] = 0; 
    ledmatrix[162][0] = 0;
    ledmatrix[5][0] = 0; 
    ledmatrix[20][0] = 0; 
    ledmatrix[31][0] = 0; 
    ledmatrix[46][0] = 0;      

}

void twentyfive() {
  // "twenty"
    ledmatrix[97][0] = 1; 
    ledmatrix[110][0] = 1; 
    ledmatrix[123][0] = 1; 
    ledmatrix[136][0] = 1;  
    ledmatrix[149][0] = 1; 
    ledmatrix[162][0] = 1;
    ledmatrix[5][0] = 1; 
    ledmatrix[20][0] = 1; 
    ledmatrix[31][0] = 1; 
    ledmatrix[46][0] = 1;    
}

void dquarter() { // 6 19  32  45  58  71  84
  // "quarter"
    ledmatrix[6][0] = 0; 
    ledmatrix[19][0] = 0; 
    ledmatrix[32][0] = 0; 
    ledmatrix[45][0] = 0;  
    ledmatrix[58][0] = 0; 
    ledmatrix[71][0] = 0; 
    ledmatrix[84][0] = 0; 
}

void quarter() {
  // "quarter"
    ledmatrix[6][0] = 1; 
    ledmatrix[19][0] = 1; 
    ledmatrix[32][0] = 1; 
    ledmatrix[45][0] = 1;  
    ledmatrix[58][0] = 1; 
    ledmatrix[71][0] = 1; 
    ledmatrix[84][0] = 1;  
}

void dtenmins() { // 137 148 163
  // "one"
    ledmatrix[137][0] = 0; 
    ledmatrix[148][0] = 0; 
    ledmatrix[163][0] = 0; 
}

void tenmins() { // 137 148 163
  // "one"
    ledmatrix[137][0] = 1; 
    ledmatrix[148][0] = 1; 
    ledmatrix[163][0] = 1; 
}

void dhalf() { // 85  96  111 122
  // "half"
    ledmatrix[85][0] = 0; 
    ledmatrix[96][0] = 0; 
    ledmatrix[111][0] = 0; 
    ledmatrix[122][0] = 0; 
}

void half() { // 85  96  111 122
  // "half"
    ledmatrix[85][0] = 1; 
    ledmatrix[96][0] = 1; 
    ledmatrix[111][0] = 1; 
    ledmatrix[122][0] = 1; 
}



void textblock() {


/*
    91 116 117 142 143 168
    92  115 118 141 144 167
 */

 // Personalised Name Text Block

}



void ditis() { // 7  18  44  59
  // "It Is"
    ledmatrix[7][0] = 0; 
    ledmatrix[18][0] = 0; 
    ledmatrix[44][0] = 0; 
    ledmatrix[59][0] = 0; 
}

void itis() { // 7  18  44  59
  // "It Is"
    ledmatrix[7][0] = 1; 
    ledmatrix[18][0] = 1; 
    ledmatrix[44][0] = 1; 
    ledmatrix[59][0] = 1; 
}

void dhappybirthday() { // 113 120 139 146 165 8 17  34  43  60  69  86  95
  // "Happy Birthday"
    ledmatrix[113][0] = 0; 
    ledmatrix[120][0] = 0; 
    ledmatrix[139][0] = 0; 
    ledmatrix[146][0] = 0; 
    ledmatrix[165][0] = 0; 
    ledmatrix[8][0] = 0; 
    ledmatrix[17][0] = 0; 
    ledmatrix[34][0] = 0; 
    ledmatrix[43][0] = 0; 
    ledmatrix[60][0] = 0; 
    ledmatrix[69][0] = 0; 
    ledmatrix[86][0] = 0; 
    ledmatrix[95][0] = 0;         
}

void happybirthday() { // 113 120 139 146 165 8 17  34  43  60  69  86  95
  // "Happy Birthday"
    ledmatrix[113][0] = 1; 
    ledmatrix[120][0] = 1; 
    ledmatrix[139][0] = 1; 
    ledmatrix[146][0] = 1; 
    ledmatrix[165][0] = 1; 
    ledmatrix[8][0] = 1; 
    ledmatrix[17][0] = 1; 
    ledmatrix[34][0] = 1; 
    ledmatrix[43][0] = 1; 
    ledmatrix[60][0] = 1; 
    ledmatrix[69][0] = 1; 
    ledmatrix[86][0] = 1; 
    ledmatrix[95][0] = 1;
}

void dtimetosleep() { // 9 16  35  42  68  87 112  121 138 147 164
  // "Time To Sleep"
    ledmatrix[9][0] = 0; 
    ledmatrix[16][0] = 0; 
    ledmatrix[35][0] = 0; 
    ledmatrix[42][0] = 0; 
    ledmatrix[68][0] = 0; 
    ledmatrix[87][0] = 0; 
    ledmatrix[112][0] = 0; 
    ledmatrix[121][0] = 0; 
    ledmatrix[138][0] = 0; 
    ledmatrix[147][0] = 0; 
    ledmatrix[164][0] = 0;       
}

void timetosleep() { // 113 120 139 146 165 8 17  34  43  60  69  86  95
  // "Time To Sleep"
    ledmatrix[9][0] = 1; 
    ledmatrix[16][0] = 1; 
    ledmatrix[35][0] = 1; 
    ledmatrix[42][0] = 1; 
    ledmatrix[68][0] = 1; 
    ledmatrix[87][0] = 1; 
    ledmatrix[112][0] = 1; 
    ledmatrix[121][0] = 1; 
    ledmatrix[138][0] = 1; 
    ledmatrix[147][0] = 1; 
    ledmatrix[164][0] = 1;   
}


void dcarpediem() { // 10 15  36  41  62  88  93  114 119
  // "dcarpediem"
    ledmatrix[10][0] = 0; 
    ledmatrix[15][0] = 0; 
    ledmatrix[36][0] = 0; 
    ledmatrix[41][0] = 0; 
    ledmatrix[62][0] = 0; 
    ledmatrix[88][0] = 0; 
    ledmatrix[93][0] = 0; 
    ledmatrix[114][0] = 0; 
    ledmatrix[119][0] = 0;       
}

void carpediem() { // 10  15  36  41  62  88  93  114 119
  // "carpediem"
    ledmatrix[10][0] = 1; 
    ledmatrix[15][0] = 1; 
    ledmatrix[36][0] = 1; 
    ledmatrix[41][0] = 1; 
    ledmatrix[62][0] = 1; 
    ledmatrix[88][0] = 1; 
    ledmatrix[93][0] = 1; 
    ledmatrix[114][0] = 1; 
    ledmatrix[119][0] = 1;  
}

void dgoodmorning() { // 12 13  38  39 11 14  37  40  63  66  89
  // "dcarpediem"
    ledmatrix[12][0] = 0; 
    ledmatrix[13][0] = 0; 
    ledmatrix[38][0] = 0; 
    ledmatrix[39][0] = 0; 
    ledmatrix[11][0] = 0; 
    ledmatrix[14][0] = 0; 
    ledmatrix[37][0] = 0; 
    ledmatrix[40][0] = 0; 
    ledmatrix[63][0] = 0;
    ledmatrix[66][0] = 0; 
    ledmatrix[89][0] = 0;           
}

void goodmorning() { // 12 13  38  39 11 14  37  40  63  66  89
  // "carpediem"
    ledmatrix[12][0] = 1; 
    ledmatrix[13][0] = 1; 
    ledmatrix[38][0] = 1; 
    ledmatrix[39][0] = 1; 
    ledmatrix[11][0] = 1; 
    ledmatrix[14][0] = 1; 
    ledmatrix[37][0] = 1; 
    ledmatrix[40][0] = 1; 
    ledmatrix[63][0] = 1;
    ledmatrix[66][0] = 1; 
    ledmatrix[89][0] = 1; 
}

void dclockname() { // 91 116 117 142 143 168 92  115 118 141 144 167
  // "dclockname"
    ledmatrix[91][0] = 0; 
    ledmatrix[116][0] = 0; 
    ledmatrix[117][0] = 0; 
    ledmatrix[142][0] = 0; 
    ledmatrix[143][0] = 0; 
    ledmatrix[168][0] = 0; 
    ledmatrix[92][0] = 0; 
    ledmatrix[115][0] = 0; 
    ledmatrix[118][0] = 0;
    ledmatrix[141][0] = 0; 
    ledmatrix[144][0] = 0;           
    ledmatrix[167][0] = 0;
}

void clockname() { // 91 116 117 142 143 168 92  115 118 141 144 167
  // "clockname"
    ledmatrix[91][0] = 1; 
    ledmatrix[116][0] = 1; 
    ledmatrix[117][0] = 1; 
    ledmatrix[142][0] = 1; 
    ledmatrix[143][0] = 1; 
    ledmatrix[168][0] = 1; 
    ledmatrix[92][0] = 1; 
    ledmatrix[115][0] = 1; 
    ledmatrix[118][0] = 1;
    ledmatrix[141][0] = 1; 
    ledmatrix[144][0] = 1;           
    ledmatrix[167][0] = 1; 
}

void dHi() { // 64  65
  // "one"
    ledmatrix[64][0] = 0; 
    ledmatrix[65][0] = 0; 
}

void Hi() { // 64 65
  // "one"
    ledmatrix[64][0] = 1; 
    ledmatrix[65][0] = 1; 
}


 void UpdateArray() { // From time values update the Array with the LED positions that require lighting and turn off the previous LED values

 // Clear the display

// Blank display
fill_solid( leds, 169, CRGB(0,0,0)); 

// firstly clear Array
   for (int z = 0; z < 169; z++) {

        ledmatrix[z][0] = 0;
   }


  
    // "Display the time based on current RTC data"
  
  itis(); // Display it is permanently
  
   if((clockminute>4) && (clockminute<10)){
    // FIVE MINUTES
    fivemins();
    past();
     
    dTo();
    dtenmins();
    dquarter();
    dtwenty();
    dhalf();
    doclock();   
    }
   if((clockminute>9) && (clockminute<15)) {
    //TEN MINUTES;
    tenmins();
    past();
    
    dTo();
    dfivemins();
    dquarter();
    dtwenty();
    dhalf();
    doclock();
    }
  
   if((clockminute>14) && (clockminute<20)) {
    // QUARTER
    quarter();
    past();
       
    dfivemins();
    dTo();
    dtenmins();
    dtwenty();
    dhalf();
    doclock();
    }
  
   if((clockminute>19) && (clockminute<25)) {
    //TWENTY MINUTES
    twenty();
    past();
    
    dfivemins();
    dTo();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((clockminute>24) && (clockminute<30)) {
    //TWENTY FIVE
    twentyfive();
    past();
    
    dTo();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((clockminute>29) && (clockminute<35)) {
    // HALF
    half();
    past();
    
    dfivemins();
    dTo();
    dtenmins();
    dquarter();
    dtwenty();
    doclock();
    }
  
   if((clockminute>34) && (clockminute<40)) {
    //TWENTY FIVE TO
    twentyfive();
    To();
    
    dpast();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((clockminute>39) && (clockminute<45)) {
    //TWENTY TO  
    twenty();
    To();
  
    dfivemins();
    dpast();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((clockminute>44) && (clockminute<50)) {
     // QUARTER TO
     quarter();
     To();
     
    dfivemins();
    dpast();
    dtenmins();
    dtwenty();
    dhalf();
    doclock();  
    }
     
  if((clockminute>49) && (clockminute<55)){
    // TEN TO
    tenmins();
    To();
    
    dfivemins();
    dpast();
    dquarter();
    dtwenty();
    dhalf();
    doclock();
    }
    
   if(clockminute>54){
    // FIVE TO
    fivemins();
    To();
   
    dpast();
    dtenmins();
    dquarter();
    dtwenty();
    dhalf();
    doclock();
    }
   
    if(clockminute<5){
     // OClock
     oclock();
     
    dfivemins();
    dpast();
    dTo();
    dtenmins();
    dquarter();
    dtwenty();
    dhalf();   
     
   }
  
  
  // Display correct Hour for the clock
  // Do this by determining if after 30mins past the hour
  
  if(clockminute<35){
  
  // Hours on the clock if 34 minutes or less past the hour
  
    if((clockhour==1)||(clockhour==13)){
     // One Oclock
     one();
     
     dtwelve();
     dtwo();
     }
  
    if((clockhour==2)||(clockhour==14)){
     // Two Oclock
     two();
     
     done();
     dthree();
     }
     
    if((clockhour==3)||(clockhour==15)){
     // Three Oclock
     three();
     
     dtwo();
     dfour();
     }   
     
    if((clockhour==4)||(clockhour==16)){
     // four Oclock
     four();
     
     dthree();
     dfive();
     }
  
    if((clockhour==5)||(clockhour==17)){
     // five Oclock
     five();
     
     dfour();
     dsix();
     }
     
    if((clockhour==6)||(clockhour==18)){
     // Six Oclock
     six();
     
     dfive();
     dseven();
     }   
    if((clockhour==7)||(clockhour==19)){
     // Seven Oclock
     seven();
     
     dsix();
     deight();
     }
  
    if((clockhour==8)||(clockhour==20)){
     // Eight Oclock
     eight();
     
     dseven();
     dnine();
     }
     
    if((clockhour==9)||(clockhour==21)){
     // Nine Oclock
     nine();
     
     deight();
     dten();
     }   
     
    if((clockhour==10)||(clockhour==22)){
     // Ten Oclock
     ten();
     
     dnine();
     deleven();
     }
  
    if((clockhour==11)||(clockhour==23)){
     // Eleven Oclock
     eleven();
     
     dten();
     dtwelve();
     }
     
    if((clockhour==12)||(clockhour==0)){
     // Twelve Oclock
     twelve();
     
     deleven();
     done();
     } 
  
  } else {
  
  
  // Hours on the clock if 35 minutes or more past the hour
  
    if((clockhour==1)||(clockhour==13)){
     // To two Oclock
     two();
     
     done();
     dthree();
     }
  
    if((clockhour==2)||(clockhour==14)){
     // To three Oclock
     three();
     
     dtwo();
     dfour();
     }
     
    if((clockhour==3)||(clockhour==15)){
     // To four Oclock
     four();
       
     dthree();
     dfive();
     }   
     
    if((clockhour==4)||(clockhour==16)){
     // To five Oclock
     five();
     
     dfour();
     dsix();
     }
  
    if((clockhour==5)||(clockhour==17)){
     // To six Oclock
     six();
     
     dfive();
     dseven();
     }
     
    if((clockhour==6)||(clockhour==18)){
     // To seven Oclock
     seven();
     
     dsix();
     deight();
     }   
    if((clockhour==7)||(clockhour==19)){
     // To eight Oclock
     eight();
     
     dseven();
     dnine();
     }
  
    if((clockhour==8)||(clockhour==20)){
     // To nine Oclock
     nine();
     
     deight();
     dten();
     }
     
    if((clockhour==9)||(clockhour==21)){
     // To ten Oclock
     ten();
     
     dnine();
     deleven();
     }   
     
    if((clockhour==10)||(clockhour==22)){
     // To eleven Oclock
     eleven();
     
     dten();
     dtwelve();
     }
  
    if((clockhour==11)||(clockhour==23)){
     // To twelve Oclock
     twelve();
     
     deleven();
     done();
     }
     
    if((clockhour==12)||(clockhour==0)){
     // To one Oclock
     one();
     
     dtwelve();
     dtwo();
     } 
  
    
  } 

// Time of Day Messages

    if((clockhour>=9)&&(clockhour<=10)){
     // Carpe Diem between 9am and 10am
     carpediem();
     clockname();
     
     dgoodmorning();
     dtimetosleep();
     } else { 
     dclockname();
     }


    if((clockhour>=23)&&(clockhour<=5)){
     // Time to Sleep between 11pm and 5am
     timetosleep();
     clockname();     
     
     dgoodmorning();
     dcarpediem();
     }else { 
     dclockname();
     }


    if((clockhour>=6)&&(clockhour<=9)){
     // Good Morning between 6am and 9am
     goodmorning();
     clockname();     

     dtimetosleep();     
     dcarpediem();
     } else { 
     dclockname();
     }

    // Happy Birthday conditions
     if((clockday==birthday)&&(clockmonth==birthmonth)){
     // Happy Birthday displayed if Date and Month correct only

        happybirthday();     
     }else 
     {
        dhappybirthday();
     }


}

//*****
 void UpdateAlarm() { // From time values update the Array with the LED positions that require lighting and turn off the previous LED values
 
  
    // "Display the time based on current RTC data"
  
  
   if((alarmminute>4) && (alarmminute<10)){
    // FIVE MINUTES
    fivemins();
    past();
     
    dTo();
    dtenmins();
    dquarter();
    dtwenty();
    dhalf();
    doclock();   
    }
   if((alarmminute>9) && (alarmminute<15)) {
    //TEN MINUTES;
    tenmins();
    past();
    
    dTo();
    dfivemins();
    dquarter();
    dtwenty();
    dhalf();
    doclock();
    }
  
   if((alarmminute>14) && (alarmminute<20)) {
    // QUARTER
    quarter();
    past();
       
    dfivemins();
    dTo();
    dtenmins();
    dtwenty();
    dhalf();
    doclock();
    }
  
   if((alarmminute>19) && (alarmminute<25)) {
    //TWENTY MINUTES
    twenty();
    past();
    
    dfivemins();
    dTo();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((alarmminute>24) && (alarmminute<30)) {
    //TWENTY FIVE
    twentyfive();
    past();
    
    dTo();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((alarmminute>29) && (alarmminute<35)) {
    // HALF
    half();
    past();
    
    dfivemins();
    dTo();
    dtenmins();
    dquarter();
    dtwenty();
    doclock();
    }
  
   if((alarmminute>34) && (alarmminute<40)) {
    //TWENTY FIVE TO
    twentyfive();
    To();
    
    dpast();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((alarmminute>39) && (alarmminute<45)) {
    //TWENTY TO  
    twenty();
    To();
  
    dfivemins();
    dpast();
    dtenmins();
    dquarter();
    dhalf();
    doclock();
    }
  
  if((alarmminute>44) && (alarmminute<50)) {
     // QUARTER TO
     quarter();
     To();
     
    dfivemins();
    dpast();
    dtenmins();
    dtwenty();
    dhalf();
    doclock();  
    }
     
  if((alarmminute>49) && (alarmminute<55)){
    // TEN TO
    tenmins();
    To();
    
    dfivemins();
    dpast();
    dquarter();
    dtwenty();
    dhalf();
    doclock();
    }
    
   if(alarmminute>54){
    // FIVE TO
    fivemins();
    To();
   
    dpast();
    dtenmins();
    dquarter();
    dtwenty();
    dhalf();
    doclock();
    }
   
    if(alarmminute<5){
     // OClock
     oclock();
     
    dfivemins();
    dpast();
    dTo();
    dtenmins();
    dquarter();
    dtwenty();
    dhalf();   
     
   }
  
  
  // Display correct Hour for the clock
  // Do this by determining if after 30mins past the hour
  
  if(alarmminute<35){
  
  // Hours on the clock if 34 minutes or less past the hour
  
    if((alarmhour==1)||(alarmhour==13)){
     // One Oclock
     one();
     
     dtwelve();
     dtwo();
     }
  
    if((alarmhour==2)||(alarmhour==14)){
     // Two Oclock
     two();
     
     done();
     dthree();
     }
     
    if((alarmhour==3)||(alarmhour==15)){
     // Three Oclock
     three();
     
     dtwo();
     dfour();
     }   
     
    if((alarmhour==4)||(alarmhour==16)){
     // four Oclock
     four();
     
     dthree();
     dfive();
     }
  
    if((alarmhour==5)||(alarmhour==17)){
     // five Oclock
     five();
     
     dfour();
     dsix();
     }
     
    if((alarmhour==6)||(alarmhour==18)){
     // Six Oclock
     six();
     
     dfive();
     dseven();
     }   
    if((alarmhour==7)||(alarmhour==19)){
     // Seven Oclock
     seven();
     
     dsix();
     deight();
     }
  
    if((alarmhour==8)||(alarmhour==20)){
     // Eight Oclock
     eight();
     
     dseven();
     dnine();
     }
     
    if((alarmhour==9)||(alarmhour==21)){
     // Nine Oclock
     nine();
     
     deight();
     dten();
     }   
     
    if((alarmhour==10)||(alarmhour==22)){
     // Ten Oclock
     ten();
     
     dnine();
     deleven();
     }
  
    if((alarmhour==11)||(alarmhour==23)){
     // Eleven Oclock
     eleven();
     
     dten();
     dtwelve();
     }
     
    if((alarmhour==12)||(alarmhour==0)){
     // Twelve Oclock
     twelve();
     
     deleven();
     done();
     } 
  
  } else {
  
  
  // Hours on the clock if 35 minutes or more past the hour
  
    if((alarmhour==1)||(alarmhour==13)){
     // To two Oclock
     two();
     
     done();
     dthree();
     }
  
    if((alarmhour==2)||(alarmhour==14)){
     // To three Oclock
     three();
     
     dtwo();
     dfour();
     }
     
    if((alarmhour==3)||(alarmhour==15)){
     // To four Oclock
     four();
       
     dthree();
     dfive();
     }   
     
    if((alarmhour==4)||(alarmhour==16)){
     // To five Oclock
     five();
     
     dfour();
     dsix();
     }
  
    if((alarmhour==5)||(alarmhour==17)){
     // To six Oclock
     six();
     
     dfive();
     dseven();
     }
     
    if((alarmhour==6)||(alarmhour==18)){
     // To seven Oclock
     seven();
     
     dsix();
     deight();
     }   
    if((alarmhour==7)||(alarmhour==19)){
     // To eight Oclock
     eight();
     
     dseven();
     dnine();
     }
  
    if((alarmhour==8)||(alarmhour==20)){
     // To nine Oclock
     nine();
     
     deight();
     dten();
     }
     
    if((alarmhour==9)||(alarmhour==21)){
     // To ten Oclock
     ten();
     
     dnine();
     deleven();
     }   
     
    if((alarmhour==10)||(alarmhour==22)){
     // To eleven Oclock
     eleven();
     
     dten();
     dtwelve();
     }
  
    if((alarmhour==11)||(alarmhour==23)){
     // To twelve Oclock
     twelve();
     
     deleven();
     done();
     }
     
    if((alarmhour==12)||(alarmhour==0)){
     // To one Oclock
     one();
     
     dtwelve();
     dtwo();
     } 
  
    
  } 



}

//*****

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); 

  clockhour = timeinfo.tm_hour;
  clockminute = timeinfo.tm_min;
  clockday = timeinfo.tm_mday; // day of month [1,31]
  clockmonth = timeinfo.tm_mon; // month of year [0,11]

/*
  Serial.print(clockday);
  Serial.print("  ");
  Serial.println(clockmonth);
*/
  
}

void refreshtime(){ // Periodically read the current date and time from the RTC and reset board

// Read the current date and time from the RTC and reset board
rfc++;
  if (rfc >= rfcvalue) { // count cycles and print time
    printLocalTime();
    UpdateArray();   
    UpdateDisplay0(); // Redraw the display using the selected screen drawing effect 
    rfc = 0;
  }
  
}


void Matrixeffect() { // Matrix effect where characters fall from the sky randomly and then settle on letters in words

// Blank display
fill_solid( leds, 169, CRGB(0,0,0)); 
FastLED.show();   
delay(300);

byte w = 0; // temp variable

// Generate random delay speeds for each falling charater in columns
byte dly1[13] = { random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10)  };

byte ptr[13] = { 0,0,0,0,0,0,0,0,0,0,0,0,0 }; // Points to the LED in each column we need to deal with
/*
  for ( byte o = 0; o < 11 ; o++) {

   Serial.print(dly1[o]); 
   Serial.print("   "); 
   Serial.println(ptr[o]);     
  }
*/

  for ( byte t = 0; t < 260; t++) { // Counter for completing all changes for 13 rows

      for ( byte c = 0; c < 13; c++) { // Count through columns and check timer not exceeded
   

          if ((t > (dly1[c]*(ptr[c] + 1))) && ptr[c] < 9 ) { // If timer exceeded then erase current value and draw a white curosr in this position based on random time period     


                // Write over the previous value
                // Calculate the LED value from the Column and ptr value
                w = ((c * 10)+(9 - ptr[c])); // Calculate the LED value in the 11 x 10 Matrix



                  if ( (ledmatrix[w][0] == 1) &&(ptr[c]!=0)) { // If the bit set in LED Matrix then leave White
                      leds[w] = CRGB(120,255,255);                      
                     } else {            
                    leds[w] = CRGB(0,150,0); 
                    }
               
                FastLED.show();   
                delay(20);

                ptr[c]++; // Increment row and print White value    
                
                if ( ptr[c] < 9 ) { 
                    
                    // Calculate the LED value from the Column and ptr value
                    w = ((c * 10)+(9 - ptr[c])); // Calculate the LED value in the 11 x 10 Matrix
                    leds[w] = CRGB(120,255,255); 
                    FastLED.show(); 


      
                } else {
                // Calculate the LED value from the Column and ptr value
                    w = ((c * 10)+(9 - ptr[c])); // Calculate the LED value in the 11 x 10 Matrix
                    leds[w] = CRGB(0,150,0); 
                    FastLED.show();
              
                }
          
          }
      }

  }

 // Do last row in Array with White

     for ( byte n = 0; n < 101; n=n+10){

                if (ledmatrix[n][0] == 1){
                     
                     leds[n] = CRGB(120,255,255); 
                    FastLED.show();  
                }    
     }

/// Now clear the screen

// Generate random delay speeds for each falling charater in columns
byte dly2[11] = { random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10), random(10) };

byte ptr2[11] = { 0,0,0,0,0,0,0,0,0,0,0 }; // Points to the LED in each column we need to deal with
/*
  for ( byte o = 0; o < 11 ; o++) {

   Serial.print(dly1[o]); 
   Serial.print("   "); 
   Serial.println(ptr[o]);     
  }
*/

  for ( byte t = 0; t < 200; t++) { // Counter for completing all changes for 10 rows

      for ( byte c = 0; c < 11; c++) { // Count through columns and check timer not exceeded
   

          if ((t > (dly2[c]*(ptr2[c] + 1))) && ptr2[c] < 9 ) { // If timer exceeded then erase current value and draw a white curosr in this position based on random time period     


                // Write over the previous value
                // Calculate the LED value from the Column and ptr value
                w = ((c * 10)+(9 - ptr2[c])); // Calculate the LED value in the 11 x 10 Matrix



                  if ( (ledmatrix[w][0] == 1) &&(ptr2[c]!=0)) { // If the bit set in LED Matrix then leave White
                      leds[w] = CRGB(120,255,255);                      
                     } else {            
                    leds[w] = CRGB(0,0,0); 
                    }
               
                FastLED.show();   
                delay(5);

                ptr2[c]++; // Increment row and print White value    
                
                if ( ptr2[c] < 9 ) { 
                    
                    // Calculate the LED value from the Column and ptr value
                    w = ((c * 10)+(9 - ptr2[c])); // Calculate the LED value in the 11 x 10 Matrix
                    leds[w] = CRGB(0,150,0); 
                    FastLED.show(); 


      
                } else {
                // Calculate the LED value from the Column and ptr value
                    w = ((c * 10)+(9 - ptr2[c])); // Calculate the LED value in the 11 x 10 Matrix

                  if (ledmatrix[w][0] == 0) {
                        leds[w] = CRGB(0,0,0); 
                        FastLED.show();
                  }     
              
                }
          
          }
      }



  }
 
 
 // Do last row in Array with White

     for ( byte n = 0; n < 101; n=n+10){

                if (ledmatrix[n][0] == 1){
                     
                     leds[n] = CRGB(120,255,255); 
                    FastLED.show();  
                }    
     }
 
 
 
 //          leds[100] = CRGB(255,255,0); 
 //         FastLED.show();    

/*      
  for ( byte o = 0; o < 11 ; o++) {

   Serial.print(dly1[o]); 
   Serial.print("   "); 
   Serial.println(ptr[o]);     
  }
*/

delay(2000);

}


//void playalarmsound(){
//
//// Blank display
//fill_solid( leds, 169, CRGB(0,0,0)); 
//FastLED.show();   
//
//
//  // UNMUTE sound to speaker via pin 5 of PAM8403 using ESP32 pin 32
//  digitalWrite(32,HIGH);
//
// 
//  for (int startsound = 1; startsound < 5000000; startsound++) {
//      
//  DacAudio.FillBuffer();                // Fill the sound buffer with data
//  if(ForceWithYou.Playing==false)       // if not playing,
//      DacAudio.Play(&ForceWithYou);
//  }
//
//  // MUTE sound to speaker via pin 5 of PAM8403 using ESP32 pin 32
//  digitalWrite(32,LOW);
//
//}


void dimdisplay() { // Read and set the brightness of the LEDs

int dimvar = 0;
float bright = 0;

// 400 - Bright Light
// 0 - Dark

dimvar = analogRead(13); 

//      p.x = map(p.x, TS_MINX, TS_MAXX, 240, 0);    

bright = map(dimvar, 400, 0, 80, 8); // needs chnging back to map(dimvar, 250, 1023, 60, 5)

if (bright > 99){ // Keep variables in range in bight light
  bright = 99;
}

// Firstly calculate new brightness based on 

bright = (alpha * legacybrightness) + ( (1 - alpha) * bright);

legacybrightness = bright;

// Insert smoothing algorythm to remove rapid variations in ambient light


  LEDS.setBrightness(bright); //  If LDR is installed uncomment this line 

//  LEDS.setBrightness(20); //  If LDR is NOT installed uncomment this line 
   Serial.print(dimvar);
   Serial.print("   ");   
   Serial.println(bright);
}

//void alarmroutine(){ // Play Alarm for sound for 60 seconds
//
//    for (int s = 1; s < 20; s++) {
//        playalarmsound();
//    
//      // Check if a button pressed then cancel alarm
//      if((digitalRead(27) == LOW)||(digitalRead(26) == LOW)){
//        delay(200);
//        if((digitalRead(27) == LOW)||(digitalRead(26) == LOW)){
//          s = 50; // stop the sound loop
//          // Disarm the Alarm
//          alarmstatus = false;
//        }
//      }
//    } 
//    // Disarm the Alarm
//    alarmstatus = false;
//    color = 4 ; 
//    bkcolor = 2;
//    
//    
//}
