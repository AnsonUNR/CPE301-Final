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

char *states[] = {"DISABLED", "IDLE", "ERROR", "RUNNING"};
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

Stepper myStepper = Stepper(200, 31, 35, 33, 37);


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

  myStepper.setSpeed(20); //Sets stepper speed

  //INPUT and OUTPUT
  *ddr_g |= 0x04; //Blue LED Pin 39/PG2 output
  *ddr_g |= 0x01; // RED LED Pin 41/PG0 output
  *ddr_l |= 0x40; //GREEN LED Pin 43/PL6 output
  *ddr_l |= 0x10; //YEllow LED Pin 45/PL4 output

  *ddr_b &= 0xFD; //right button Pin 52/PB1 input
  *ddr_b &= 0xF7; //left button Pin 50/PB3 input
  *ddr_l &= 0xFD; //reset button Pin 48/PL1 input
  *ddr_l &= 0xF7; //Off button Pin 46/PL3 input
  *ddr_d &= 0xFB; //on  button Pin 19/PD2 input

  *ddr_a |= 0x04;; //fan/motor Pin 24/PA2 output

  //Sets Pin 19/PD2, the On Button, to trigger an interrupt
  attachInterrupt(digitalPinToInterrupt(19), startButtonISR, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:

  //DISABLED State
  if(cooler_state == DISABLED){
    if(previous_state != DISABLED){
      reportTransition();
      previous_state = DISABLED;
      lcd.clear();
    }
      
    
    *port_g &= 0xFB; //blue off
    *port_g &= 0xFE; //red off
    *port_l &= 0xBF; //green off
    *port_l |= 0x10; //yellow on

    fan_state = 0;
  }

  //IDLE State
  else if(cooler_state == IDLE){
    if(previous_state != IDLE){
      reportTransition();
      if(previous_state != RUNNING)
        reportDHT();
      previous_state = IDLE;
      
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
      reportDHT();
    }

    if(*pin_l & 0x08){//if off button (pin46) is high
      cooler_state = DISABLED;
      
    } 
    
  }

  //ERROR State
  else if(cooler_state == ERROR){
    if(previous_state != ERROR){
      reportTransition();
      previous_state = ERROR;
      lcd.clear();
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
        
        reportDHT();
      }
    }
    if(*pin_l & 0x08){//if off button (pin46) is high
      cooler_state = DISABLED;
      
    }
  }

  //RUNNING State
  else if(cooler_state == RUNNING){
    if(previous_state != RUNNING){
      reportTransition();
      previous_state = RUNNING;
      
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
      reportDHT();
    }
    if(*pin_l & 0x08){//if off button (pin46) is high
      cooler_state = DISABLED;
      
    }
    
  }

  unsigned long currentMillis = millis();

  if(cooler_state != DISABLED){
    if(currentMillis - previousMillis >= interval){
      previousMillis = currentMillis;
      if(cooler_state != ERROR){
        
        reportDHT();
      }
      
    }
    //print error message if water level is too low
    if(cooler_state == ERROR){
        
        lcd.setCursor(0, 0);
        lcd.write("Error:Water Low.");
      }
    }
  
  bool movedRight=false;
  bool movedLeft=false;
  //stepper motor
  while(*pin_b & 0x02){ //while pin 52(right) is high
    myStepper.step(-1);
    movedRight = true;
  }
  if(movedRight){
    printMessage("Vent Moved Clockwise\n");
  }
  while(*pin_b & 0x08){ //while pin 50(left) is high
    myStepper.step(1);
    movedLeft = true;
  }
  if(movedLeft){
    printMessage("Vent Moved Counter Clockwise\n");
  }

  if(fan_state == 1)
    *port_a |= 0x04;
  else
    *port_a &= 0xFB;
  
}

void startButtonISR(){
    if(cooler_state == previous_state){
      if(cooler_state == DISABLED){
        cooler_state = IDLE;
      }
    }
}

void reportDHT(){
  
  lcd.clear();
  DHT.read11(DHT11_PIN);
  lcd.setCursor(0, 0);
  lcd.write("T:");
  lcd.print(DHT.temperature);
  //lcd.setCursor(0, 1); //uncomment this if you want Humidity on its own line
  lcd.write(" H:");
  lcd.print(DHT.humidity);
}

void reportTransition(){
    tmElements_t time;
    RTC.read(time);

    printMessage("Transition from ");
    printMessage(states[previous_state]);
    printMessage(" to ");
    printMessage(states[cooler_state]);
    printMessage(" on ");

    unsigned char month[2];
    sprintf(month, "%u", time.Month);
    printMessage(month);
    printMessage("/");
    unsigned char day[2];
    sprintf(day, "%u", time.Day);
    printMessage(day);
    printMessage("/");
    unsigned char year[4];
    sprintf(year, "%u", tmYearToCalendar(time.Year));
    printMessage(year);

    printMessage(" at ");

    unsigned char hour[2];
    sprintf(hour, "%02u", time.Hour);
    printMessage(hour);
    printMessage(":");
    unsigned char minute[2];
    sprintf(minute, "%02u", time.Minute);
    printMessage(minute);
    printMessage(":");
    unsigned char second[2];
    sprintf(second, "%02u", time.Second);
    printMessage(second);
    U0putchar('\n');
    
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

void printMessage(char* message){
  for(int i = 0; i < strlen(message); i++){
    U0putchar(message[i]);
  }
  
}

