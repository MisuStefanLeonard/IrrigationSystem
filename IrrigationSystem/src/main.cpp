#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

/*--------DEFINES-------*/

#define OK_LED 0
#define NOT_OK_LED 2
#define MOISTURE_SENSOR A0
#define PUMP_CONTROL_PIN 14
#define FORCE_STOP_BUTTON 12
#define SOL_UMED 100
#define SOL_USCAT 1024
#define MESSAGES_INTERVAL 5000
const int port = 5055;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org"); 
WiFiServer server(port);

/*--------DEFINES-------*/

/*WI-FI SSID AND PASSWORD*/
/*---------WI-FI ACASA ROSIORI-----------*/
const char *ssid = "Misu" /* your ssid here */;
const char *password = "FamiliaMisu21" /* your ssid  password here */;
/*----------------------------------------*/
/*WI-FI SSID AND PASSWORD*/


/*GLOBAL VARS*/

volatile bool modeChanged = true; // false for "AUTO", true for "MANUAL"
int moistureThreshold = 0;
volatile bool pompState = false;
long long askedProgramTime = -1;
long long startTheProgrammedPump = -1;
volatile bool isProgrammedByTime = false;
volatile bool cancelProgrammedTimed = false;
volatile bool forceStop = false;
volatile bool isOn = false;
unsigned long lastSendTime = 0;
volatile bool resetTime = false;
volatile bool isPumpOnAuto = false;
int dataId = 0;

/*GLOBAL VARS*/

// INTRERRUPT FUNCTION
void IRAM_ATTR PumpControlManualISR()
{
  pompState = false;
  forceStop = true;
  modeChanged = true; // defaults to AUTO;
}

void setup()
{
  Serial.begin(9600);
  pinMode(OK_LED, OUTPUT);
  pinMode(NOT_OK_LED, OUTPUT);
  pinMode(PUMP_CONTROL_PIN, OUTPUT);
  pinMode(FORCE_STOP_BUTTON, INPUT_PULLUP);
  digitalWrite(PUMP_CONTROL_PIN, LOW);
  digitalWrite(OK_LED, HIGH);
  digitalWrite(NOT_OK_LED, HIGH);
  Serial.println();
  Serial.print("Connecting to Wi-Fi -> ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Controller IP Address: ");
  Serial.print(WiFi.localIP());
  Serial.println();
  Serial.print("Connected to port: ");
  Serial.println();
  server.begin();
  Serial.print(port);
  Serial.println();

  lcd.init();
  lcd.backlight();
  timeClient.begin();
  timeClient.setTimeOffset(2*3600);
  attachInterrupt(digitalPinToInterrupt(FORCE_STOP_BUTTON), PumpControlManualISR, FALLING);
}

/*SET MODE FUNCTION*/
/*CHANGING MODE BETWEEN AUTO AND MANUAL TRACKING USING A BOOL*/
void SetMode(char selectMode)
{
  resetTime = true;
  dataId++;
  switch (selectMode)
  {
  case 'a':
    Serial.println("Switched to auto");
    modeChanged = true; // AUTO MODE
    isProgrammedByTime = false;
    askedProgramTime = -1;
    startTheProgrammedPump = -1;
    break;
  case 'm':
    Serial.println("Switched to manual");
    modeChanged = false; // MANUAL MODE
    break;

  default:
    break;
  }
}

void PrintOnLcd(const char *message)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

// HUMIDITY PERCENTAGE CALCULATOR FUNCTION
float CalculateHumidityPercent()
{
  int readAnalogHumidityValue = analogRead(MOISTURE_SENSOR);
  Serial.println(readAnalogHumidityValue);

  // Ensure values are clamped between 0% and 100%
  float percent = 100.0 * ((SOL_USCAT - readAnalogHumidityValue) / (float)(SOL_USCAT - SOL_UMED));
  percent = constrain(percent, 0, 100); // Clamp to range 0-100
  return percent;
}
long long ConvertTimeToSeconds(int hours, int minutes, int seconds)
{
  return (hours * 3600) + (minutes * 60) + seconds;
}
/*BUTTON FORCE STOP ON BREADBOARD FUNCTION*/
void ForceStop(WiFiClient *client)
{
  if (forceStop)
  {
    // Stop the pump
    digitalWrite(PUMP_CONTROL_PIN, LOW);
    pompState = false;

    // Reset states
    forceStop = false;
    isProgrammedByTime = false;
    askedProgramTime = -1;
    startTheProgrammedPump = -1;
    isOn = false;

    // Display status on LCD
    PrintOnLcd("Force Stop Activated");
    lcd.setCursor(1, 0);
    lcd.print("Pump: OFF");

    Serial.println("Pump force-stopped via button.");
    client->print("FORCESTOP");
  }
}
/*BUTTON FORCE STOP ON BREADBOARD FUNCTION*/

/*MAIN FUNCTIONALITY OF THE PROGRAM*/
void Functionality(WiFiClient *client, String message)
{
  ForceStop(client);
  float humidity = CalculateHumidityPercent();
  unsigned long currentTimePassed = millis();
  String autoPumpMessage = "";
  switch (static_cast<int>(modeChanged))
  {
  // mode AUTO
  case 1:
    PrintOnLcd("POMPA:");
    lcd.print(pompState == true ? "ON" : "OFF");
    lcd.setCursor(12, 0);
    lcd.print("AUTO");
    lcd.setCursor(0, 1);
    lcd.print("HUMI:");
    lcd.print(humidity);
    lcd.print("%");

    // moisture to send to the frontend and mode
    if (resetTime)
    {
      String message = ""; // Create a single string to hold the entire message
      message += "START";
      message += String(dataId) + " ";
      message += "AUTO ";
      message += String(CalculateHumidityPercent()) + " ";
      message += String(pompState ? "ON " : "OFF ");
      message += String(moistureThreshold) + " ";
      message += "BLANK "; // PROGRAMMED INFO 
      if(isProgrammedByTime){
        message += "BLANK " ; 
      }
      message += "SETAT " ; // was set
      message += "END" + String(dataId) + "\n";
      resetTime = false;
      lastSendTime = currentTimePassed;
      client->print(message);
    }
    else
    {
      if (currentTimePassed - lastSendTime >= MESSAGES_INTERVAL)
      {
        lastSendTime = currentTimePassed;
        String message = ""; // Create a single string to hold the entire message
        message += "START";
        message += String(dataId) + " ";
        message += "AUTO ";
        message += String(CalculateHumidityPercent()) + " ";
        message += String(pompState ? "ON " : "OFF ");
        message += String(moistureThreshold) + " ";
        message += "BLANK "; // PROGRAMMED INFO 
        if(isProgrammedByTime){
          message += "BLANK " ; 
        }
        message += "SETAT " ; // was set
        message += "END" + String(dataId) + "\n";
        client->print(message);
      }
    }

    if (moistureThreshold > humidity)
    {
      pompState = true;
      isPumpOnAuto = true;
    }

    while (isPumpOnAuto)
    {
      // de afisat STAREA POMPEI SI UMIDITATEA CURENTA
      int readingPumpState = digitalRead(PUMP_CONTROL_PIN);
      if (readingPumpState == LOW)
      {
        digitalWrite(PUMP_CONTROL_PIN, HIGH);
        pompState = true;
      }
      humidity = CalculateHumidityPercent();
      Serial.println("Recalculata: ");
      Serial.print(humidity);
      if (humidity > moistureThreshold)
      {
        digitalWrite(PUMP_CONTROL_PIN, LOW);
        pompState = false;
        PrintOnLcd("POMPA:");
        lcd.print(pompState == true ? "ON" : "OFF");
        lcd.setCursor(12, 0);
        lcd.print("AUTO");
        lcd.setCursor(0, 1);
        lcd.print("HUMI:");
        lcd.print(humidity);
        lcd.print("%");
        isPumpOnAuto = false;
      }
      if (currentTimePassed - lastSendTime >= MESSAGES_INTERVAL){
        lastSendTime = currentTimePassed;
        String message = ""; 
        message += "START";
        message += String(dataId) + " ";
        message += "MANUAL ";
        message += String(humidity) + " ";
        message += String(pompState ? "ON " : "OFF ");
        message += String(moistureThreshold) + " ";
        message += "BLANK "; // PROGRAMMED INFO 
        if(isProgrammedByTime){
          message += "BLANK " ; 
        }
        message += "SETAT " ; // was set
        message += "END" + String(dataId) + "\n";
        client->print(message);
      }
      yield();
    }
    break;
  // mode MANUAL
  case 0:
    ForceStop(client);
    PrintOnLcd("POMPA:");
    lcd.print(pompState == true ? "ON" : "OFF");
    lcd.setCursor(12, 0);
    lcd.print("MAN");
    lcd.setCursor(0, 1);
    lcd.print("HUMI:");
    lcd.print(humidity);
    lcd.print("%");
    if (resetTime)
    {
      String message = ""; // Create a single string to hold the entire message
      message += "START";
      message += String(dataId) + " ";
      message += "MANUAL ";
      message += String(CalculateHumidityPercent()) + " ";
      message += String(pompState ? "ON " : "OFF ");
      message += String(moistureThreshold) + " ";
      message += "BLANK "; // PROGRAMMED INFO 
      if(isProgrammedByTime){
        message += "SETAT " ; 
      }
      message += "BLANK " ; // was set
      message += "END" + String(dataId) + "\n";
      resetTime = false;
      lastSendTime = currentTimePassed;
      client->print(message);
    }
    else
    {
      if (currentTimePassed - lastSendTime >= MESSAGES_INTERVAL)
      {
        lastSendTime = currentTimePassed;
        String message = ""; // Create a single string to hold the entire message
        message += "START";
        message += String(dataId) + " ";
        message += "MANUAL ";
        message += String(CalculateHumidityPercent()) + " ";
        message += String(pompState ? "ON " : "OFF ");
        message += String(moistureThreshold) + " ";
        message += "BLANK "; // PROGRAMMED INFO 
        if(isProgrammedByTime){
          message += "SETAT " ; 
        }
        message += "BLANK " ; // was set
        message += "END" + String(dataId) + "\n";

        client->print(message); // Send the entire message at once
      }
    }
    if (message == "START")
    {
      if (!pompState)
      {
        digitalWrite(PUMP_CONTROL_PIN, HIGH);
        pompState = true; // POMP ON
      }
     
    }
    else if (message == "STOP")
    {
      if (pompState)
      {
        digitalWrite(PUMP_CONTROL_PIN, LOW);
        pompState = false; // POMP OFF
      }
      // PROGRAM . 0123456
      // PROGRAM_12:30:50
    }
    else if (message.substring(0, 7) == "PROGRAM")
    {
      int underscoreIndex = message.indexOf('_');
      String time = message.substring(underscoreIndex + 1);

      // Extract HH, MM, and SS
      int firstColonIndex = time.indexOf(':');
      int secondColonIndex = time.indexOf(':', firstColonIndex + 1);

      int HH = time.substring(0, firstColonIndex).toInt();                    // Extract HH
      int MM = time.substring(firstColonIndex + 1, secondColonIndex).toInt(); // Extract MM
      int SS = time.substring(secondColonIndex + 1).toInt();                  // Extract SS

      askedProgramTime = ConvertTimeToSeconds(HH, MM, SS); // this will be eh 1:30:20 converted to seconds

      timeClient.update();
      String getFormatedTimeFromNTP = timeClient.getFormattedTime();
      int currentHourFromNTP = getFormatedTimeFromNTP.substring(0,2).toInt();
      int currentMinutesFromNTP = getFormatedTimeFromNTP.substring(3,5).toInt();
      int currentSecondsFromNTP = getFormatedTimeFromNTP.substring(6,8).toInt();
      // askedProgramTime is how much to add 
      long long currentTimeToSecondsFromNtp = ConvertTimeToSeconds(currentHourFromNTP, currentMinutesFromNTP, currentSecondsFromNTP);

      startTheProgrammedPump = currentTimeToSecondsFromNtp + askedProgramTime;

      int hours = startTheProgrammedPump / 3600;              // Calculate hours
      int remainingSeconds = startTheProgrammedPump % 3600;   // Calculate remaining seconds after hours
      int minutes = remainingSeconds / 60;                   // Calculate minutes
      int seconds = remainingSeconds % 60;                  // Calculate remaining seconds
      isProgrammedByTime = true;
      lcd.clear();
      lcd.setCursor(12, 0);
      lcd.print("MAN");
      lcd.setCursor(0, 0);
      lcd.print("TIMP SETAT");
      lcd.setCursor(1, 1);
      lcd.print(hours);
      lcd.print(":");
      lcd.print(minutes);
      lcd.print(":");
      lcd.print(seconds);
      String message = ""; // Create a single string to hold the entire message
      message += "START";
      message += String(dataId) + " ";
      message += "MANUAL ";
      message += String(CalculateHumidityPercent()) + " ";
      message += String(pompState ? "ON " : "OFF ");
      message += String(moistureThreshold) + " ";
      message += "BLANK "; // PROGRAMMED INFO 
      message += "SETAT "; // was set
      message += "END" + String(dataId) + "\n";
      client->print(message);
    }
    else if (isProgrammedByTime)
    {
      if (message == "CANCEL")
      {
        askedProgramTime = 1;
        isProgrammedByTime = false;
        isOn = false;
        String message = ""; // Create a single string to hold the entire message
        message += "START";
        message += String(dataId) + " ";
        message += "MANUAL ";
        message += String(CalculateHumidityPercent()) + " ";
        message += String(pompState ? "ON " : "OFF ");
        message += String(moistureThreshold) + " ";
        message += "PROGRAMARE_ANULATA ";
        message += "BLANK ";
        message += "END" + String(dataId) + "\n";
        client->print(message);
      }
      timeClient.update();
      // 13:21:20
      String getFormatedTimeFromNTP = timeClient.getFormattedTime();
      int currentHourFromNTP = getFormatedTimeFromNTP.substring(0,2).toInt();
      int currentMinutesFromNTP = getFormatedTimeFromNTP.substring(3,5).toInt();
      int currentSecondsFromNTP = getFormatedTimeFromNTP.substring(6,8).toInt();
      // askedProgramTime is how much to add 
      long long currentTimeToSecondsFromNtp = ConvertTimeToSeconds(currentHourFromNTP, currentMinutesFromNTP, currentSecondsFromNTP);
      if (currentTimeToSecondsFromNtp == startTheProgrammedPump)
      {
        long startTime = millis(); // pomp START time eg : 25
        long elapsedTime = 0;
        isOn = true;
        while (isOn)
        {
          ForceStop(client);
          elapsedTime = millis() - startTime;
          if (!pompState)
          {
            digitalWrite(PUMP_CONTROL_PIN, HIGH);
            lcd.clear();
            lcd.setCursor(12, 10);
            lcd.print("MAN");
            lcd.setCursor(0, 0);
            lcd.print("POMPA : ON");
            lcd.setCursor(1, 1);
            lcd.print("TIMP PROGRAMAT");
          }
          if (elapsedTime >= 60000)
          {
            
            isOn = false;
            isProgrammedByTime = false;
            askedProgramTime = -1;
            startTheProgrammedPump = -1;
          }
          String overrideMessage = client->readStringUntil('\n');
          Serial.println(overrideMessage);
          if (!overrideMessage.isEmpty())
          {
            if (overrideMessage == "CANCEL")
            {
              isOn = false;
              isProgrammedByTime = false;
              askedProgramTime = -1;
              startTheProgrammedPump = -1;
            }
          }
        }
        // de trimis la client ca s-a terminat pompa 
        digitalWrite(PUMP_CONTROL_PIN, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TERMINAT!");
        lcd.setCursor(0, 10);
        lcd.print("MAN");
      }
    }
    break;

  default:
    break;
  }
}

/*END FUNCTIONALITY*/

/*LOOP*/
void loop()
{
  WiFiClient client = server.accept();
  if (client)
  {
    if (client.connected())
    {
      PrintOnLcd("Connectat");
      client.print("CON");
      client.print("\n");
    }
  }
  while (client.connected())
  {

    timeClient.update();
    client.flush();
    Serial.println(timeClient.getFormattedTime());
    String messageReceived = client.readStringUntil('\n');
    if (!messageReceived.isEmpty())
    {
      // m_SELECT , a_SELECT
      int underscoreIndex = messageReceived.indexOf('_');
      if (messageReceived.startsWith("SELECT", underscoreIndex + 1))
      {
        noInterrupts(); // disable intrerrupts
        char mode = messageReceived.charAt(0);
        SetMode(mode);
        interrupts(); // enable
      }
      // value_THRESHOLD
      Serial.println(modeChanged);
      if (messageReceived.startsWith("THRESHOLD", underscoreIndex + 1) && static_cast<int>(modeChanged) == 1)
      {
        noInterrupts();
        Serial.println(messageReceived.substring(0, 2).toInt());
        moistureThreshold = messageReceived.substring(0, 2).toInt();
        Serial.println(moistureThreshold);
        interrupts(); // enable
      }
    }

    Functionality(&client, messageReceived);

    if (!client.connected())
    {
      client.stop();
      PrintOnLcd("Deconectat!");
    }
  }
}
