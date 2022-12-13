#include <LiquidCrystal.h>
#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
#define DOUT 8
#define CLK 9

//defining pins and ESP AVR EEPROM and all necessary libraries to function

const int buttonPin = 10;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
HX711_ADC LoadCell(DOUT, CLK);

//VARIABLES


unsigned long t = 0;
const int calVal_eepromAdress = 0;
float newCalibrationValue = 0;

//***********************************




void setup() {
  // boolean value checked for condition for all statements
  // coming to configuration
  bool _check = false;

  // Setting up I/O pins and their respected positions
  // Commented specifically which ports represent which pins.

  // D7 - D0 = PD7 - PD0
  PORTD = B00111100;
        //76543210
  DDRD = B00111100;
        //76543210

  // D13 - D8 = PD5 -> PD0
  PORTB = B011111;
         //543210
  DDRB = B011111;
         //543210

  // A7 - A0 = PC7-> PC0
  PORTC = B00000000;
        //76543210

  DDRC = B00000000;
       //76543210


  Serial.begin(9600);
  Serial.println("Starting...");
  LoadCell.begin();
  lcd.begin(20, 4);
  unsigned long stabilizingTime = 2000;
  boolean _tare = true;
  // can set to false if no need for tare
  //Taring the scale for 0 longer duration of time improves accuracy
  if (LoadCell.getTareTimeoutFlag()) {
    lcd.setCursor(0, 0);
    lcd.print("Timeout wirePIN");
  } else {
    //1.0 used for initial value for calibration
    lcd.print("Setup Complete");
    delay(500);
    lcd.clear();
  }

  // printing information on hx711
  // a tool to debug incase something doesn't work accordingly.
  //everything displayed serial side so end user doesn't specifically
  //has to look at it.

  lcd.print("Loading...");
  while (!LoadCell.update());
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settlingtime ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Do you want to configure the scale y/n");
  Serial.print("EEPROM calibration value is: ");
  Serial.println(EEPROM.get(0, newCalibrationValue));
  LoadCell.start(stabilizingTime, _tare);

  // after the load time lcd is cleared and we showcase
  // current calibration value to the user and the option to skip
  // calibration as its not necessary to calibrate it everytime its used
  // tested multiple instances of functionality and eeprom provides
  // correct calibrated value necessary.

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration:");
  lcd.setCursor(12, 0);
  lcd.print(newCalibrationValue);
  lcd.setCursor(0, 1);
  lcd.print("Y/N for Calibration");
  lcd.setCursor(0, 2);
  lcd.print("Click button to");
  lcd.setCursor(0, 3);
  lcd.print("Skip calibration");
  pinMode(buttonPin, INPUT_PULLUP);

  // reading btn value constantly to figure out when it's clicked once clicked the
  // program continues as if it's already configured for the user's personal use.
  // specifically capabilities are set aswell for console side use with serial print
  // serial print option available and used solely during configuration.
  // data is fetched with configured value from EEPROM.get
  // EEPROM address is always the same so it's not necessary to configure it again.

  while (_check == false) {
    uint8_t btn = digitalRead(buttonPin);
    if (Serial.available() > 0 || btn == HIGH) {
      char inByte = Serial.read();
      if (inByte == 'n' || btn == HIGH) {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        Serial.println(newCalibrationValue);
        lcd.clear();
        btn == LOW;
        _check = true;
      } else if (inByte == 'y') {
        calibrate();
        _check = true;
      }
    }
  }
}
// Function clears out unnecessary 2 rows of options that user can use
// no clear solution to be exact, except for sending the function
// ounces and grams seperately to showcase them that way
// but this is a simpler solution.
void pause() {
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  for (int i = 0; i < 5; i++) {
    lcd.noDisplay();
    delay(250);
    lcd.display();
    delay(750);
  }
  // not seperately taring the scale but also possible
  // with LoadCell.tareNoDelay(); 
  // reason as to not break the offset possibly caused by
  // user leaving the weight for too long.
}

void loop() {
  // Reading constantly the state of buttonPin to enable
  // usage of pause function as attachInterrupt is incompatible
  // as the specific pins are used for LCD display on NANO.
  uint8_t btn = digitalRead(buttonPin);
  if (btn == HIGH) {
    pause();
  }

  // Calculating the value with HX711.adc library function and setting specific
  // variables to display our collected data
  // i representing the default grams as it was calibrated for grams
  // and ounces representing ounces
  // user displayed with an option to pause with an installed button
  // previously mentioned button state that we read during the loop.

  static boolean newDataReady = 0;
  const int serialPrintInterval = 200;
  if (LoadCell.update()) newDataReady = true;
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      lcd.setCursor(0, 0);
      lcd.print("Grams:");
      lcd.setCursor(6, 0);
      lcd.print(i);
      lcd.setCursor(0, 1);
      lcd.print("Ounces:");
      lcd.setCursor(7, 1);
      float ounces = i * 0.0352739619;
      lcd.print(ounces);
      lcd.setCursor(0, 2);
      lcd.print("Press button");
      lcd.setCursor(0, 3);
      lcd.print("To pause the scale");
      t = millis();
    }
  }

  // function is for more advanced use where you access with serial monitor
  // no specific usage for normal usage outside of calibration
  // reason as to why it's not displayed or told specficially to the end user.
  // giving option to calibrate tare and change the calibrated factor for sharper adjustment.

  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();       // tare
    else if (inByte == 'r') calibrate();             // calibrate
    else if (inByte == 'c') changeSavedCalFactor();  //edit calibration value manually
  }

  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}

void calibrate() {
  lcd.clear();
  // calibration happens serial monitor side and the serial
  // messages should guide user for calibration with all the
  // necessary details needed for proper functionality.

  Serial.println("***");
  Serial.println("Start the calibration");
  Serial.println("Place the load cell on a level stable surface");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte = 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare Complete");
      _resume = true;
    }
  }

  Serial.println("Now place a known mass on the loadcell");
  Serial.println("Now send the weight of this mass (i.e 100.0) from serial monitor ");

  float knownMass = 0;
  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      knownMass = Serial.parseFloat();
      if (knownMass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(knownMass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet();
  Serial.println(knownMass);
  Serial.println(LoadCell.getNewCalibration(knownMass));
  float newCalibrationValue = LoadCell.getNewCalibration(knownMass);

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch");
  Serial.print("Save this value to EEROM address ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;

  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value: ");
        Serial.print(newCalibrationValue);
        Serial.print(" Saved to EEPROM address: ");
        Serial.print(calVal_eepromAdress);
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
}

void changeSavedCalFactor() {
  // similarly serial messages should give proper
  // guidance for users self adjusted values.
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}
