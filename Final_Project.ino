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

volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23;

const int stepsPerRevolution = 2038;

Stepper myStepper = Stepper(stepsPerRevolution, 31, 35, 33, 37);
dht DHT; 

int sensorValue = 0;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int fan_state = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);

  //initialize the ADC for the Water Level Sensor
  adc_init();

  //set PB1 to input (UNUSED RN)
  *ddr_b &= 0xFD;

  //CHANGE THESE TO NOT USE THE LIBRARY
  pinMode(39, OUTPUT); //Blue
  pinMode(41, OUTPUT); // RED
  pinMode(43, OUTPUT); //GREEN
  pinMode(45, OUTPUT); //YEllow

  pinMode(52, INPUT); //right
  pinMode(50, INPUT); //left
  pinMode(48, INPUT); //reset
  pinMode(46, INPUT); //on and off

  pinMode(24, OUTPUT); //fan

  
}

void loop() {
  // put your main code here, to run repeatedly:

  //RTC test
  /*
  tmElements_t time;
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
  
  lcd.setCursor(0, 0);
  lcd.write("test");
  delay(1000);

  //Button and LED test code
  if(digitalRead(52) == HIGH)
    Serial.println("right pressed");
  if(digitalRead(50) == HIGH)
    Serial.println("left pressed");
  if(digitalRead(48) == HIGH)
    Serial.println("reset pressed");
  if(digitalRead(46) == HIGH){
    Serial.println("on/off pressed");
    fan_state = 1 - fan_state;
    digitalWrite(24, fan_state);
  }
    


  //all LEDs on
  digitalWrite(39, HIGH);
  digitalWrite(41, HIGH);
  digitalWrite(43, HIGH);
  digitalWrite(45, HIGH);
  

  

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


