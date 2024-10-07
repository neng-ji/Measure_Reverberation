#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
// #include <SD.h>
#include "Adafruit_FRAM_SPI.h"
#include <sstream>
#include <iostream>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include <LiquidCrystal.h>


#include <SdFat.h>                // SD card & FAT filesystem library
using namespace std;



//---------- FFT ----------
#include "arduinoFFT.h"
arduinoFFT FFT;
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03



//---------- SD CARD ----------

SdFat SD;
// set up variables using the SD utility library functions:
File myFile;
// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
#define SD_SPI_CS 9// OLD: 12; 4 for adalogger



//---------- BLUETOOTH ----------

// here is the name to give to this particular BluetoothLE device,
// including the AT command that sets the (new) device name.
// Pick one...
// #define SETBLUETOOTHNAME  "AT+GAPDEVNAME=_P523_tone_generator"
#define SETBLUETOOTHNAME  "AT+GAPDEVNAME=_P523_mic1"
// #define SETBLUETOOTHNAME  "AT+GAPDEVNAME=_P523_mic2"
// #define SETBLUETOOTHNAME  "AT+GAPDEVNAME=_P523_mic3"
// #define SETBLUETOOTHNAME  "AT+GAPDEVNAME=_P523_mic4"

#define name_start 14

// Size of the read buffer for incoming data
#define BUFSIZE 160   

// If set to 'true' enables debug output
#define VERBOSE_MODE  true  

// Feather M4 pins
#define BLUEFRUIT_SPI_CS               A5 // OLD: 10
#define BLUEFRUIT_SPI_IRQ              A4 // OLD: 5
#define BLUEFRUIT_SPI_RST              A3 // OLD: 6 

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ,
BLUEFRUIT_SPI_RST);



//---------- ANALOG INPUTS ----------
int analogPin = A0;

//---------- LED pins ----------
#define RED_LED_PIN 13
int LED_pin = RED_LED_PIN;
#define LED_OFF LOW
#define LED_ON HIGH



//---------- LCD ---------- (not on board)

// // RadioFeather M0 processor pin numbers for LCD:
// const int rs = 18, en = 11, d4 = 15, d5 = 16, d6 = 9, d7 = 19;

// // GCM4
// // const int rs = 4, en = 22, d4 = 36, d5 = 34, d6 = 32, d7 = 30;

// LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// // carriage return is 0x0A.
#define ASCII_CR 0x0A

// // useful(???) utility variable
// int LCD_char_index = 0;

//---------- iPhone stuff ----------

char iPhone_incoming[BUFSIZE+1];
int iphone_incoming_index = 0;
char iPhone_last_message[BUFSIZE+1];
int iPhone_last_message_length = 0;


//---------- MAIN ----------

void setup(void){

  Serial.begin(115200);
  uint32_t time_start = millis();
  while (!Serial && millis() - time_start < 10000) {}
  Serial.println(F("STARTING KCPA V1.2"));

  analogReadResolution(12);


  //---------- SD card setup ----------
  Serial.print("\nInitializing SD card...");
  delay(200);
  if (!SD.begin(SD_SPI_CS,SD_SCK_MHZ(5))) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  SD.remove("data.txt");
  Serial.println("1");
  myFile = SD.open("data.txt", FILE_WRITE);
  Serial.println("1");
  myFile.print("raw data");
  Serial.println("1");
  myFile.println("");
  Serial.println("1");
  myFile.close();  


  //---------- BLUETOOTH ----------
  Serial.println("\n \n *****Bluetooth Setup*****");
  Serial.println(F("file holding this program is ")); 
  Serial.println(__FILE__);
  Serial.print(F("System date and time of program compilation: "));
  Serial.print(__DATE__);
  Serial.print(F("  ")); 
  Serial.println(__TIME__);
  Serial.println("This program is executing on a Feather M4.");

  // set the LED-illuminating pin to be a digital output.
  pinMode(LED_pin, OUTPUT);
  // make sure the LED is off.
  digitalWrite(LED_pin, LED_OFF);

  /* Initialise the BLE module */
  Serial.print(F("Initialising the Bluefruit LE module: "));
  if ( !ble.begin(VERBOSE_MODE) ){
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() )
    {
      error(F("Couldn't factory reset"));
    }
  }

  // Disable command echo from Bluefruit
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println();

  // debug info is a little annoying after this point!
  ble.verbose(false);  

  // Wait for connection to iPhone
  Serial.println("waiting for connection...");
  while (! ble.isConnected()) {delay(500);}
  Serial.println("now BLE is connected.");

  // now try changing the name of the BLE device.
  Serial.println("About to change bluetoothLE module name via the command\n   ");
  Serial.println(SETBLUETOOTHNAME);
  ble.println(SETBLUETOOTHNAME);

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  // see Adafruit_BluefruitLE_SPI.cpp.
  // mode choices are  BLUEFRUIT_MODE_COMMAND and BLUEFRUIT_MODE_DATA

  Serial.println("Setup Complete");
  ble.print("Setup Complete");

}

int startTime = micros();
const int size = 86000;
double freq = 35000;
int run = 0;

void loop(void) {
  // Check for user input from the serial monitor input field
  char n, inputs[BUFSIZE+1];
  
  if (Serial.available()){
    n = Serial.readBytes(inputs, BUFSIZE);
    // inputs[n] = 0;
    // Send characters to Bluefruit
    Serial.print("Sending to iPhone: ");
    Serial.println(inputs);

    // Send input data to host via Bluefruit
    ble.print(inputs);
  }

  // Echo received-from-iPhone data
  while (ble.available()){
    int c = ble.read();

    // Serial.print((char)c);

    iPhone_incoming[iphone_incoming_index] = (char)c;
    iphone_incoming_index++;
    if(iphone_incoming_index > BUFSIZE) {iphone_incoming_index = 0;}

    iPhone_last_message[iPhone_last_message_length] = (char)c;
    iPhone_last_message_length++;
    if(iPhone_last_message_length > BUFSIZE) {iPhone_last_message_length = 0;}

    if(c == ASCII_CR){
      Serial.print("Command: '");

      // add a null after the message, where the carriage return is.
      iPhone_last_message[iPhone_last_message_length - 1] = '\0';

      Serial.print(iPhone_last_message);
      Serial.println("'");
      iPhone_last_message_length = 0;

      if(is_number(iPhone_last_message)){ // manual data taking
            int run_start = micros();
            for(int i=1; i<16; i++){
              takeData(extractIntegerWords(iPhone_last_message), i);
              while (micros()-run_start < 7000000*i){}
            }
         } 

      else if (iPhone_last_message[0] == 's' && iPhone_last_message[1] == 'e' && iPhone_last_message[2] == 't'){ // set run number
            run = extractIntegerWords(iPhone_last_message);
            ble.println("Set run number");
           } 

      else if (iPhone_last_message[0] == 'r' && iPhone_last_message[1] == 'u' && iPhone_last_message[2] == 'n'){ // run the default set
            int run_start = micros();
            for(int i=1; i<6; i++){
              delay(1000);
              takeData(330, i);
              while (micros()-run_start < 7500000*i){}
            }
            for(int i=1; i<6; i++){
              delay(1000);
              takeData(440, i);
              while (micros()-run_start < 7500000*5 + 7500000*i){}
            }
            for(int i=1; i<6; i++){
              delay(1000);
              takeData(550, i);
              while (micros()-run_start < 7500000*10 + 7500000*i){}
            }
           } 

      else if (iPhone_last_message[0] == 'c' && iPhone_last_message[1] == 'd'){ // easter egg
            ble.println("This is an easter egg");
           } 

      else{ 
            Serial.println("unrecognized command");
            ble.println("unrecognized command");
         }
    }
  }
}



//---------- A small helper ----------
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


//---------- Blink the red LED ----------
void blink_LED(uint32_t blink_ms){
  // blink the LED for the specofied number of milliseconds

  uint32_t start_time = millis();

  // make sure the LED is off, initially.
  digitalWrite(LED_pin, LED_OFF);

  while(millis() - start_time < blink_ms)
  {
    digitalWrite(LED_pin, LED_ON);
    delay(50);
    digitalWrite(LED_pin, LED_OFF);
    delay(50);
  }
}

//---------- Check if string is a number ----------
bool is_number(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

//---------- get the first number from a string ----------
int extractIntegerWords(string str){
  stringstream ss;

  /* Storing the whole string into string stream */
  ss << str;

  /* Running loop till the end of the stream */
  string temp;
  int found;
  while (!ss.eof()) {
      /* extracting word by word from stream */
      ss >> temp;

      /* Checking the given word is integer or not */
      if (stringstream(temp) >> found){
          return found;
      }
    }
  return 0;
}

//---------- data taking and saving ----------
void takeData (double run_freq, int run){
  uint16_t data[size];
  ble.println("starting data taking");    
  // wait for sound to start up  
  blink_LED(300);
  digitalWrite(LED_pin, HIGH);

  // take the data
  startTime = micros();
  for (int i = 0; i<size; i++){
    data[i] = analogRead(A0);
  }
  freq = size/((micros() - startTime) * 0.000001);

  // save the data
  stringstream stream;
  stream<<run_freq<<" "<<run;
  string str1, str2;
  stream>>str1>>str2;
  string title = str2 + "a" + str1 +".txt";
  Serial.println(title.c_str());
  SD.remove(title.c_str());
  myFile = SD.open(title.c_str(), FILE_WRITE);
  if(myFile){
    myFile.println("raw data");
    for (int j=0; j<size;j++){
      myFile.println(data[j]);
    }
    myFile.println(freq);
    myFile.close();
    ble.println("data saved");
    // blink_LED(100);
  }
  else{
    ble.println("Error saving data");
  }
  digitalWrite(LED_pin, LOW);
}

//---------- Data analysis ----------
bool analyze(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}