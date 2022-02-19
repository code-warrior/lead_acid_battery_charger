//-------© Electronics-Project-hub-------//
#include "constants.h"
#include "ACS712.h"            // https://github.com/rkoptev/ACS712-arduino
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h> // https://gitlab.com/growduino/libs/LiquidCrystal_I2C

LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 × 2 display (16 chars, 2 lines)
ACS712 AnalogCurrentSensor(ACS712_05B, ACS712_CURRENT_SENSOR);

//------ Time out Setting --------//
int h_lt = 7; // in hrs
int m_lt = 0; // in min
// -------------------------------//

int address = 0;
int battery_capacity;
int current_limit = 0;
float peak_I_lt = 0;
float cut_off = 0;
boolean set_batt = true;
boolean var = true;
int hours_spent_charging = 0;
int minutes_spent_charging = 0;
int seconds_spent_charging = 0;
float currentReading;
float CV_current = 0;

void setup() {
   pinMode(RELAY_PIN, OUTPUT);
   digitalWrite(RELAY_PIN, LOW);
   pinMode(INCREMENT_CAPACITY_BUTTON, INPUT_PULLUP);
   pinMode(START_CHARGING_BUTTON, INPUT_PULLUP);
   lcd.init();
   lcd.backlight();
   EEPROM.get(address, battery_capacity);

   if (battery_capacity < 4500) {
      EEPROM.put(address, 4500);
   }

   lcd.clear();

   while (set_batt) {
      lcd.setCursor(0, 0);
      lcd.print("Enter capacity:");
      lcd.setCursor(0, 1);
      EEPROM.get(address, battery_capacity);
      lcd.print(battery_capacity);
      lcd.print(" mAh");

      if (digitalRead(INCREMENT_CAPACITY_BUTTON) == LOW) {
         while (var) {
            if (digitalRead(START_CHARGING_BUTTON) == LOW)
               var = false;

            if (digitalRead(INCREMENT_CAPACITY_BUTTON) == LOW) {
               lcd.setCursor(0, 1);
               battery_capacity += 500;

               if (battery_capacity > 15000) {
                  battery_capacity = 4500;

                  lcd.clear();
               }

               lcd.setCursor(0, 0);
               lcd.print("Enter capacity:");
               lcd.setCursor(0, 1);
               lcd.print(battery_capacity);
               lcd.print(" mAh");

               delay(250);
            }
         }
      }

      if (digitalRead(START_CHARGING_BUTTON) == LOW) {
         EEPROM.put(address, battery_capacity);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Your battery");
         lcd.setCursor(0, 1);
         lcd.print("is ");
         lcd.print(battery_capacity);
         lcd.print(" mAh.");

         delay(2 * ONE_SECOND);

         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Set current");
         lcd.setCursor(0, 1);
         lcd.print("limit = ");

         //------- Charging Parameters ----------//
         current_limit = battery_capacity * 0.2;
         peak_I_lt = battery_capacity * 0.3 * 0.001;
         cut_off = battery_capacity * 0.04 * 0.001;
         //-------------------------------------//

         lcd.print(current_limit);
         lcd.print(" mA");

         delay(3 * ONE_SECOND);

         set_batt = false;
      }
   }

   current_calib();
   CCCV();
}

void loop() {
   static int i;

   for (i = 0; i < 10; i++) {
      currentReading = AnalogCurrentSensor.getCurrentDC();
      delay(.1 * ONE_SECOND);
   }

   timer();
   lcd.clear();
   lcd.setCursor(0, 0);

   if (currentReading <= CV_current) {
      lcd.print("MODE:CV");
   }

   if (currentReading > CV_current) {
      lcd.print("MODE:CC");
   }

   lcd.setCursor(0, 1);
   lcd.print("CURRENT: ");
   lcd.print(currentReading);
   lcd.print(" A");

   if (currentReading <= cut_off) {
      for (i = 0; i < 10; i++) {
         currentReading = AnalogCurrentSensor.getCurrentDC();
         delay(.1 * ONE_SECOND);
      }

      if (currentReading <= cut_off) {
         digitalWrite(RELAY_PIN, LOW);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("BATTERY FULLY");
         lcd.setCursor(0, 1);
         lcd.print("CHARGED.");

         /* ??? */
         while (true) {
         }
      }
   }

   currentReading = AnalogCurrentSensor.getCurrentDC();

   if (currentReading >= peak_I_lt) {
      digitalWrite(RELAY_PIN, LOW);
      current_calib();
      digitalWrite(RELAY_PIN, HIGH);
      delay(3 * ONE_SECOND);

      currentReading = AnalogCurrentSensor.getCurrentDC();

      if (currentReading >= peak_I_lt) {
         while (true) {
            digitalWrite(RELAY_PIN, LOW);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Overcharging");
            lcd.setCursor(0, 1);
            lcd.print("current detected");
            delay(2 * ONE_SECOND);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Charging halted.");
            lcd.setCursor(0, 1);
            lcd.print("Press reset.");
            delay(2 * ONE_SECOND);
         }
      }
   }
}

void current_calib() {
   lcd.clear();
   lcd.print("Auto Calibrating");
   lcd.setCursor(0, 1);
   lcd.print("Current Sensor.");
   AnalogCurrentSensor.calibrate();
   delay(ONE_SECOND);
   currentReading = AnalogCurrentSensor.getCurrentDC();

   if (currentReading >= 0.02 || currentReading <= -0.02 ) {
      AnalogCurrentSensor.calibrate();
      delay(5 * ONE_SECOND);
      currentReading = AnalogCurrentSensor.getCurrentDC();

      if (currentReading >= 0.02) {
         current_calib();
      }
   }
}

void timer() {
   seconds_spent_charging += 1;

   if (seconds_spent_charging == 60) {
      seconds_spent_charging = 0;
      minutes_spent_charging += 1;
      re_calib();
   }

   if (minutes_spent_charging == 60) {
      minutes_spent_charging = 0;
      hours_spent_charging += 1;
   }

   if (hours_spent_charging == h_lt && minutes_spent_charging == m_lt) {
      digitalWrite(RELAY_PIN, LOW);

      while (true) {
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Time out !!!");
         lcd.setCursor(0, 1);
         lcd.print("Charge Completed");
         delay(2 * ONE_SECOND);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("  Press reset");
         lcd.setCursor(0, 1);
         lcd.print("****************");
         delay(2 * ONE_SECOND);
      }
   }
}

void re_calib() {
   if (minutes_spent_charging == 10 || minutes_spent_charging == 20 || minutes_spent_charging == 30 || minutes_spent_charging == 40 ||
       minutes_spent_charging == 50 || minutes_spent_charging == 60 && seconds_spent_charging == 0) {
      digitalWrite(RELAY_PIN, LOW);
      current_calib();
      digitalWrite(RELAY_PIN, HIGH);
   }
}

void CCCV() {
   static int i;

   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("Analyzing CC/CV");
   lcd.setCursor(0, 1);
   lcd.print("Modes...");
   digitalWrite(RELAY_PIN, HIGH);

   for (i = 0; i < 20; i++) {
      currentReading = AnalogCurrentSensor.getCurrentDC();
      delay(.1 * ONE_SECOND);
   }

   if (currentReading <= -0.1) {
      while (true) {
         digitalWrite(RELAY_PIN, LOW);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Reverse current");
         lcd.setCursor(0, 1);
         lcd.print("detected.");
         delay(2 * ONE_SECOND);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Flip current");
         lcd.setCursor(0, 1);
         lcd.print("sensor polarity.");
         delay(2 * ONE_SECOND);
      }
   }

   CV_current = currentReading * 0.8;
}
//-------© Electronics-Project-hub-------//
