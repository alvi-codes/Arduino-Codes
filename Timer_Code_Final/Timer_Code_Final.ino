#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int Pause_pin = 7;
const int Resume_pin = 8;

void setup() {
  pinMode(13, OUTPUT);
  pinMode(Pause_pin, INPUT);
  pinMode(Resume_pin, INPUT);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Time:");
  lcd.setCursor(10, 1);
  lcd.print(":");
  lcd.setCursor(13, 1);
  lcd.print(":");
}

void infinte_delay() {
  int buttonState = LOW;
  do {
    buttonState = digitalRead(Resume_pin);
   } while (buttonState != HIGH);  
   lcd.setCursor(5, 0);
   lcd.print("       ");
}

 
void loop() {
  int buttonState = LOW;
  digitalWrite(13, HIGH);
  for (int h = 0; h < 24; h++){ 
    lcd.setCursor(8, 1);
    lcd.print(h);
    for (int m = 0; m < 60; m++){
           lcd.setCursor(11, 1);
           lcd.print(m);
           for (int s = 0; s < 60; s++){
                lcd.setCursor(14, 1);
                lcd.print(s);
                int buttonState = digitalRead(Pause_pin);
                if (buttonState == HIGH){
                  lcd.setCursor(5, 0);
                  lcd.print("PAUSED");
                  infinte_delay();
                }     
                delay(1000);
                if (s == 59){
                  lcd.setCursor(14, 1);
                  lcd.print("  ");
                }
            }
            if (m == 59){
            lcd.setCursor(11, 1);
            lcd.print("  ");
            }
     }   
     if (h == 23){
     lcd.setCursor(11, 1);
     lcd.print("  ");
     }
  }
}
