// This #include statement was automatically added by the Particle IDE.
#include <blynk.h>

// This #include statement was automatically added by the Particle IDE.
#include <HC-SR04.h>

char auth[] = "bcc4915453ad4022a2a8bb61f38e0b97";

// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.
const int numReadings = 10;

int waterReadings[numReadings];      // the readings from the analog input
int waterReadIndex = 0;              // the index of the current reading
int waterTotal = 0;                  // the running total
int waterAverage = 0;                // the average

int flameReadings[numReadings];      // the readings from the analog input
int flameReadIndex = 0;              // the index of the current reading
int flameTotal = 0;                  // the running total
int flameAverage = 0;                // the average

int lightReadings[numReadings];      // the readings from the analog input
int lightReadIndex = 0;              // the index of the current reading
int lightTotal = 0;                  // the running total
int lightAverage = 0;                // the average

#define WATERPIN A0 // Pin for watersensor
#define FLAMEPIN A3 // Pin for flamesensor
#define LIGHTPIN A4 // Pin for lightsensor
#define TRIGGER A2 // Trigger pin for distance sensor
#define ECHO A1 // Echo pin for distance sensor

#define TURNALARMONOFF D3 // Pin for button
#define REDPIN D2 // Pin for RGB (RED)
#define GREENPIN D1 // Pin for RGB (GREEN)
#define BLUEPIN D0 // Pin for RGB (BLUE)

// variables for delay on start
bool isSystem = false;
bool isBooting = false;
unsigned long startBootTime = 0;

// variables for alarm state
int alarmSystemState = 0;
int lastAlarmSystemState = 0;

// distance sensor
HC_SR04 distanceSensor = HC_SR04(TRIGGER, ECHO);

unsigned long lastWaterMillis = 0;
unsigned long lastWaterMessageMillis = 0;

unsigned long lastDistanceMessageMillis = 0;

unsigned long lastFlameMillis = 0;
unsigned long lastFlameMessageMillis = 0;

unsigned long lastLightMillis = 0;
unsigned long lastLightMessageMillis = 0;

BlynkTimer aTimer; // Announcing the timer

// setup all stuffs for alarm
void setup() {

  Blynk.begin(auth);

  initPinModes();

  initAllWaterReadingsToZero();
  initAllFlameReadingsToZero();
  initAllLightReadingsToZero();

  distanceSensor.init();

  digitalWrite(REDPIN, 255);

  aTimer.setInterval(1000L, myTimerEvent);
}

// forever loop for the system
void loop() {

  Blynk.run(); // Run Blynk
  aTimer.run(); // running timer every second

  alarmSystemState = digitalRead(TURNALARMONOFF);

  if (isSystem) {
    digitalWrite(REDPIN, 0);
    digitalWrite(GREENPIN, 255);
    storageAlarm();
  }

  // When button is pressed, start alarm in 15 seconds
  startDelay();

  // Button for handling the StoreAlarm On/Off
  if (alarmSystemState == HIGH && lastAlarmSystemState == LOW) {

    // Turn On/Off based on current state
    if (isSystem != true) {
      isBooting = true;
      startBootTime = millis();

    } else {
      // Set alarm OFF
      Blynk.notify("Alarm is OFF");
      sendSlackAlarm("systemSlack", "Alarm is OFF");
      isSystem = false;
      digitalWrite(GREENPIN, 0);
      digitalWrite(REDPIN, 255);
    }

  }

  lastAlarmSystemState = alarmSystemState;
}


// Init pin modes
void initPinModes() {
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(TURNALARMONOFF, INPUT);
}

// Blink event timer
void myTimerEvent()
{
  int sensorData = analogRead(WATERPIN);
  Blynk.virtualWrite(V1, sensorData);
}


// Start alarm in 30 seconds when turn On
void startDelay() {

  if (!isBooting) {
    return;
  }
  if (millis() - startBootTime > 30000) {
    Blynk.notify("Alarm is ON");
    sendSlackAlarm("systemSlack", "Alarm is ON");
    isSystem = true;
    digitalWrite(REDPIN, 0);
    digitalWrite(GREENPIN, 255);
    isBooting = false;
  }

}


// initialize all the readings for water array to 0:
void initAllWaterReadingsToZero() {
  for (int i = 0; i < numReadings; i++) {
    waterReadings[i] = 0;
  }
}


// initialize all the readings for flame array to 0:
void initAllFlameReadingsToZero() {
  for (int i = 0; i < numReadings; i++) {
    flameReadings[i] = 0;
  }
}


// initialize all the readings for light array to 0:
void initAllLightReadingsToZero() {
  for (int i = 0; i < numReadings; i++) {
    lightReadings[i] = 0;
  }
}


// method for sending message on slack
void sendSlackAlarm(String eventName, String message) {
  Particle.publish(eventName, message);
}


// run the sensors
void storageAlarm() {
  runWaterSensor();
  runFlameSensor();
  runLightSensor();
  runDistanceSensor();
}


// distance sensor
void runDistanceSensor() {
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);

  long duration = pulseIn(ECHO, HIGH);
  int distance = duration * 0.034 / 2;

  // trigger alarm if distance is lower or equal 150cm
  if (distance <= 150 && millis() - lastDistanceMessageMillis > 15000) { // send message every 15 seconds!
    Blynk.notify("Distance alarm!");
    sendSlackAlarm("distanceSlack", "Someone is inside your private storage! Better check it out!");
    lastDistanceMessageMillis = millis();
  }
}


// water sensor
void runWaterSensor() {

  // get values every 500milliseconds
  if (millis() - lastWaterMillis < 500) {
    return;
  }

  lastWaterMillis = millis();

  // subtract the last reading:
  waterTotal = waterTotal - waterReadings[waterReadIndex];
  // read from the sensor:
  waterReadings[waterReadIndex] = analogRead(WATERPIN);
  // add the reading to the total:
  waterTotal = waterTotal + waterReadings[waterReadIndex];
  // advance to the next position in the array:
  waterReadIndex = waterReadIndex + 1;

  // if we're at the end of the array...
  if (waterReadIndex >= numReadings) {
    // ...wrap around to the beginning:
    waterReadIndex = 0;
  }

  // calculate the average:
  waterAverage = waterTotal / numReadings;

  // trigger alarm if water sensor value is greater then 50, which indicates that is detected water
  if (waterAverage > 50 && millis() - lastWaterMessageMillis > 15000) { // send message every 15 seconds!
    Blynk.notify("Water alarm!");
    sendSlackAlarm("waterSlack", "There may be some water leak in your storage! Better check it out!");
    lastWaterMessageMillis = millis();
  }
}


// flame sensor
void runFlameSensor() {

  if (millis() - lastFlameMillis < 500) {
    return;
  }

  lastFlameMillis = millis();

  // subtract the last reading:
  flameTotal = flameTotal - flameReadings[flameReadIndex];
  // read from the sensor:
  flameReadings[flameReadIndex] = analogRead(FLAMEPIN);
  // add the reading to the total:
  flameTotal = flameTotal + flameReadings[flameReadIndex];
  // advance to the next position in the array:
  flameReadIndex = flameReadIndex + 1;

  // if we're at the end of the array...
  if (flameReadIndex >= numReadings) {
    // ...wrap around to the beginning:
    flameReadIndex = 0;
  }

  // calculate the average:
  flameAverage = flameTotal / numReadings;

  // trigger alarm if flame sensor value is greater then 15, which indicates it has detected flames
  if (flameAverage > 15 && millis() - lastFlameMessageMillis > 15000) { // send message every 15 seconds!
    sendSlackAlarm("flameSlack", "There may be some fire on your common storage! Better check it out!");
    Blynk.notify("Flame alarm!");
    lastFlameMessageMillis = millis();
  }
}


// light sensor
void runLightSensor() {

  if (millis() - lastLightMillis < 500) {
    return;
  }

  lastLightMillis = millis();

  // subtract the last reading:
  lightTotal = lightTotal - lightReadings[lightReadIndex];
  // read from the sensor:
  lightReadings[lightReadIndex] = analogRead(LIGHTPIN);
  // add the reading to the total:
  lightTotal = lightTotal + lightReadings[lightReadIndex];
  // advance to the next position in the array:
  lightReadIndex = lightReadIndex + 1;

  // if we're at the end of the array...
  if (lightReadIndex >= numReadings) {
    // ...wrap around to the beginning:
    lightReadIndex = 0;
  }

  // calculate the average:
  lightAverage = lightTotal / numReadings;

  // trigger alarm if light sensor value is greater then 10
  if (lightAverage > 80 && millis() - lastLightMessageMillis > 20000) { // send message every 20 seconds!
    Blynk.notify("Light alarm!");
    //sendSlackAlarm("lightSlack", "The light is turned on! Might be some of your neighbor visiting the storage!");
    lastLightMessageMillis = millis();
  }
}