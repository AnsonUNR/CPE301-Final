//Anson Wapstra Scott
//CPE301 Final Project
//Spring 2024
#include <Stepper.h>
#include <dht.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

#define DHT11_PIN 7
#define DISABLED 0
#define IDLE 1
#define ERROR 2
#define RUNNING 3
#define RDA 0x80
#define TBE 0x20 

//ADC Variables
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//UART Variables
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;


//Digital Input/Output Variables
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23;

volatile unsigned char* port_l = (unsigned char*) 0x10B; 
volatile unsigned char* ddr_l  = (unsigned char*) 0x10A; 
volatile unsigned char* pin_l  = (unsigned char*) 0x109;

volatile unsigned char* port_g = (unsigned char*) 0x34; 
volatile unsigned char* ddr_g  = (unsigned char*) 0x33; 
volatile unsigned char* pin_g  = (unsigned char*) 0x32;

volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a  = (unsigned char*) 0x21; 
volatile unsigned char* pin_a  = (unsigned char*) 0x20;

volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
volatile unsigned char* pin_d  = (unsigned char*) 0x29;



const int stepsPerRevolution = 2038;

Stepper myStepper = Stepper(stepsPerRevolution, 31, 35, 33, 37);
dht DHT; 

int sensorValue = 0;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

volatile int fan_state = 0;
int cooler_state = DISABLED;
int previous_state = DISABLED;

int tempThreshold = 24;
int waterThreshold = 50;
unsigned int water_level;

unsigned long previousMillis = 0;
const long interval = 60000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);

  //initialize the ADC for the Water Level Sensor
  adc_init();

  

  //INPUT and OUTPUT
  *ddr_g |= 0x04; //Blue LED Pin 39/PG2 output
  *ddr_g |= 0x01; // RED LED Pin 41/PG0 output
  *ddr_l |= 0x40; //GREEN LED Pin 43/PL6 output
  *ddr_l |= 0x10; //YEllow LED Pin 45/PL4 output

  *ddr_b &= 0xFD; //right button Pin 52/PB1 input
  *ddr_b &= 0xF7; //left button Pin 50/PB3 input
  *ddr_l &= 0xFD; //reset button Pin 48/PL1 input
  *ddr_d &= 0xFB; //on and off button Pin 19/PD2 input

  *ddr_a |= 0x04;; //fan/motor Pin 24/PA2 output

  //Sets Pin 19/PD2, the On and Off Button, to trigger an interrupt
  //attachInterrupt(digitalPinToInterrupt(19), startButtonISR, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:

  //DISABLED State
  if(cooler_state == DISABLED){
    if(previous_state != DISABLED){
      previous_state = DISABLED;
      reportTransition();
    }
      
    
    *port_g &= 0xFB; //blue off
    *port_g &= 0xFE; //red off
    *port_l &= 0xBF; //green off
    *port_l |= 0x10; //yellow on

    fan_state = 0;
    lcd.clear();

    if(*pin_d & 0x04){
      cooler_state = IDLE;
      delay(1000);
    } //polling version of start button
  }

  //IDLE State
  else if(cooler_state == IDLE){
    if(previous_state != IDLE){
      previous_state = IDLE;
      reportTransition();
    }

    *port_g &= 0xFB; //blue off
    *port_g &= 0xFE; //red off
    *port_l |= 0x40; //green on
    *port_l &= 0xEF; //yellow off
    fan_state = 0;

    water_level = adc_read(0);
    DHT.read11(DHT11_PIN);
    if(water_level <= waterThreshold){
      cooler_state = ERROR;
    }
    
    else if(DHT.temperature > tempThreshold){
      cooler_state = RUNNING;
    }

    if(*pin_d & 0x04){
      cooler_state = DISABLED;
      delay(1000);
    } //polling version of start button
    
  }

  //ERROR State
  else if(cooler_state == ERROR){
    if(previous_state != ERROR){
      previous_state = ERROR;
      reportTransition();
    }

    *port_g &= 0xFB; //blue off
    *port_g |= 0x01; //red on
    *port_l &= 0xBF; //green off
    *port_l &= 0xEF; //yellow off

    fan_state = 0;
    water_level = adc_read(0);
    if(*pin_l & 0x02){ //if pin 48 (RESET) is high
      if(water_level > waterThreshold){
        cooler_state = IDLE;
      }
    }
    if(*pin_d & 0x04){
      cooler_state = DISABLED;
      delay(1000);
    } //polling version of start button
    //LCD should ERROR
  }

  //RUNNING State
  else if(cooler_state == RUNNING){
    if(previous_state != RUNNING){
      previous_state = RUNNING;
      reportTransition();;
    }

    *port_g |= 0x04; //blue on
    *port_g &= 0xFE; //red off
    *port_l &= 0xBF; //green off
    *port_l &= 0xEF; //yellow off

    fan_state = 1;
    water_level = adc_read(0);
    DHT.read11(DHT11_PIN);
    if(water_level <= waterThreshold){
      cooler_state = ERROR;
    }
    
    else if(DHT.temperature <= tempThreshold){
      cooler_state = IDLE;
    }
    if(*pin_d & 0x04){
      cooler_state = DISABLED;
      delay(1000);
    } //polling version of start button

    
  }

  unsigned long currentMillis = millis();

  if(cooler_state != DISABLED){
    if(currentMillis - previousMillis >= interval){
      previousMillis = currentMillis;
      //lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Temp:");
    
      lcd.print(DHT.temperature);
    }
    
  }

  if(fan_state == 1)
    *port_a |= 0x04;
  else
    *port_a &= 0xFB;

  
  //RTC test
  
  /*tmElements_t time;
  RTC.read(time);
  Serial.print("time: ");
  Serial.print(time.Hour);
  Serial.print(":");
  Serial.print(time.Minute);
  Serial.print(":");
  Serial.println(time.Second); */

  //Water sensor test code
  /*unsigned int water_level = adc_read(0);
  Serial.print("Water Level: ");
  Serial.println(water_level);*/


  //Stepper sample code
  /*
  myStepper.setSpeed(5);
  //myStepper.step(stepsPerRevolution);
  delay(1000);
  // Rotate CCW quickly at 10 RPM
  myStepper.setSpeed(10);
  //myStepper.step(-stepsPerRevolution);
  delay(1000);*/

  //DHT humidity and temp sample code
  /*
  int chk = DHT.read11(DHT11_PIN);
  Serial.print("Temperature=");
  Serial.println(DHT.temperature);
  //Serial.print("Humidity=");
  //Serial.println(DHT.humidity);
  delay(1000);*/

  //lcd test code
  
  /*lcd.setCursor(0, 0);
  lcd.write("test");
  delay(1000);*/

  /*//Button and LED test code
  if(*pin_b & 0x02) //if pin 52 is high
    Serial.println("right pressed");
  if(*pin_b & 0x08) //if pin 50 is high
    Serial.println("left pressed");
  if(*pin_l & 0x02) //if pin 48 is high
    Serial.println("reset pressed");*/

    

  /*
  //all LEDs on
  *port_g |= 0x04; //blue on
  *port_g |= 0x01; //red on
  *port_l |= 0x40; //green on
  *port_l |= 0x10; //yellow on
  
  //LEDs off
  *port_g &= 0xFB; //blue off
  *port_g &= 0xFE; //red off
  *port_l &= 0xBF; //green off
  *port_l &= 0xEF; //yellow off
  */
  
    //Turns the fan on or off depending on its state
    
  
}

void startButtonISR(){
    if(cooler_state == previous_state){
      if(cooler_state == DISABLED){
        cooler_state = IDLE;
      }
      else if(cooler_state == IDLE){
        cooler_state = DISABLED;
      }
      else if(cooler_state == ERROR){
        cooler_state = DISABLED;
      }
      else if(cooler_state == RUNNING){
        cooler_state = DISABLED;
      }
    }
}

void reportTransition(){
    Serial.println("transition placeholder");
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}
void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

