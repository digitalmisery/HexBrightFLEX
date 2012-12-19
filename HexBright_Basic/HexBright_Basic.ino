/* 
  HexBright Basic Code - High/Med/Low/Dazzle
  
  Based on:
  Factory firmware for HexBright FLEX 
  v2.4  Dec 6, 2012
  https://github.com/hexbright/samples
  
  17 December 2012
  - Added function/pin reference to pin #defines for easier
  cross-reference to schematic.
  - Renamed sketch and added to same-named folder for Arduino
  IDE compatibility
  - Leveraged accelerometer inactivity auto-off code from 
  https://github.com/bhimoff/samples/
  - Added 1.0 second button hold power-off based on code from 
  https://github.com/jaebird/samples
  
  18 December 2012
  - Changed the way dazzle works and set flicker rate to match
  the known frequencies for vertigo (about 5 to 20Hz):
  http://en.wikipedia.org/wiki/Flicker_vertigo
*/

#include <math.h>
#include <Wire.h>

// Settings
#define OVERTEMP                340 //~1.1V = 60°C = 140°F

// Accelerometer defines
#define ACC_ADDRESS             0x4C
#define ACC_REG_XOUT            0
#define ACC_REG_YOUT            1
#define ACC_REG_ZOUT            2
#define ACC_REG_TILT            3
#define ACC_REG_INTS            6
#define ACC_REG_MODE            7

// Pin assignments
#define DPIN_RLED_SW            2 //PD2, INT0, MLF PIN 28
#define DPIN_ACC_INT            3 //PD3, INT1, MLF PIN 1
#define DPIN_GLED               5 //PD5, OC0B, MLF PIN 7
#define DPIN_PGOOD              7 //PD7, MLF PIN 9
#define DPIN_PWR                8 //PB0, MLF PIN 10
#define DPIN_DRV_MODE           9 //PB1, OC1A, MLF PIN 11
#define DPIN_DRV_EN             10 //PB2, OC1B, MLF PIN 12
#define APIN_TEMP               0 //PC0, ADC0, MLF PIN 19
#define APIN_CHARGE             3 //PC3, ADC3, MLF PIN 22

// Interrupts
#define INT_SW                  0
#define INT_ACC                 1

// Modes
#define MODE_OFF                0
#define MODE_LOW                1
#define MODE_MED                2
#define MODE_HIGH               3
#define MODE_DAZZLE             4
#define MODE_DAZZLE_PREVIEW     5

// State
byte mode = 0;
unsigned long btnTime = 0;
boolean btnDown = false;
boolean dazzle_on = true;
long dazzle_period = 100;

void setup()
{
  // We just powered on!  That means either we got plugged 
  // into USB, or the user is pressing the power button.
  pinMode(DPIN_PWR,      INPUT);
  digitalWrite(DPIN_PWR, LOW);

  // Initialize GPIO
  pinMode(DPIN_RLED_SW,  INPUT);
  pinMode(DPIN_GLED,     OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN,   OUTPUT);
  pinMode(DPIN_ACC_INT,  INPUT);
  pinMode(DPIN_PGOOD,    INPUT);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN,   LOW);
  digitalWrite(DPIN_ACC_INT,  HIGH);
  
  // Initialize serial busses
  Serial.begin(9600);
  Wire.begin();
  
  // Configure accelerometer
  byte config[] = {
    ACC_REG_INTS,  // First register (see next line)
    0xE4,  // Interrupts: shakes, taps
    0x00,  // Mode: not enabled yet
    0x00,  // Sample rate: 120 Hz
    0x0F,  // Tap threshold
    0x10   // Tap debounce samples
  };
  Wire.beginTransmission(ACC_ADDRESS);
  Wire.write(config, sizeof(config));
  Wire.endTransmission();

  // Enable accelerometer
  byte enable[] = {ACC_REG_MODE, 0x01};  // Mode: active!
  Wire.beginTransmission(ACC_ADDRESS);
  Wire.write(enable, sizeof(enable));
  Wire.endTransmission();
  
  btnTime = millis();
  btnDown = digitalRead(DPIN_RLED_SW);
  mode = MODE_OFF;

  Serial.println("Powered up!");
  randomSeed(analogRead(1));
}

void loop()
{
  static unsigned long lastDazzleTime, lastTempTime, lastModeTime, lastAccTime;
  static int ledState = LOW;
  
  unsigned long time = millis();
  
  // Check the state of the charge controller
  int chargeState = analogRead(APIN_CHARGE);
  if (chargeState < 128)  // Low - charging
  {
    digitalWrite(DPIN_GLED, (time&0x0100)?LOW:HIGH);
  }
  else if (chargeState > 768) // High - charged
  {
    digitalWrite(DPIN_GLED, HIGH);
  }
  else // Hi-Z - shutdown
  {
    digitalWrite(DPIN_GLED, LOW);    
  }
  
  // Check the temperature sensor
  if (time-lastTempTime > 1000)
  {
    lastTempTime = time;
    int temperature = analogRead(APIN_TEMP);
    Serial.print("Temp: ");
    Serial.println(temperature);
    if (temperature > OVERTEMP && mode != MODE_OFF)
    {
      Serial.println("Overheating!");

      for (int i = 0; i < 6; i++)
      {
        digitalWrite(DPIN_DRV_MODE, LOW);
        delay(100);
        digitalWrite(DPIN_DRV_MODE, HIGH);
        delay(100);
      }
      digitalWrite(DPIN_DRV_MODE, LOW);

      mode = MODE_LOW;
    }
  }

  // Check if the accelerometer wants to interrupt
  byte tapped = 0, shaked = 0;
  if (!digitalRead(DPIN_ACC_INT))
  {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_TILT);
    Wire.endTransmission(false);       // End, but do not stop!
    Wire.requestFrom(ACC_ADDRESS, 1);  // This one stops.
    byte tilt = Wire.read();
    
    if (time-lastAccTime > 500)
    {
      lastAccTime = time;
  
      tapped = !!(tilt & 0x20);
      shaked = !!(tilt & 0x80);
  
      if (tapped) Serial.println("Tap!");
      if (shaked) Serial.println("Shake!");
    }
  }
  
  // Do whatever this mode does
  switch (mode)
  {
  case MODE_DAZZLE:
  case MODE_DAZZLE_PREVIEW:
    if (time - lastDazzleTime > dazzle_period)
    {
      digitalWrite(DPIN_DRV_EN, dazzle_on);
      dazzle_on = !dazzle_on;
      lastDazzleTime = time;
      dazzle_period = random(25,100);
    }    
    break;
  }
  
  // Periodically pull down the button's pin, since
  // in certain hardware revisions it can float.
  pinMode(DPIN_RLED_SW, OUTPUT);
  pinMode(DPIN_RLED_SW, INPUT);
  
  // Check for mode changes
  byte newMode = mode;
  byte newBtnDown = digitalRead(DPIN_RLED_SW);
  switch (mode)
  {
  case MODE_OFF:
    if (btnDown && !newBtnDown && (time-btnTime)>20)
      newMode = MODE_LOW;
    if (btnDown && newBtnDown && (time-btnTime)>500)
      newMode = MODE_DAZZLE_PREVIEW;
    break;
  case MODE_LOW:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      if (time-lastModeTime > 1000)
        newMode = MODE_OFF;
      else newMode = MODE_MED;
    break;
  case MODE_MED:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      if (time-lastModeTime > 1000)
        newMode = MODE_OFF;
      else newMode = MODE_HIGH;
    break;
  case MODE_HIGH:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_OFF;
    break;
  case MODE_DAZZLE_PREVIEW:
    // This mode exists just to ignore this button release.
    if (btnDown && !newBtnDown)
      newMode = MODE_DAZZLE;
    break;
  case MODE_DAZZLE:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_OFF;
    break;
  }
  
  //activity power down
  if (time-max(lastAccTime,lastModeTime) > 600000UL) { //10 minutes
    newMode = MODE_OFF;
  }

  // Do the mode transitions
  if (newMode != mode)
  {
    lastModeTime = millis();
    switch (newMode)
    {
    case MODE_OFF:
      Serial.println("Mode = off");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, LOW);
      digitalWrite(DPIN_DRV_MODE, LOW);
      digitalWrite(DPIN_DRV_EN, LOW);
      break;
    case MODE_LOW:
      Serial.println("Mode = low");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 32);
      break;
    case MODE_MED:
      Serial.println("Mode = medium");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 255);
      break;
    case MODE_HIGH:
      Serial.println("Mode = high");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      analogWrite(DPIN_DRV_EN, 255);
      break;
    case MODE_DAZZLE:
    case MODE_DAZZLE_PREVIEW:
      Serial.println("Mode = dazzle");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      break;
    }

    mode = newMode;
  }

  // Remember button state so we can detect transitions
  if (newBtnDown != btnDown)
  {
    btnTime = time;
    btnDown = newBtnDown;
    delay(50);
  }
}

void readAccel(char *acc)
{
  while (1)
  {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_XOUT);
    Wire.endTransmission(false);       // End, but do not stop!
    Wire.requestFrom(ACC_ADDRESS, 3);  // This one stops.

    for (int i = 0; i < 3; i++)
    {
      if (!Wire.available())
        continue;
      acc[i] = Wire.read();
      if (acc[i] & 0x40)  // Indicates failed read; redo!
        continue;
      if (acc[i] & 0x20)  // Sign-extend
        acc[i] |= 0xC0;
    }
    break;
  }
}

float readAccelAngleXZ()
{
  char acc[3];
  readAccel(acc);
  return atan2(acc[0], acc[2]);
}

