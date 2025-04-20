/*
 * LAB Name: Keystudio rotary encoder, 8x16 matrix, colour serial, L298N
 * Author: JustsomeStuff
 * For More Info Visit: most reputable arduino sites
 * Grafted from lots of reading and sites, sorry for missing lots of credits to people
 * Deft liked the 5x5 font someone uploaded for digits to get more chars on the led matrix

 * I moved to using visual code, so this is main.cpp save to compile with this name
 * in visual Studio, remapped keys to be ctrl+shift+caps to kill running terminal
 * in visual Studio, remapped keys to be ctrl+caps to upload to arduino
 * might still run as an ino file in arduino studio, but there are library differences to resolve
   prefered visual code in the end,  
  
 * Setup template / overloaded Serial Colour output
 * Setup some specific board constants
 * Setup Stepper motor variables, there were two boards I worked with so bits of code for both here keyestudio 
 * have the stepper motor 2 set of windings and a small driver board in a small kit I used
 * I setup the stepper motor with its own board to test
 * and also used the L298H board as well for a larger dc motor
 
 * Rotory encoder, the interupt routines for these were a fail, due to lack of debounce,
   so moved to loop and logic instead,someone wrote this bit, works very reliably

 * setup calls led, motor, rotary encoder setup
 * loop calls rotary encoder  / button loop only, the rest is 'event' driven
 * bigget learning, fonts, led matrix setup

*/


/* Credit where available : 
 * LAB Name: Arduino DC Motor Control With L298N Driver
 * Author: Khaled Magdy
 * For More Info Visit: www.DeepBlueMbedded.com
*/



#include <Arduino.h>
//#include <l298n.h>

// Below is for  Keyestudio_8x16matrix matrix = Keyestudio_8x16matrix();
#include <Wire.h>
//#include "Matrix.h"
#include "Keyestudio_LEDBackpack.h"
#include "Keyestudio_GFX.h"

// #include "Adafruit_LEDBackpack.h"


/*=====================================================================================
====== Sorting out colour serial printing
======

*/

// FIRST CHAR 
const String Bold 		= "1";
const String Underline 	= "4";

// Second CHAR 
const String fgcolor 	= "3" ;
const String bgcolor 	= "4" ; 

const String Normal		= "0";
const String Intense 	= "9";

// Third Colors  ( or 2nd for background )
const String Black 		= "0" ; 
const String Red 		= "1" ;
const String Green  	= "2" ;
const String Yellow 	= "3" ;
const String Blue  		= "4" ;
const String Purple 	= "5" ;
const String Cyan  		= "6" ;
const String White  	= "7" ;


const bool mydebugstate = true;

//using ColorNames 
using namespace std;

//template<class TYPE> TYPE Add(TYPE n1, TYPE n2);


template <class T>  void mydebug(String FColor = White , String BColor = Black ,T ToPrint = '\0' ) {
	if (!mydebugstate) {

	}else {
		Serial.print("\e[0;9"+FColor+"m\e[4"+BColor+"m");
		Serial.print(ToPrint);
		Serial.print("\e[0m");
		Serial.flush();
	}
}

template <class T>  void mydebugln(String FColor = White , String BColor = Black , T ToPrint = '\0'  ) {
	if (!mydebugstate) {

	}else {

		
		Serial.print("\e[0;9"+FColor+"m\e[4"+BColor+"m");
		Serial.print(ToPrint);
		Serial.println("\e[0m");
		Serial.flush();
   }
}

template <class T>  void mydebugln(T ToPrint  = '\0' ) {
	if (!mydebugstate) {

	}else {
		Serial.print(ToPrint);
		Serial.print("\e[0m");
    Serial.print("\n");
		Serial.flush();
	}
}

template <class T>  void mydebug(T ToPrint  = '\0' ) {
	if (!mydebugstate) {

	}else {
		Serial.print(ToPrint);
		Serial.print("\e[0m");
		Serial.flush();
	}
}



template <class T>  void mydebugAlert(T ToPrint = '\0' ) { // print Alert default alert colors
	if (!mydebugstate) {

	}else {
		Serial.print("\e[0;9"+Red+"m\e[4"+White+"m");
		Serial.print(ToPrint);
		Serial.print("\e[0m");
		Serial.flush();
	}
}

template <class T>  void mydebugInfo(T ToPrint = '\0' ) { // print info default info colors
	if (!mydebugstate) {

	}else {
		Serial.print("\e[0;9"+Green+"m\e[4"+White+"m");
		Serial.print(ToPrint);
		Serial.print("\e[0m");
		Serial.flush();
	}
}



void mydebugtest () { 
  
  mydebugln(Red,White,"Loop1");
  mydebugln(Green,Purple,"Loop2");
  mydebugln(Purple,Yellow,"Loop4");
  mydebugln(Blue,Green,"Loop5");
  //mydebugln(White,White,"Loop3");

  mydebug(Red,White,"Loop1");
  mydebug(Green,Purple,"Loop2");
  mydebug(Purple,Yellow,"Loop4");
  mydebug(Blue,Green,"Loop5");



//const ColorNames=( Black Red Green Yellow Blue Magenta Cyan White )
//const char  Colors=(   0    1    2     3     4      5      6    7  )

}



/*=====================================================================================
====== Global Constants
======

*/

const int DECIMAL = 10;
const int HEX16 = 16;


// Arrduino Uno hardware
// CONST for LEDS
const int ledRed = 8;       ////define digital 8
const int ledYellow = 10;   ////define digital 10
const int ledGreen = 13;    //define digital 13

// CONST for BUTTONS
const int ButtonLeft = 3;   //define digital 2
const int ButtonRight = 2;  //define digital 3

const int SPEAKER9 = 9 ; // PITA internal speaker on keystudion uno dev board, shutup the speaker setting to outpput in setup() 


/*=====================================================================================
====== Stepper Motor Variables
======

*/


// common interface vars
// Stepper motor informaiton 
int NewRpmMotorA = 0; // for stepper motor note this : < 6 gets hot , good = 16 , > 24 skips
int NewRpmMotorB = 0; // for stepper motor note this :  < 6 gets hot , good = 16 , > 24 skips
// these are really for the DC motor driver L298N
// mA is confusing, means MotorA on the driver board, not milliamps
int NewLocationMotorA = 0 ; // lets say current is zero , no Hall effect at this time .. 2048
bool mA_running = false ; // keep logic of motor state -1 = ccw ; 1 = cw  
int mA_direction = 1 ; // keep logic of motor state -1 = ccw ; 1 = cw  
int MotorControllerA = 0 ; // CCW  < 0 , 0 = stopped ,> 0  CW ; if 0 then mBpin1=0 and mBpin2=0 and mEn1 =0 
int MotorControllerB = 0 ; // CCW  < 0 , 0 = stopped ,> 0  CW ; if 0 then mBpin1=0 and mBpin2=0 and mEn1 =0 


unsigned long T1 = 0, T2 = 0;
uint8_t TimeInterval = 5; // 5ms
bool mA_CW = true ; // switch if cw and ccw need to be reversed
bool mB_CW = true ; // switch if cw and ccw need to be reversed


// Define motor pins

#define mA_pIn1 7 // Pin CONTROLS DIRECTION // Red // digital
#define mA_pIn2 8 // Pin CONTROLS DIRECTION // Digital
#define mA_En 10 // THIS CONTROLS MOTOR SPEED 0..255 // PWM // note 9 is buzzer..
#define mB_pIn1 13 // Pin CONTROLS DIRECTION // Green // digital
#define mB_pIn2 4 // Pin CONTROLS DIRECTION // Digital // Sw4 Digital
#define mB_En 11 // THIS CONTROLS MOTOR SPEED 0..255 // PWM // note 9 is buzzer..


// rotor encoder variables


// https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
// https://docs.particle.io/reference/device-os/api/interrupts/attachinterrupt/
// https://deepbluembedded.com/arduino-interrupts-tutorial-examples/
// https://stackoverflow.com/questions/610916/easiest-way-to-flip-a-boolean-value

/*
// when using PWM / digital ports but we dont need to can use analog as well
#define CLK 2
#define DT 3
#define SW 4

*/


// ROTARY ENCODER : when using PWM / digital ports but we dont need to can use analog as well
// All Arduino A0..A6 are INPUTS no OUTPUTS
#define reA_CLK A2
#define reA_DT  A1
#define reA_SW  A0


// Vars for Rotary Encoder Module XC3736
int counter = 0;
volatile int reA_currentStateCLK;
int reA_lastStateCLK;
volatile int reA_currentStateDT;
int reA_lastStateDT;

bool reA_btnStateFlip  = false ;  // reads the  button being clicked , stuff happens

unsigned long lastButtonPress = 0;
volatile int reA_btnState ;


bool ButtonLeft_btnState_btnStateFlip  = false ;  // reads the  button being clicked , stuff happens
unsigned long ButtonLeft_btnState_lastButtonPress = 0;
volatile int ButtonLeft_btnState ;

bool ButtonRight_btnState_btnStateFlip  = false ;  // reads the  button being clicked , stuff happens
unsigned long ButtonRight_btnState_lastButtonPress = 0;
volatile int ButtonRight_btnState ;



// int ButtonStatus[] = { 0 };  //define variable 8 bits for button states, will add DIP switch as well



/*=====================================================================================
====== Some Common functions 
======
*/
 
 bool debounce(int thisBtn)
{
  static uint16_t btnState = 0;
  btnState = (btnState<<1) | (!digitalRead(thisBtn));
  return (btnState == 0xFFF0);
}




/*=====================================================================================
====== Some dotmatrix functions 
======
*/

/*
const uint64_t IMAGES[] = {
  0x7e1818181c181800
};
const int IMAGES_LEN = sizeof(IMAGES)/8;
*/

/*

{
'0', { 0x2, 0x5, 0x5, 0x5, 0x2 },
'1', { 0x2, 0x6, 0x2, 0x2, 0x7 },
'2', { 0x6, 0x1, 0x2, 0x4, 0x7 },
'3', { 0x6, 0x1, 0x2, 0x1, 0x6 },
'4', { 0x1, 0x5, 0x7, 0x1, 0x1 },
'5', { 0x7, 0x4, 0x6, 0x1, 0x6 },
'6', { 0x3, 0x4, 0x6, 0x5, 0x2 },
'7', { 0x7, 0x1, 0x2, 0x2, 0x2 },
'8', { 0x2, 0x5, 0x2, 0x5, 0x2 },
'9', { 0x2, 0x5, 0x3, 0x1, 0x6 },
};

*/
/*

static const uint8_t PROGMEM 
  num0[] =  { 0x2, 0x5, 0x5, 0x5, 0x2 },
  num1[] =  { 0x2, 0x5, 0x5, 0x5, 0x2 },
  num2[] =  { 0x2, 0x6, 0x2, 0x2, 0x7 },
  num3[] =  { 0x6, 0x1, 0x2, 0x4, 0x7 },
  num4[] =  { 0x6, 0x1, 0x2, 0x1, 0x6 },
  num5[] =  { 0x1, 0x5, 0x7, 0x1, 0x1 },
  num6[] =  { 0x7, 0x4, 0x6, 0x1, 0x6 },
  num7[] =  { 0x3, 0x4, 0x6, 0x5, 0x2 },
  num8[] =  { 0x7, 0x1, 0x2, 0x2, 0x2 },
  num9[] =  { 0x2, 0x5, 0x3, 0x1, 0x6 };

const PROGMEM uint8_t  numA[12][5] =  {
  { 0x2, 0x5, 0x5, 0x5, 0x2 },
  { 0x2, 0x6, 0x2, 0x2, 0x7 },
  { 0x6, 0x1, 0x2, 0x4, 0x7 },
  { 0x6, 0x1, 0x2, 0x1, 0x6 },
  { 0x1, 0x5, 0x7, 0x1, 0x1 },
  { 0x7, 0x4, 0x6, 0x1, 0x6 },
  { 0x3, 0x4, 0x6, 0x5, 0x2 },
  { 0x7, 0x1, 0x2, 0x2, 0x2 },
  { 0x2, 0x5, 0x2, 0x5, 0x2 },
  { 0x2, 0x5, 0x3, 0x1, 0x6 }
};

const PROGMEM uint8_t  numB[12][8] =  {
   { 0xD6,0xE0,0x0,0x0,0x0,0x0,0x0,0x0 }, // '0'
   { 0xC9,0x20,0x0,0x0,0x0,0x0,0x0,0x0}, // '1'
   { 0x62,0x48,0x0,0x0,0x0,0x0,0x0,0x0}, // '2'
   { 0xE8,0xE0,0x0,0x0,0x0,0x0,0x0,0x0}, // '3'
   { 0x26,0x62,0x0,0x0,0x0,0x0,0x0,0x0}, // '4'
   { 0x08,0x90,0x0,0x0,0x0,0x0,0x0,0x0}, // '5'
   { 0x7A,0xE0,0x0,0x0,0x0,0x0,0x0,0x0}, // '6'
   { 0x25,0x20,0x0,0x0,0x0,0x0,0x0,0x0}, // '7'
   { 0xEA,0xE0,0x0,0x0,0x0,0x0,0x0,0x0}, // '8'
   { 0xD5,0xE0,0x0,0x0,0x0,0x0,0x0,0x0}, // '9'
};

*/


/*
'0', { 0x2, 0x5, 0x5, 0x5, 0x2 },
'1', { 0x2, 0x6, 0x2, 0x2, 0x7 },
'2', { 0x6, 0x1, 0x2, 0x4, 0x7 },
'3', { 0x6, 0x1, 0x2, 0x1, 0x6 },
'4', { 0x1, 0x5, 0x7, 0x1, 0x1 },
'5', { 0x7, 0x4, 0x6, 0x1, 0x6 },
'6', { 0x3, 0x4, 0x6, 0x5, 0x2 },
'7', { 0x7, 0x1, 0x2, 0x2, 0x2 },
'8', { 0x2, 0x5, 0x2, 0x5, 0x2 },
'9', { 0x2, 0x5, 0x3, 0x1, 0x6 },
*/

static const uint8_t PROGMEM
  heart_bmp[] =
  {0x00, 0x00, 0x00, 0x0e, 0x1f, 0x3f, 0x7f, 0xfe, 0xfe, 0x7f, 0x3f, 0x1f, 0x0e, 0x00, 0x00, 0x00},
  smile_bmp[] =
  { 0x00, 0x00, 0x00, 0x04, 0x22, 0x42, 0x84, 0x80, 0x80, 0x84, 0x42, 0x22, 0x04, 0x00, 0x00, 0x00 },
  frown_bmp[] =
  { 0x00, 0x00, 0x00, 0x02, 0x84, 0x44, 0x22, 0x20, 0x20, 0x22, 0x44, 0x84, 0x02, 0x00, 0x00, 0x00 };

/*
struct Cgr { 
  char c; 
  byte d[5];
}  nums[] = 
{
'0', { 0x2, 0x5, 0x5, 0x5, 0x2 },
'1', { 0x2, 0x6, 0x2, 0x2, 0x7 },
'2', { 0x6, 0x1, 0x2, 0x4, 0x7 },
'3', { 0x6, 0x1, 0x2, 0x1, 0x6 },
'4', { 0x1, 0x5, 0x7, 0x1, 0x1 },
'5', { 0x7, 0x4, 0x6, 0x1, 0x6 },
'6', { 0x3, 0x4, 0x6, 0x5, 0x2 },
'7', { 0x7, 0x1, 0x2, 0x2, 0x2 },
'8', { 0x2, 0x5, 0x2, 0x5, 0x2 },
'9', { 0x2, 0x5, 0x3, 0x1, 0x6 },
};

*/


// someones 5x5 font for numbers, really good
const PROGMEM uint8_t  num58[12][8] =  {
  { 0x2, 0x5, 0x5, 0x5, 0x2, 0x0,0x0,0x0 },
  { 0x2, 0x6, 0x2, 0x2, 0x7, 0x0,0x0,0x0 },
  { 0x6, 0x1, 0x2, 0x4, 0x7, 0x0,0x0,0x0 },
  { 0x6, 0x1, 0x2, 0x1, 0x6, 0x0,0x0,0x0 },
  { 0x1, 0x5, 0x7, 0x1, 0x1, 0x0,0x0,0x0 },
  { 0x7, 0x4, 0x6, 0x1, 0x6, 0x0,0x0,0x0 },
  { 0x3, 0x4, 0x6, 0x5, 0x2, 0x0,0x0,0x0 },
  { 0x7, 0x1, 0x2, 0x2, 0x2, 0x0,0x0,0x0 },
  { 0x2, 0x5, 0x2, 0x5, 0x2, 0x0,0x0,0x0 },
  { 0x2, 0x5, 0x3, 0x1, 0x6, 0x0,0x0,0x0 }
};



Keyestudio_8x16matrix matrix = Keyestudio_8x16matrix();
Keyestudio_AlphaNum4  matrix4 = Keyestudio_AlphaNum4();

//Adafruit_8x16matrix  matrix =  Adafruit_8x16matrix();


int matrix_brightness = 0;

void Keyestudio_8x16matrix_setup() 
{
 matrix.begin(0x70);  // pass in the address
 matrix.setBrightness(matrix_brightness); // Keyestudio_LEDBackpack::setBrightness(uint8_t b) 0 = very dim , 15 = bright
}



char* binToStr(int val) {
  static char buf[5];
  for (int i = 0; i < 4; i++)
  {
    buf[i] = '0';
    if (val & (8 >> i) ) {
      ++buf[i];
    }
  }
  return buf;
}

unsigned int bintohex(char *digits){
  unsigned int res=0;
  while(*digits)
    res = (res<<1)|(*digits++ -'0');
  return res;
}

int x;
String s;


void mydrawPixel(int x, int y, boolean led_state) {
    // matrix.setBrightness(14); // matrix_brightness); // Keyestudio_LEDBackpack::setBrightness(uint8_t b) 0 = very dim , 15 = bright 
    matrix.setRotation(1);
    matrix.drawPixel(x, y, led_state);
    matrix.writeDisplay();
  
}

char * dumpBits8( uint8_t inB ) {
  // formatted output of uint8_t (byte)  as bit pattern
  static char f8[ 9 ] = {0} ;
  for ( uint8_t i = 0 ; i < 8 ; i++ ) {
    f8[ 7 - i ] = bitRead( inB, i ) == true ? '1' : '0' ;
  }
  return &f8[0] ;
}

// https://arduino.stackexchange.com/questions/35268/5pt-font-for-neopixel-shield

void Keyestudio_8x16matrix_showdigit(int ToPrint) { // matrix display prefonfigured
  // char[] chars = ("" + i ).toCharArray();
  // num0
  // numA[i]
  int i;
  int val = ToPrint;
  char buffer [7] ; // long enough for 16 bit integer, sign, 5 digits and null.
  uint8_t z1 ; 
  uint8_t z2 ; 
  itoa(val, buffer, 10);  // value, char array, base

  
        //mydebugln(num0);  // error: no matching function for call to 'print(const unsigned char*&)' 
        // Serial.print(num0);  // error: no matching function for call to 'print(const uint8_t [5])'
        // mydebugln(Blue,White, numA[i]); //  error: no matching function for call to 'print(unsigned char*&)'
        // Serial.print(&(numA[i])); //  error: no matching function for call to 'print(uint8_t [5])'
      // char chars[] = ("" + i).toCharArray();
      //matrix.clear();
      //matrix.setRotation(1);
      //matrix.setCursor(0,0);
      //matrix4.writeDigitRaw(0,z);
      //  matrix4.writeDigitAscii(0,z);
      //matrix.print(0); 
      int offset = -5;
      int width = 4;
      for (i = 0; ( z1 = buffer[i] ) ; i++) { //exists at end of buffer
        z2 = num58[z1 - 48 ]; // drop ASCII to index pattern 0..9 
        mydebugln(Blue,White,"z1"); 
        mydebugln(Blue,White,z1); 
        matrix.drawBitmap(offset , 0,  z2  ,8, 5, true);  
        offset = offset + width ; 
      }  

}


void Keyestudio_8x16matrix_show(int ToPrint) { // matrix display prefonfigured

  Keyestudio_8x16matrix_showdigit(5);
  matrix.setTextSize(1);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setRotation(1);
  matrix.clear();
  matrix.setCursor(0,0);
  matrix.setBrightness(matrix_brightness); // Keyestudio_LEDBackpack::setBrightness(uint8_t b) 0 = very dim , 15 = bright
  if ( ToPrint < 0 ) {
      ToPrint = -1 * ToPrint;                          
      Keyestudio_8x16matrix_showdigit(ToPrint);
      mydrawPixel(1, 7, LED_OFF);  
      mydrawPixel(0, 7, LED_ON);
      mydebugln(Blue,White,"Got Negative"); 
      
    }  else {
        Keyestudio_8x16matrix_showdigit(ToPrint);
        mydrawPixel(1, 7, LED_ON);
        mydrawPixel(0, 7, LED_OFF);
    }   
  //matrix.print(ToPrint);
  //matrix.writeDisplay();
  matrix.writeDisplay();
}


void Keyestudio_8x16matrix_show(String ToPrint) { // matrix display prefonfigured
  matrix.setTextSize(1);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setRotation(1);
  matrix.clear();
  matrix.setCursor(0,0);
  matrix.print(ToPrint);
  matrix.writeDisplay();
  matrix.setRotation(0);
	
}


void Keyestudio_8x16matrix_dots(int x) {
  //matrix.setRotation(1);

    //x += 1;
    // std::bitset<8> bin_x(x);
    //mydebugln(White,Blue,x);    
    //mydebugln(White,Blue,binToStr(x));    
///    matrix.clear();
    // matrix.drawBitmap(0, 0, bintohex(binToStr(x)), 8, 16, LED_ON);
///    matrix.drawBitmap(0, 0, x, 8, 16, LED_ON);
///    matrix.writeDisplay();
  
  //delay(2000);

  matrix.setTextSize(1);//设置字体大小
  matrix.setTextWrap(true);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setRotation(1);// 显示旋转
  matrix.clear();
  matrix.setCursor(0,0);//设置字体坐标位置
  matrix.print(x);   //显示12
//  matrix.print("A"); 
  matrix.writeDisplay();
//  delay(200);
//  matrix.print("A"); 

//   x++;
//  if(x==99) x=0;


}


void Keyestudio_8x16matrix_demo() {
  //matrix.setRotation(1);

    //x += 1;
    // std::bitset<8> bin_x(x);
    //mydebugln(White,Blue,x);    
    //mydebugln(White,Blue,binToStr(x));    
    matrix.clear();
    // matrix.drawBitmap(0, 0, bintohex(binToStr(x)), 8, 16, LED_ON);
    // matrix.drawBitmap(0, 0, 0x44, 8, 16, LED_ON );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               );
    matrix.writeDisplay();
  
  //delay(2000);
/*
  matrix.setTextSize(1);//设置字体大小
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setRotation(1);// 显示旋转
  matrix.clear();
  matrix.setCursor(3,0);//设置字体坐标位置
  matrix.print(x);   //显示12
  matrix.print("A"); 
  matrix.writeDisplay();
  delay(200);
  matrix.print("A"); 
   x++;
  if(x==99) x=0;

*/
}


/*=====================================================================================
====== Stepper Motor functions 
======

#define mA_pIn1 8 // Pin CONTROLS DIRECTION // Red // digital
#define mA_pIn2 7 // Pin CONTROLS DIRECTION // Digital
#define mA_En 10 // THIS CONTROLS MOTOR SPEED 0..255 // PWM // note 9 is buzzer..
#define mB_pIn1 13 // Pin CONTROLS DIRECTION // Green // digital
#define mB_pIn2 2 // Pin CONTROLS DIRECTION // Digital // Sw2 Digital
#define mB_En 11 // THIS CONTROLS MOTOR SPEED 0..255 // PWM // note 9 is buzzer..


*/


void motorSetup() {

  pinMode(mA_pIn1, OUTPUT);
  pinMode(mA_pIn2, OUTPUT);
  pinMode(mA_En, OUTPUT);
  
  digitalWrite(mA_pIn1, LOW);
  digitalWrite(mA_pIn2, LOW);
  digitalWrite(mA_En, LOW);

  pinMode(mB_pIn1, OUTPUT);
  pinMode(mB_pIn2, OUTPUT);
  pinMode(mB_En, OUTPUT);


  digitalWrite(mB_pIn1, LOW);
  digitalWrite(mB_pIn2, LOW);
  digitalWrite(mB_En, LOW);
  
}


void motorEnableA(int MotorControllerA = 0 ) { ; // 0 = off ,  < 0  CCW Speed > 0 CW Speed
      mydrawPixel(14, 7, LED_ON);      
     //digitalWrite(IN1_PIN, !digitalRead(IN1_PIN));
     //digitalWrite(IN2_PIN, !digitalRead(IN2_PIN))
      mydebugln(Blue,White,"StepMotor Recieved:");
      mydebugln(Blue,White, MotorControllerA );
      if ( MotorControllerA == 0 ) {
              mydebugln(Blue,Green,"StepMotor OFF");
              digitalWrite(mA_pIn1, LOW);
              digitalWrite(mA_pIn2, LOW);
              digitalWrite(mA_En, LOW);  
              mA_running = false;

     } else if ( MotorControllerA  < 0  ) {
              mydebugln(Blue,Green,"StepMotor CCW Speed change");
              Keyestudio_8x16matrix_show(MotorControllerA);
              mydebugln(Blue,Green,MotorControllerA);
              digitalWrite(mA_pIn1, !mA_CW);
              digitalWrite(mA_pIn2, mA_CW);
              if ( mA_running == false ) { // kick start motor on max for 500 milliseconds 
                mydebugln(Blue,Green,"KickStart StepMotor CCW");
                mydrawPixel(4, 7, LED_ON); 
                analogWrite(mA_En,  -254); // controls the speed via the enable pin, the other two pins control direction 00,11 = off, 01=fwd,10=rev              
                delay(1500);
                mydrawPixel(4, 7, LED_OFF);
             }

              analogWrite(mA_En, -MotorControllerA); // controls the speed via the enable pin, the other two pins control direction 00,11 = off, 01=fwd,10=rev
              mA_running = true;
     } else if ( MotorControllerA  > 0  ) {
              Keyestudio_8x16matrix_show(MotorControllerA);
              mydebugln(Blue,Green,"StepMotor CW");
              mydebugln(Blue,Green,MotorControllerA);
              digitalWrite(mA_pIn1, mA_CW);
              digitalWrite(mA_pIn2, !mA_CW);
              if ( mA_running == false ) { // kick start motor on max for 500 milliseconds 
                mydebugln(Blue,Green,"KickStart StepMotor CW");
                mydrawPixel(4, 7, LED_ON);
                analogWrite(mA_En,  254); // controls the speed via the enable pin, the other two pins control direction 00,11 = off, 01=fwd,10=rev              
                delay(1500);
                mydrawPixel(4, 7, LED_OFF);

              }
              analogWrite(mA_En, MotorControllerA); // controls the speed via the enable pin, the other two pins control direction 00,11 = off, 01=fwd,10=rev
              mA_running = true;
     }
    mydrawPixel(3, 7, mA_running);    
    mydrawPixel(14, 7, LED_OFF);      
}


void motorLoop() {
  T2 = millis();
  if( (T2-T1) >= TimeInterval) // Every 5ms
  {
    // Read The Direction Control Button State
    if (debounce(reA_SW)) {
      digitalWrite(mA_pIn1, !digitalRead(mA_pIn1));
      digitalWrite(mA_pIn2, !digitalRead(mA_pIn2));
    }
    // Read The Potentiometer & Control The Motor Speed (PWM)
    
    // analogWrite(EN1_PIN, (analogRead(A0)>>2));
    T1 = millis();
  }
}

/*=====================================================================================
====== Rotary Encoder functions 
======

*/



// Rotary Encoder Inputs


void encoder_clk () {
  reA_currentStateCLK = digitalRead(reA_CLK);
  reA_currentStateDT = digitalRead(reA_DT) ;
	// Read the current state of CLK

	// If last and current state of CLK are different, then pulse occurred
	// React to only 1 state change to avoid double count
	if  (reA_currentStateCLK != reA_lastStateCLK  && reA_currentStateCLK == 1)     { //|| (currentStateDT != lastStateDT  && currentStateDT == 1) ) {
        // If the DT state is different than the CLK state then
        // the encoder is rotating CCW so decrement
        // if btnStateFlip is true, use 10 units , if false use 1 units for + and - 
        if ( (reA_currentStateDT == reA_currentStateCLK) ){
          reA_btnStateFlip ? counter -- : counter = counter - 10 ;
        } else {
          // Encoder is rotating CW so increment
          reA_btnStateFlip ? counter ++ : counter = counter + 10 ;
          
        }
        if ( counter <  0 ) { counter = 0  ; }
        else if ( counter >  254 ) { counter = 254  ; }
        /* neede if motor stalls  */
        /*
        if ( ( counter >  -100 ) && ( counter <  - 0 ) ) { counter = -100 ; }
        if ( ( counter <   100 ) && ( counter >  0 )    ) { counter = -100 ; }
*/
        NewRpmMotorA = mA_direction * counter  ;
                
        mydebug(White,Yellow,"MotorUpdateA:");
        mydebugln(NewRpmMotorA);
        Keyestudio_8x16matrix_show(NewRpmMotorA);
        if ( mA_running == true) {
          motorEnableA(NewRpmMotorA);
        } else { 
          // dont send speed to motor ; just update variable 
        }
    // LEAVE MOTOR IN CURRENT STATE EITHER RUNNING OR NOT RUNNING , JUST ADJUST SPEED motorEnableA(NewRpmMotorA);
	}
  
  // Remember last CLK state
	reA_lastStateCLK = reA_currentStateCLK;
  reA_lastStateDT = reA_currentStateDT;
	// Put in a slight delay to help debounce the reading
  delay(1);

}


void test_button () {
	// Read the button state
	reA_btnState = digitalRead(reA_SW);

	//If we detect LOW signal, button is pressed
	if (reA_btnState == LOW) {
		//if 50ms have passed since last LOW pulse, it means that the
		//button has been pressed, released and pressed again
		if (millis() - lastButtonPress > 50) {
			mydebugln(Yellow,White,"Rotary Encoder Pressed");  
			if (millis() - lastButtonPress < 300) {
		  	mydebugln(Red,White,"Rotary Encoder Double Pressed. Motor Reverse Direction");  
        mA_direction = -mA_direction;
        NewRpmMotorA = -NewRpmMotorA;
        mA_running = false; // force a kickstart
        motorEnableA(NewRpmMotorA); // pass in run value to start motor
        reA_btnStateFlip ^= true ;
			
			} else {  
        mydebugln(Red,White,"Rotary Encoder Pressed. Motor Swap On/Off");  
        reA_btnStateFlip ? motorEnableA(0)  : motorEnableA(NewRpmMotorA) ; 
        reA_btnStateFlip ^= true ;
			}
		}
		// Remember last button press event
		lastButtonPress = millis();
	}

  
  ButtonLeft_btnState = digitalRead(ButtonLeft);

	//If we detect LOW signal, button is pressed
	if (ButtonLeft_btnState == LOW) {
		//if 50ms have passed since last LOW pulse, it means that the
		//button has been pressed, released and pressed again
		if (millis() - ButtonLeft_btnState_lastButtonPress > 50) {
			mydebugln(Yellow,White,"Left Button  Pressed");  
			if (millis() - lastButtonPress < 300) {
		  	mydebugln(Red,White," Left Button Double Pressed. Motor Off");  
        NewRpmMotorA = -NewRpmMotorA;
        motorEnableA(NewRpmMotorA);
        mA_running = false;
        ButtonLeft_btnState_btnStateFlip ^= true ;
			
			} else {  
        ButtonLeft_btnState_btnStateFlip ? motorEnableA(0)  : motorEnableA(NewRpmMotorA) ; 
        ButtonLeft_btnState_btnStateFlip ^= true ;
			}
		}
		// Remember last button press event
		ButtonLeft_btnState_lastButtonPress = millis();
	}

  
    
  ButtonRight_btnState = digitalRead(ButtonRight);

	//If we detect LOW signal, button is pressed
	if (ButtonRight_btnState == LOW) {
		//if 50ms have passed since last LOW pulse, it means that the
		//button has been pressed, released and pressed again
		if (millis() - ButtonRight_btnState_lastButtonPress > 50) {
			mydebugln(Yellow,White,"Right Button  Pressed");  
			if (millis() - lastButtonPress < 300) {
		  	mydebugln(Red,White," Right Button Double Pressed. Motor Off");  
        NewRpmMotorA = -NewRpmMotorA;
        motorEnableA(NewRpmMotorA);
        mA_running = false;
        ButtonRight_btnState_btnStateFlip ^= true ;
			
			} else {  
        ButtonRight_btnState_btnStateFlip ? motorEnableA(0)  : motorEnableA(NewRpmMotorA) ; 
        ButtonRight_btnState_btnStateFlip ^= true ;
			}
		}
		// Remember last button press event
		ButtonRight_btnState_lastButtonPress = millis();
	}



}

//looses one CLICK AFTER A DOUBLE CLICK , GO FIGURE ....

void InputSetup() {
        
	// Set encoder pins as inputs
	pinMode(reA_CLK,INPUT);
	pinMode(reA_DT,INPUT ); // INPUT //INPUT_PULLDOWN
	pinMode(reA_SW, INPUT_PULLUP);
  pinMode(SPEAKER9, OUTPUT); // shutup internal speaker on ping 9


	// Read the initial state of CLK
	reA_lastStateCLK = digitalRead(reA_CLK);
  //attachInterrupt(digitalPinToInterrupt(reA_CLK), encoder_clk, HIGH) ; // |_; // not working right using interupts
  //attachInterrupt(digitalPinToInterrupt(reA_SW), test_button, HIGH) ; // |_; // not working right using interupts

  pinMode(ButtonLeft, INPUT_PULLUP);
  pinMode(ButtonRight, INPUT_PULLUP);

}


void ButtonLoop() {
  encoder_clk();
  test_button();
  delay(1);
}

 
 



/*=====================================================================================
====== Setup 
====== initial setup of values etc, fired once per power up / run 
====== 
======
C:\Users\test\Documents\PlatformIO\Projects\l298-n-testing\src\main.cpp
*/


void setup() {
  delay(1000);
  Serial.begin(230400); 
  mydebugln(White,Black,'\0');
  mydebugln(White,Green,"Start: V2 Stepper Motor and Rotary Controller Example");
  
  mydebugln(Red,White,"Keyestudio_8x16matrix_setup");
  
  Keyestudio_8x16matrix_setup();
  Keyestudio_8x16matrix_show("---");
  
  mydebugln(Yellow,Green,"inputSetup");

  InputSetup();


  mydebugln(Blue,Green,"StepMotorSetup");
  
  
  motorSetup();
  delay(4000);
   
} 



/*=====================================================================================
====== Main line, its arduino , no main instead its loop, as there is a main in android.h
====== However this file has to be main.cpp to run.. funny that
======
======
======

*/



void loop() {
  delay(1);
  // mydebugln(Red,Blue,"Start Loop");
 
//  mydebugln(White,Blue,"ButtonLoop");

  ButtonLoop();
  //mydebugln(White,Blue,"Keyestudio_8x16matrix_demo");
  //Keyestudio_8x16matrix_demo() ; 
  

 //  motorLoop();
//  mydebugln(Green,Blue,"MotorLoop");

  //StepMotorLoop();

 
} 

