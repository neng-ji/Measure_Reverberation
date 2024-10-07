//Include libraries
#include "Wire.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "iostream"
#include "string"
#include "math.h"
#include "array"

//Define keypad and amplifier boards
#define MAX9744_I2CADDR 0x4B
#define KEYPAD_PID1824

//Define constants for the DAC readout
#define DACpin A0
#define MAXDAC 1023
#define DACVREF 3.29
#define number_per_period 180
#define dtheta_radians 0.034906585
#define number_delay_values_to_consider 50

//float theta_radians[number_per_period];
//float sin_values[number_per_period];
int DAC_values[number_per_period];
float frequency_vs_usec_delay[number_delay_values_to_consider];
bool sine_generator_initiaized = false;


//////////////////////////////////////////////////////////////////////////
//Stuff for bluetooth capabilities
//Set up name of Bluetooth LE device
#define SETBLUETOOTHNAME  "AT+GAPDEVNAME=_P523_tone_generator"
#define name_start 14

#define BUFSIZE 160
#define VERBOSE_MODE true

#define BLUEFRUIT_SPI_CS            A5
#define BLUEFRUIT_SPI_IRQ           A4
#define BLUEFRUIT_SPI_RST           A3
#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ,BLUEFRUIT_SPI_RST);

///////////////////   LCD stuff   //////////////////////

// RadioFeather M0 processor pin numbers for LCD:
const int rs = 18, en = 11, d4 = 5, d5 = 6, d6 = 9, d7 = 19;

// GCM4
// const int rs = 4, en = 22, d4 = 36, d5 = 34, d6 = 32, d7 = 30;

//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// carriage return is 0x0A.
#define ASCII_CR 0x0A

// useful(???) utility variable
int LCD_char_index = 0;

/////////////////// iPhone stuff //////////////////////

char iPhone_incoming[BUFSIZE+1];
int iphone_incoming_index = 0;
char iPhone_last_message[BUFSIZE+1];
int iPhone_last_message_length = 0;

bool start_recording = false;

void setup() {
  //Serial.begin(19200);
  uint32_t t1234 = millis();
  //while (!Serial && millis() - t1234 < 10000) {}
  
  //Initialize bluetooth
  if ( !ble.begin(VERBOSE_MODE) )
  {
    //error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    //Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset())
    {
      //error(F("Couldn't factory reset"));
    }
  }

  ble.echo(false);
  ble.println(SETBLUETOOTHNAME);
  for(int ijk = name_start; ijk < name_start + 16; ijk++)
  {
    //lcd.print(SETBLUETOOTHNAME[ijk]);
    if(SETBLUETOOTHNAME[ijk] == '\0') break;
  }
  ble.setMode(BLUEFRUIT_MODE_DATA);

  generate_DAC_sine(100, 0.10, 0.5);
  //while(!Serial){}
}

//This section of our program was 
//contributed by Prof. George Gollin, UIUC
int generate_DAC_sine(float f_Hz, float duration_seconds, float exponential_decay_time) {
  // generate a sine of approximate frequence f_Hz for an amount
  // of time given by duration_seconds. Allow user to specify an exponential
  // decay time (in seconds)to the sine.

  // some timing utility variables
  uint32_t t0, t1;

  // do I need to initialize my arrays?
  if(!sine_generator_initiaized)
  {
    sine_generator_initiaized = true;
    //Serial.println("Now initializing the sine generator");

    // how many different delay values to look at
    int max_delay_val = number_delay_values_to_consider + 1;

    // load the DAC values array for one period

    float theta_load = 0.;
    for(int i = 0; i < number_per_period; i++)
    {
      // DAC data is from 0 to 1023, and is never negative.
      DAC_values[i] = floor((1. + sin(theta_load)) * MAXDAC / 2.0);
      //Serial.println(DAC_values[i]);
      theta_load = theta_load + dtheta_radians;
    }

    // how many sine periods to generate, to calulate the frequencies...
    int n_times = 10;
    int ijk = -1;

    // we'll generate 10 periods, see how long that takes.
    for(int this_delay_value = 0; this_delay_value < max_delay_val; this_delay_value++)
    {
      ijk++;
      t0 = micros();
      for(int period_number = 0; period_number < n_times; period_number++)
      {
        for(int i = 0; i < number_per_period; i++)
        {
          analogWrite(DACpin, DAC_values[i]);
          delayMicroseconds(this_delay_value);
        }
      }
      t1 = micros();
      frequency_vs_usec_delay[ijk] = 1000000. * float(n_times) / float(t1 - t0);
    }
  }
  // and that's the end of the sine generator intialization code!
  //////////////
  
  // now find the closest possible frequency to what the user's requested.
  float last_df = 9999.;
  float df;
  int my_bin = 0;

  for (int if_bin = 0; if_bin < number_delay_values_to_consider; if_bin++) 
  {
    // how far from the desired frequency is this particular frequency?
    df = abs(f_Hz - frequency_vs_usec_delay[if_bin]);
    // is it closer than the previous one we chacked?
    if(df < last_df)
    {
      last_df = df;
      my_bin = if_bin;
    }
  }
  uint32_t number_to_do = duration_seconds * frequency_vs_usec_delay[my_bin];
  ble.print("Closest generatable frequency is: ");  
  ble.print(frequency_vs_usec_delay[my_bin]);
  ble.println(" Hz");
  
  // only recalculate the exponential damping immediately after each period.
  float sine_period = 1. / frequency_vs_usec_delay[my_bin];
  // damping factor for one period
  float damping_per_period = exp(-sine_period /(5 * exponential_decay_time));
  float damping_now = 1.0;
  int DAC_val_with_damping;

  for(uint32_t period_number = 0; period_number < number_to_do; period_number++)
  {
    for(int i = 0; i < number_per_period; i++)
    {
      DAC_val_with_damping = int(DAC_values[i]);
      analogWrite(DACpin, DAC_val_with_damping);
      delayMicroseconds(my_bin);
    }
  }
  // for(uint32_t period_number = 0; period_number < number_to_do/2; period_number++)
  // {
  //   for(int i = 0; i < number_per_period; i++)
  //   {
  //     DAC_val_with_damping = int(DAC_values[i] * damping_now); 
  //     analogWrite(DACpin, DAC_val_with_damping);
  //     delayMicroseconds(my_bin);
  //     damping_now = damping_now * damping_per_period;
  //     Serial.println(damping_now);
  //   }
  // }
  return 0;
}

void loop() {-
  ble.println(); ble.println(); ble.println();
  //Specify frequency (Hz)...
  //Serial.println("Input desired frequency (Hz)..."); 
  ble.println("Input desired frequency (Hz)..."); 

  // Check for user input from the serial monitor input field
  char n, inputs[BUFSIZE+1];
  int f = 0;

  // Echo received-from-iPhone data
  while (f == 0){
    while (ble.available())
    {
      int c = ble.read();
      iPhone_incoming[iphone_incoming_index] = (char)c;
      iphone_incoming_index++;
      if(iphone_incoming_index > BUFSIZE) {iphone_incoming_index = 0;}

      iPhone_last_message[iPhone_last_message_length] = (char)c;
      iPhone_last_message_length++;
      if(iPhone_last_message_length > BUFSIZE) {iPhone_last_message_length = 0;}

      if(c == ASCII_CR)
      {
        //Serial.print("Just saw an iPhone message: '");

        // add a null after the message, where the carriage return is.
        iPhone_last_message[iPhone_last_message_length - 1] = '\0';

        //Serial.println(iPhone_last_message);
        //f = iPhone_last_message; //atoi(iPhone_last_message);
        //char message = iPhone_last_message;
        //Serial.println("'");
        iPhone_last_message_length = 0;
        f = 1;
      }
    } 
  }
  

  //Generate the sine wave...
  int frequencies[4] = {0, 440, 440, 440};
  int t_init = micros();
  for (int j = 1; j <= 2; j++){  
    for (int i = 1; i <= 1; i++) {  
      //Generate a sine wave with the specified frequency...
      float duration_seconds = 3.00;
      float exponential_decay_time = 0.01;
      digitalWrite(13, HIGH);
      generate_DAC_sine(frequencies[j], duration_seconds, exponential_decay_time);
      digitalWrite(13, LOW);
      //while (micros() - t_init < (duration_seconds*1000000*(j - 1)*5*2 + i*duration_seconds*1000000*7.5)){}
      //while (micros() - t_init < (1000000*(j-1)*5*7.5 + i*1000000*7.5)){}
      while (micros() - t_init < 1000000*j*9){}
    }
  }
  //Wait a second before starting the loop again...
  delay(2000);
}

// A small helper
void error(const __FlashStringHelper*err) {
  //Serial.println(err);
  while (1);
}