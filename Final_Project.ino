//Anson Wapstra Scott
//CPE301 Final Project
//Spring 2024
#include <Stepper.h>
#include <dht.h>
#include <LiquidCrystal.h>

#define DHT11_PIN 7

const int stepsPerRevolution = 2038;

Stepper myStepper = Stepper(stepsPerRevolution, 47, 51, 49, 53);
dht DHT; 

int sensorValue = 0;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //Stepper sample code
  myStepper.setSpeed(5);
  myStepper.step(stepsPerRevolution);
  delay(1000);
  // Rotate CCW quickly at 10 RPM
  myStepper.setSpeed(10);
  myStepper.step(-stepsPerRevolution);
  delay(1000);

  //DHT humidity and temp sample code
  int chk = DHT.read11(DHT11_PIN);
  Serial.print("Temperature=");
  Serial.println(DHT.temperature);
  //Serial.print("Humidity=");
  //Serial.println(DHT.humidity);
  delay(1000);

  //lcd test code
  lcd.setCursor(0, 0);
  lcd.write("test");
  delay(1000);

  

}


