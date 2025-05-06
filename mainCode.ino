//Include the library
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Servo.h>


//Declare a RTC & LCD object:
const byte ROWS = 4;
const byte COLS = 4;
LiquidCrystal_I2C lcd(0x27, 16, 2); //12C address 0x27 (from DIYables LCD), 16 column and 1 row
RTC_DS1307 rtc;


//SERVO & KEYPAD:
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

  char getKey() {
    int analogValue = analogRead(keypadPin);

    if (analogValue <=  10) return '\0'; // No key pressed

    if (analogValue < 671) return '1';
    else if (analogValue < 661) return '2';
    else if (analogValue < 649) return '3';
    else if (analogValue < 641) return 'A';
    else if (analogValue < 628) return '4';
    else if (analogValue < 620) return '5';
    else if (analogValue < 609) return '6';
    else if (analogValue < 602) return 'B';
    else if (analogValue < 591) return '7';
    else if (analogValue < 582) return '8';
    else if (analogValue < 574) return '9';
    else if (analogValue < 568) return 'C';
    else if (analogValue < 557) return '*';
    else if (analogValue < 550) return '0';
    else if (analogValue < 543) return '#';
    else return 'D';
  }

//FOR BUZZER
void buzzerSound () {
  #define NOTE_E5  659
  #define NOTE_D5  587
  #define NOTE_FS4 370
  #define NOTE_GS4 415
  #define NOTE_CS5 554
  #define NOTE_B4  494
  #define NOTE_D4  294
  #define NOTE_E4  330
  #define NOTE_A4  440
  #define NOTE_CS4 277
  #define REST      0

  // FOR BUZZER: notes of the moledy followed by the duration.
  // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
  // !!negative numbers are used to represent dotted notes,
  // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
  int melody[] = {
 
   // Nokia Ringtone 
   // Score available at https://musescore.com/user/29944637/scores/5266155
   
   NOTE_E5, 8, NOTE_D5, 8, NOTE_FS4, 4, NOTE_GS4, 4, 
   NOTE_CS5, 8, NOTE_B4, 8, NOTE_D4, 4, NOTE_E4, 4, 
   NOTE_B4, 8, NOTE_A4, 8, NOTE_CS4, 4, NOTE_E4, 4,
   NOTE_A4, 2, 
 1};

  // FOR BUZZER: change this to make the song slower or faster
  int tempo = 100;
  // FOR BUZZER: change this to whichever pin you want to use
  int buzzer = 7;


  // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
  // there are two values per note (pitch and duration), so for each note there are four bytes
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;
  
  // this calculates the duration of a whole note in ms
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;
  
  
  
 //FOR BUZZER:
 // iterate over the notes of the melody.
 // Remember, the array is twice the number of notes (notes + durations)
 for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
  // calculates the duration of each note
  divider = melody[thisNote + 1];

  if (divider > 0) {
    // regular note, just proceed
    noteDuration = (wholenote) / divider;
    } 

  else if (divider < 0) {
    // dotted notes are represented with negative durations!!
    noteDuration = (wholenote) / abs(divider);
    noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a paus
    tone(buzzer, melody[thisNote], noteDuration * 0.5);
    // Wait for the specief duration before playing the next note.
    delay(noteDuration);
    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
}


//FOR RTC:
// UNCHANGABLE PARAMATERS
#define SUNDAY    0
#define MONDAY    1
#define TUESDAY   2
#define WEDNESDAY 3
#define THURSDAY  4
#define FRIDAY    5
#define SATURDAY  6

#define JANUARY   1
#define FEBRUARY  2
#define MARCH     3
#define APRIL     4
#define MAY       5
#define JUNE      6
#define JULY      7
#define AUGUST    8
#define SEPTEMBER 9
#define OCTOBER   10
#define NOVEMBER  11
#define DECEMBER  12


char daysOfTheWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

//EVENT FROM 
DateTime EVENT_START(2025, APRIL, 30, 17, 10);
DateTime EVENT_END(2025, APRIL, 30, 17, 12);

void setup() {
  lcd.begin(16,2);
  lcd.init(); //Initialize the lcd
  lcd.backlight(); //Open the backlight

  lockServo.attach(9); //Attaches the servo on pin 9 to the servo object
  lockServo.write(0); //rotate slowly servo to 0 degrees immediately

  // put your setup code here, to run once:
  //SETUP RTC MODULE
  if (!rtc.begin()) {
    lcd.print("Couldn't find RTC");
    Serial.println("Couldn't find RTC");
    while (1);
  }
  else {
    lcd.print("Found rtc");
    Serial.println("Found rtc");
  }
  //automatically sets the RTC to the date & time on PC this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //Manually sets the RTC with an explicit data & time, for example to set
  // January 21, 2021 at 3a, you would call:
  //rtc.adjust(DateTime(2021, 1, 21, 3, 0, 0));
  Serial.begin(9600);
}


void loop() {
  char key = getKey();
  int analogValue = analogRead(keypadPin);
  lockServo.write(90);
  if (analogValue <= 10) {
    // put your main code here, to run repeatedly:
    DateTime now = rtc.now();
      
    //Serial.println(analogRead(A0));

    int year = now.year();
    int month = now.month();
    int day = now.day();
    int hour = now.hour();
    int minute = now.minute();
    int second = now.second();

    //lockServo.write(100);
      
    if (now.secondstime() >= EVENT_START.secondstime() && now.secondstime() < EVENT_END.secondstime()) {
      lcd.print("Time to Refill");
      buzzerSound();

    }
    else {
      lcd.clear();
      lcd.setCursor(0,0); //Start to print at the first ro
      lcd.print("Date: ");
      lcd.print(year);
      lcd.print("/");
      lcd.print(month);
      lcd.print("/");
      lcd.print(day);
        
      lcd.setCursor(0,1); //Start to print at the second row
      lcd.print("Time: ");
      lcd.print(hour);
      lcd.print(":");
      lcd.print(minute);
      lcd.print(":");
      lcd.print(second);
        
      delay(1000); // delay 1 seconds TO update every second
      }

    
  }
  key = getKey();
    Serial.println("  Analog Value: ");
    Serial.println(analogRead(keypadPin));
    Serial.println("  key:");
    Serial.println(key);
  

  //KEYPAD & SERVO
  /*
  else {
    lcd.clear();
    lcd.print("Enter Password");
    Serial.println("Enter Password");
  }
*/

/*
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
  }

  // Handle normal password check
  if (!isChangingPassword && inputBuffer.length() >= passwordLength) {
    lcd.clear();
    if (inputBuffer == password) {
      lcd.print("Access Granted");
      lockServo.write(90); // Unlock
      delay(3000);
      lockServo.write(0);  // Lock again
    } 

    else {
      lcd.print("Access Denied");
      delay(2000);
    }

    inputBuffer = "";
    lcd.clear();
    lcd.print("Enter Password:");
  }
  */
  //KEYPAD
  // === Analog Keypad Mapping ===
  // You should calibrate these based on your resistor ladder
  
}

  




