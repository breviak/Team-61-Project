#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Servo.h>

// === LCD Setup ===
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27, 16x2 LCD

// === Servo Setup ===
Servo lockServo;
const int servoPin = 9;

// === Keypad Setup ===
const int keypadPin = A0;

// === Password Setup ===
const int passwordLength = 4;
String inputBuffer = "";
String password = "";
bool isChangingPassword = false;

// === Debounce Control ===
char lastKey = '\0';
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// === EEPROM Functions ===
String readPasswordFromEEPROM() {
  String pwd = "";
  for (int i = 0; i < passwordLength; i++) {
    char c = EEPROM.read(i);
    if (c == 255 || c < 32 || c > 126) return ""; // invalid char check
    pwd += c;
  }
  return pwd;
}

void savePasswordToEEPROM(const String& newPwd) {
  for (int i = 0; i < passwordLength; i++) {
    EEPROM.write(i, newPwd[i]);
  }
}

// === Setup ===
void setup() {
  Serial.begin(9600);
  
  lcd.begin(16, 2);        // Correct usage for your board
  lcd.backlight();         // Turn on backlight

  lockServo.attach(servoPin);
  lockServo.write(0);      // Locked position

  password = readPasswordFromEEPROM();
  if (password.length() != passwordLength) {
    password = "1234";     // Default password
    savePasswordToEEPROM(password);
  }

  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");
}

// === Main Loop ===
void loop() {
  char key = getKey();

  if (key != '\0' && key != lastKey && millis() - lastDebounceTime > debounceDelay) {
    lastDebounceTime = millis();
    lastKey = key;

    inputBuffer += key;
    lcd.setCursor(0, 1);
    lcd.print(inputBuffer);

    // Handle password change command
    if (inputBuffer.endsWith("*0#") && !isChangingPassword) {
      isChangingPassword = true;
      inputBuffer = "";
      lcd.clear();
      lcd.print("New Password:");
      return;
    }

    // Handle setting new password
    if (isChangingPassword && inputBuffer.length() >= passwordLength) {
      password = inputBuffer;
      savePasswordToEEPROM(password);
      isChangingPassword = false;
      inputBuffer = "";

      lcd.clear();
      lcd.print("Password Changed");
      delay(2000);
      lcd.clear();
      lcd.print("Enter Password:");
      return;
    }

    // Handle normal password check
    if (!isChangingPassword && inputBuffer.length() >= passwordLength) {
      lcd.clear();
      if (inputBuffer == password) {
        lcd.print("Access Granted");
        lockServo.write(90); // Unlock
        delay(3000);
        lockServo.write(0);  // Lock again
      } else {
        lcd.print("Access Denied");
        delay(2000);
      }

      inputBuffer = "";
      lcd.clear();
      lcd.print("Enter Password:");
    }
  }
}

// === Analog Keypad Mapping ===
// You should calibrate these based on your resistor ladder
char getKey() {
  int analogValue = analogRead(keypadPin);

  if (analogValue > 1020) return '\0'; // No key pressed

  if (analogValue < 50) return '1';
  else if (analogValue < 125) return '2';
  else if (analogValue < 200) return '3';
  else if (analogValue < 275) return 'A';
  else if (analogValue < 350) return '4';
  else if (analogValue < 425) return '5';
  else if (analogValue < 500) return '6';
  else if (analogValue < 575) return 'B';
  else if (analogValue < 650) return '7';
  else if (analogValue < 725) return '8';
  else if (analogValue < 800) return '9';
  else if (analogValue < 875) return 'C';
  else if (analogValue < 930) return '*';
  else if (analogValue < 980) return '0';
  else if (analogValue < 1015) return '#';
  else return 'D';
}

