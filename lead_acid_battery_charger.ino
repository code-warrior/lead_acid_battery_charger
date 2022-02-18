//-------© Electronics-Project-hub-------//
#include "ACS712.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define sensorInput A0
ACS712 sensor(ACS712_05B, sensorInput);

//------ Time out Setting --------//
int h_lt = 7; // in hrs
int m_lt = 0; // in min
// -------------------------------//

const int ok = 3;
int address = 0;
int batt_cap;
int current_lt = 0;
float peak_I_lt = 0;
float cut_off = 0;
boolean set_batt = true;
boolean var = true;
int i = 0;
int hrs = 0;
int Min = 0;
int sec = 0;
float currentReading;
float CV_current = 0;
void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(INCREMENT_CAPACITY_BUTTON, INPUT_PULLUP);
  pinMode(ok, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  EEPROM.get(address, batt_cap);
  if (batt_cap < 4500)
  {
    EEPROM.put(address, 4500);
  }
  lcd.clear();
  while (set_batt)
  {
    lcd.setCursor(0, 0);
    lcd.print("Enter capacity:");
    lcd.setCursor(0, 1);
    EEPROM.get(address, batt_cap);
    lcd.print(batt_cap);
    lcd.print(" mAh");
    if (digitalRead(INCREMENT_CAPACITY_BUTTON) == LOW)
    {
      while (var)
      {
        if (digitalRead(ok) == LOW) var = false;
        if (digitalRead(INCREMENT_CAPACITY_BUTTON) == LOW)
        {
          lcd.setCursor(0, 1);
          batt_cap = batt_cap + 500;
          if (batt_cap > 15000)
          {
            batt_cap = 4500;
            lcd.clear();
          }
          lcd.setCursor(0, 0);
          lcd.print("Enter capacity:");
          lcd.setCursor(0, 1);
          lcd.print(batt_cap);
          lcd.print(" mAh");
          delay(250);
        }
      }
    }
    if (digitalRead(ok) == LOW)
    {
      EEPROM.put(address, batt_cap);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Your battery");
      lcd.setCursor(0, 1);
      lcd.print("is ");
      lcd.print(batt_cap);
      lcd.print(" mAh.");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set current");
      lcd.setCursor(0, 1);
      lcd.print("limit = ");
      //------- Charging Parameters ----------//
      current_lt = batt_cap * 0.2;
      peak_I_lt = batt_cap * 0.3 * 0.001;
      cut_off = batt_cap * 0.04 * 0.001; 
      //-------------------------------------//
      lcd.print(current_lt);
      lcd.print(" mA");
      delay(3000);
      set_batt = false;
    }
  }
  current_calib();
  CCCV();
}

void loop()
{
  for (i = 0; i < 10; i++)
  {
    currentReading = sensor.getCurrentDC();
    delay(100);
  }
  timer();
  lcd.clear();
  lcd.setCursor(0, 0);
  if (currentReading <= CV_current)
  {
    lcd.print("MODE:CV");
  }
  if (currentReading > CV_current)
  {
    lcd.print("MODE:CC");
  }
  lcd.setCursor(0, 1);
  lcd.print("CURRENT: ");
  lcd.print(currentReading);
  lcd.print(" A");
  if (currentReading <= cut_off)
  {
    for (i = 0; i < 10; i++)
    {
      currentReading = sensor.getCurrentDC();
      delay(100);
    }
    if (currentReading <= cut_off)
    {
      digitalWrite(RELAY_PIN, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BATTERY FULLY");
      lcd.setCursor(0, 1);
      lcd.print("CHARGED.");
      while (true) {}
    }
  }
  currentReading = sensor.getCurrentDC();
  if (currentReading >= peak_I_lt)
  {
    digitalWrite(RELAY_PIN, LOW);
    current_calib();
    digitalWrite(RELAY_PIN, HIGH);
    delay(3000);
    currentReading = sensor.getCurrentDC();
    if (currentReading >= peak_I_lt)
    {
      while (true)
      {
        digitalWrite(RELAY_PIN, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Overcharging");
        lcd.setCursor(0, 1);
        lcd.print("current detected");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Charging halted.");
        lcd.setCursor(0, 1);
        lcd.print("Press reset.");
        delay(2000);
      }
    }
  }
}

void current_calib()
{
  lcd.clear();
  lcd.print("Auto Calibrating");
  lcd.setCursor(0, 1);
  lcd.print("Current Sensor.");
  sensor.calibrate();
  delay(ONE_SECOND);
  currentReading = sensor.getCurrentDC();
  if (currentReading >= 0.02 || currentReading <= -0.02 )
  {
    sensor.calibrate();
    delay(5 * ONE_SECOND);
    currentReading = sensor.getCurrentDC();
    if (currentReading >= 0.02)
    {
      current_calib();
    }
  }
}

void timer()
{
  sec = sec + 1;
  if (sec == 60)
  {
    sec = 0;
    Min = Min + 1;
    re_calib();
  }
  if (Min == 60)
  {
    Min = 0;
    hrs = hrs + 1;
  }
  if (hrs == h_lt && Min == m_lt)
  {
    digitalWrite(RELAY_PIN, LOW);
    while (true)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time out !!!");
      lcd.setCursor(0, 1);
      lcd.print("Charge Completed");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Press reset");
      lcd.setCursor(0, 1);
      lcd.print("****************");
      delay(2000);
    }
  }
}

void re_calib()
{
  if (Min == 10 || Min == 20 || Min == 30 || Min == 40 ||
      Min == 50 || Min == 60 && sec == 0)
  {
    digitalWrite(RELAY_PIN, LOW);
    current_calib();
    digitalWrite(RELAY_PIN, HIGH);
  }
}

void CCCV()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Analyzing CC/CV");
  lcd.setCursor(0, 1);
  lcd.print("Modes...");
  digitalWrite(RELAY_PIN, HIGH);
  for (i = 0; i < 20; i++)
  {
    currentReading = sensor.getCurrentDC();
    delay(100);
  }
  if (currentReading <= -0.1)
  {
    while (true)
    {
      digitalWrite(RELAY_PIN, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Reverse current");
      lcd.setCursor(0, 1);
      lcd.print("detected.");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Flip current");
      lcd.setCursor(0, 1);
      lcd.print("sensor polarity.");
      delay(2000);
    }
  }
  CV_current = currentReading * 0.8;
}
//-------© Electronics-Project-hub-------//
