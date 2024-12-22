# IrrigationSystem

Irrigation system made using arduino uno and ESP 8266

# Ideea

Ideea a pornit de la faptul ca doream sa fac un proiect avand si o interfata de unde sa poata sa fie operat
asa ca am ales un sistem de irigare care poate fi controlat prin Wi-Fi . Mi s-a parut interesant cum lucrurile sunt
controlate prin Wi-Fi si am vrut si eu sa incerc sa fac ceva asemanator.

# Descriere

Un sistem de irigatie programabil , care detecteaza umiditatea solului si porneste o pompa de apa pentru a-l uda . Acesta poate gestiona cantitatea apei , timpul cat sa curga si programabil la diferite ore.

# Componente folosite

#### 1 x ESP 8266

#### 1 x Pompa Apa - 80/120 l/h

#### 1 x Rezervor apa

#### 1 x Senzor umiditate LM393

#### 1 x LCD/OLED display

#### Fire legatura : Cate sunt nevoie

####  Rezistente -> 2x220 Ohm , 2x 1kOhm 

#### 1 x Tranzistor BC547 (pentru activarea/dezactivarea pompei)

#### 1 x Modul coborator tensiune XL4015 , 5-36VDC , 5A , 75W

#### 1 x Dioda 1N4001 (pentru voltage spike-urile de la dezactivarea/activarea pompei de apa prin tranzistor)

#### 2 x DALBI 18650 3.7V Li-Ion (amplasate in serie) , 5000mAh

#### 1 x Support Acumulatori

# Flow

#### - Se vor putea selecta doua moduri de functionare : AUTO si MANUAL

##### Modul AUTO

###### - Inseamna ca atunci cand senzorul detectaza ca umiditatea pamantul cade sub un prag , acesta va porni automat

##### Modul MANUAL

###### - Inseamna ca utilizatorul v-a putea sa ii dea drumul cand doreste

###### - Va putea sa fie programat in functie de ora

###### - Cantitatea de apa sa fie aleasa (cat sa curga)

#### - In functie de umiditatea solului care este masurata de senzorul de umiditate , pompa de apa va fi actionata prin intermediul WI-FI-ului (sau automat cand senzorul detecteaza ), trimitandu-i o comanda care sa porneasca sau sa opreasca apa. 
#### - Un rezervor de apa va fi atasat langa recipientul de pamant , de unde furtunul va trage apa spre pamant. Un ### ecran OLED / LCD va afisa starea curenta de functionare.

# Probleme intampinate

- Nu stiam cum pot sa controlez pompa de apa , asa ca am folosit un tranzistor sa o fac . Am folosit un BC547 care este indeajuns pentru acest lucru , iar acum totul este functional.
- Pot aparea voltage spike-uri atunci cand activ/dezactiv pompa incontinuu , asa ca am folosit o dioda polarizata invers pentru a scapa de aceste spike-uri ( legata in paralel (catodul(-) la borna pozitiva a pompei si anodul(+) la borna negativa a bateriei)).
- Senzorul de umiditate LM393 este legat la un pin analog . Am luat cateva calcule : SOL UMED - 1023 (valoare citita analog) , SOL USCAT - in jur de 500-530 (valoare citita analog) . Am folosit urmatoare formula pentru a calcula cat la suta de umed este solul :

# Calculul procental al umiditatii solului 
#### - SOL_USCAT - valoare citita atunci cand solul este uscat
#### - SOL_UMED - valoare citita atunci cand solul este umed (abundent in apa)

##### Moisture ( % ) = (SOL USCAT - VALOARE_CITITA) / (SOL USCAT - SOL UMED)

# Legare pini

#### Vin -> + placa arduino
#### GND -> - placa arduino
#### D5 -> 1Kohm rezistor -> B (Baza tranzistor)
#### D6 -> picioar dreapta buton (1k ohm rezistor legat la + placa)
#### Picior stanga buton -> GND (- placa breadboard)
#### GND LCD -> GND placa
#### Vcc LCD -> + placa
#### SDA LCD -> D2 ESP8266
#### SCL LCD -> D1 ESP8266
#### D3 ESP8266 -> LED (rezistenta 220 Ohm)
#### D4 ESP8266 -> LED (rezistenta 220 Ohm)
#### Colector tranzistor -> GND Pompa apa
#### Emitor tranzistor -> GND Placa breadboard
#### + Pompa apa -> + Placa breadboard
#### Cathode (-) dioda -> + Pompa apa
#### Anode (+) dioda -> - Pompa apa
#### Senzor umiditate + -> + Placa breadboard
#### Senzor umiditate - -> - Placa breadboard
#### Analog PIN senzor umiditate -> A0
#### Modul coborator tensiune + (IN) -> + Suport acumulatori (2 bucati de acumulatori legati in serie)
#### Modul coborator tensiune - (IN) -> - Suport acumulatori (2 bucati de acumulatori legati in serie)
#### Modul coborator tensiune + (OUT) -> Vin ESP8266
#### Modul coborator tensiune - (OUT) -> GND ESP8266


# Code snips

#### Definirea constantelor / librariilor de care avem nevoie
```cpp
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
```


#### Conectarea la WI-FI cu placa
```cpp
const char *ssid = " /* your ssid here */";
const char *password = "/* your ssid  password here */" ;
```

#### Variabile globale folosite de controller

```cpp
volatile bool modeChanged = true; // false for "AUTO", true for "MANUAL"
int moistureThreshold = 0; // pragul umezitatii minime
volatile bool pompState = false; // starea pompei
long long askedProgramTime = -1; // timpul cerut de utilizator la programarea pompei
long long startTheProgrammedPump = -1; // timpul cand se da startul la pompa 
volatile bool isProgrammedByTime = false; // daca pompa a fost setata sa fie programata
volatile bool cancelProgrammedTimed = false; // daca se anuleaza starea de programare
volatile bool forceStop = false; // daca se forteaza resetarea din buton
volatile bool isOn = false; // daca pompa este pornita
unsigned long lastSendTime = 0; // ultima oara cand trimite datele prin WI-FI (interval de 5 secunde (vezi DEFINE))
// Variabila aceasta este doar pentru a semnala FRONT-END-ul ca modul de operare a fost schimbat
volatile bool resetTime = false; // Daca s-a schimbat modul de operare din AUTO - MANUAL / MANUAL - AUTO 
// verifica daca pompa este pe modul auto
volatile bool isPumpOnAuto = false;
// diferenta dintre datele primite
int dataId = 0;
```

#### Functia pe care intra in intrerupere cand se apasa butonul pe placa breadboard
```cpp
void IRAM_ATTR PumpControlManualISR()
{
  pompState = false;
  forceStop = true;
  modeChanged = true; // defaults to AUTO;
}
```


#### Setup

```cpp
Serial.begin(9600); // Setare clock-rate
pinMode(OK_LED, OUTPUT); // de adaugat functionalitate
pinMode(NOT_OK_LED, OUTPUT); // de adaugat functionalitate
pinMode(PUMP_CONTROL_PIN, OUTPUT); // pin-ul de contro al pompei
pinMode(FORCE_STOP_BUTTON, INPUT_PULLUP); // butonul de oprire fortata
digitalWrite(PUMP_CONTROL_PIN, LOW); // initial setam pompa pe OFF
digitalWrite(OK_LED, HIGH); // test
digitalWrite(NOT_OK_LED, HIGH); // test
/* Conectarea la WI-FI */
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
/* Conectarea la WI-FI */

/* Initializare LCD */
lcd.init();
lcd.backlight();
/* Initializare LCD */

/* Initializare server NTP */
timeClient.begin(); // setam ca controller-ul sa isi traga timpul local 
timeClient.setTimeOffset(2*3600); // UTC + 2 pentru Romania
/* Initializare server NTP */

attachInterrupt(digitalPinToInterrupt(FORCE_STOP_BUTTON), PumpControlManualISR, FALLING);
```


#### Functia pentru schimbarea modului de lucru

```cpp
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
```


#### Functii ajutatoare
##### - Calcularea umiditatii in procente
##### - Convertirea in secunde a timpului din format HH:MM:SS

#### ATENTIE! Masurati valorile de margine ale senzorului (SOL_USCAT - sol uscat || SOL_UMED - sol umed) pentru calcul exact!

```cpp
float CalculateHumidityPercent()
{
  int readAnalogHumidityValue = analogRead(MOISTURE_SENSOR);
  Serial.println(readAnalogHumidityValue);
  
  float percent = 100.0 * ((SOL_USCAT - readAnalogHumidityValue) / (float)(SOL_USCAT - SOL_UMED));
  percent = constrain(percent, 0, 100); 
  return percent;
}
long long ConvertTimeToSeconds(int hours, int minutes, int seconds)
{
  return (hours * 3600) + (minutes * 60) + seconds;
}
```

#### Functia de oprire fortata

```cpp
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
```


#### Functionalitatea principala

```cpp
void Functionality(WiFiClient *client, String message)
{
  // verificam de fiecare daca butonul a fost apasat 
  ForceStop(client);
  float humidity = CalculateHumidityPercent();
  unsigned long currentTimePassed = millis();
  String autoPumpMessage = "";
  switch (static_cast<int>(modeChanged))
  {
  // mode AUTO
  case 1:
  // modul AUTO
  /* Printam informatii pe ecranul LCD */
    PrintOnLcd("POMPA:");
    lcd.print(pompState == true ? "ON" : "OFF");
    lcd.setCursor(12, 0);
    lcd.print("AUTO");
    lcd.setCursor(0, 1);
    lcd.print("HUMI:");
    lcd.print(humidity);
    lcd.print("%");
  /* Printam informatii pe ecranul LCD */
  /* Verificam daca modul a fost schimbat  */
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
    /* Verificam daca modul a fost schimbat  */
    /* Daca nu , transimisie normala de date  */
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
    /* Daca nu , transimisie normala de date  */
    /* Verificam daca pragul setat de noi este mai mare decat umiditatea curenta in sol . Daca da , dam drumul la pompa pana cand ajunge la valoare dorita!  */
    if (moistureThreshold > humidity)
    {
      pompState = true;
      isPumpOnAuto = true;
    }
    /*Functia principala*/
    while (isPumpOnAuto)
    {
      
      int readingPumpState = digitalRead(PUMP_CONTROL_PIN);
      if (readingPumpState == LOW)
      {
        digitalWrite(PUMP_CONTROL_PIN, HIGH);
        pompState = true;
      }
      // La fiecare pas , calculam umiditatea curenta si o comparam
      humidity = CalculateHumidityPercent();
      Serial.println("Recalculata: ");
      Serial.print(humidity);
      // Compararea aici
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
      // Trimitem mesaje la FRONT-END 
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
      // !! FOARTE IMPORTANT , after intervine WATCHDOG-ul si opreste programul
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
    /* START POMPA */
    if (message == "START")
    {
      if (!pompState)
      {
        digitalWrite(PUMP_CONTROL_PIN, HIGH);
        pompState = true; // POMP ON
      }
     
    }
    /* OPRIRE POMPA */

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
    /* Programarea la ora a pompei */
    else if (message.substring(0, 7) == "PROGRAM")
    {
      int underscoreIndex = message.indexOf('_');
      String time = message.substring(underscoreIndex + 1);

      // Extract HH, MM, and SS
      // Extragem orele , minutele si secundele
      int firstColonIndex = time.indexOf(':');
      int secondColonIndex = time.indexOf(':', firstColonIndex + 1);
      int HH = time.substring(0, firstColonIndex).toInt();                    // Extract HH
      int MM = time.substring(firstColonIndex + 1, secondColonIndex).toInt(); // Extract MM
      int SS = time.substring(secondColonIndex + 1).toInt();                  // Extract SS

      // convertim la secunde
      askedProgramTime = ConvertTimeToSeconds(HH, MM, SS); // this will be eh 1:30:20 converted to seconds

      // updatam ora ESP-ului
      timeClient.update();
      // luam timp-ul curent local UTC + 2
      String getFormatedTimeFromNTP = timeClient.getFormattedTime();
      int currentHourFromNTP = getFormatedTimeFromNTP.substring(0,2).toInt();
      int currentMinutesFromNTP = getFormatedTimeFromNTP.substring(3,5).toInt();
      int currentSecondsFromNTP = getFormatedTimeFromNTP.substring(6,8).toInt();
      // askedProgramTime is how much to add 
      // convertim la secunde
      long long currentTimeToSecondsFromNtp = ConvertTimeToSeconds(currentHourFromNTP, currentMinutesFromNTP, currentSecondsFromNTP);

      // TIMPUL LA CARE VA PORNI POMPA IN SECUNDE!
      startTheProgrammedPump = currentTimeToSecondsFromNtp + askedProgramTime;

      int hours = startTheProgrammedPump / 3600;              // Calculate hours
      int remainingSeconds = startTheProgrammedPump % 3600;   // Calculate remaining seconds after hours
      int minutes = remainingSeconds / 60;                   // Calculate minutes
      int seconds = remainingSeconds % 60;                  // Calculate remaining seconds
      // Variabila care decide daca a fost programat la ora
      isProgrammedByTime = true;
      // Trimitem date pe front-end si ecranul LCD
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
    // VERIFICAM DACA A FOST PROGRAMAT PE TIMP
    else if (isProgrammedByTime)
    {
    // SE POATE ANULA IN TIMPUL EXECUTIEI 
    // Verificam mesajul CANCEL trimis din front-end
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
      // UPDATAM TIMPUL
      timeClient.update();
      // 13:21:20
      String getFormatedTimeFromNTP = timeClient.getFormattedTime();
      // EXTRAGEM ORA , MINUTUL SI SECUNDA
      int currentHourFromNTP = getFormatedTimeFromNTP.substring(0,2).toInt();
      int currentMinutesFromNTP = getFormatedTimeFromNTP.substring(3,5).toInt();
      int currentSecondsFromNTP = getFormatedTimeFromNTP.substring(6,8).toInt();
      
      long long currentTimeToSecondsFromNtp = ConvertTimeToSeconds(currentHourFromNTP, currentMinutesFromNTP, currentSecondsFromNTP);
      // VERIFICAM DACA TIMPUL CURENT ESTE EGAL CU TIMPUL PROGRAMAT
      // DAM DRUMUL LA POMPA PENTRU 60 SECUNDE
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
          // AICI SE OPRESTE POMPA ( DUPA 60 SECUNDE)
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

```

#### Functia LOOP

```cpp
void loop()
{
  // Pornim serverul (pe portul 5055 ) si conexiunea spre FRONT-END
  WiFiClient client = server.accept();
  if (client)
  {
    // SEMNALAM CA S-A CONECTAT CLIENTUL
    if (client.connected())
    {
      PrintOnLcd("Connectat");
      client.print("CON");
      client.print("\n");
    }
  }
  while (client.connected())
  {
    // UPDATAM TIMMPUL LA INTERVAL DE 60 SECUNDE , (DEFAULT);
    timeClient.update();
    // curatam bufferul
    client.flush();
    // logs
    Serial.println(timeClient.getFormattedTime());
    // Mesajul primit
    String messageReceived = client.readStringUntil('\n');
    if (!messageReceived.isEmpty())
    {
      // m_SELECT , a_SELECT
      // SELECTIA DE MODURI (AUTO / MANUAL) , m_SELECT - MANUAL , a_SELECT - AUTO
      int underscoreIndex = messageReceived.indexOf('_');
      if (messageReceived.startsWith("SELECT", underscoreIndex + 1))
      {
        // dezactivam intreruperile pentru o logica fluenta
        noInterrupts(); // disable intrerrupts
        char mode = messageReceived.charAt(0);
        SetMode(mode);
        interrupts(); // enable
      }
      //  FORMAT : value_THRESHOLD
      // logs
      Serial.println(modeChanged);
      // Valoarea prag in formatul : 20_THRESHOLD
      if (messageReceived.startsWith("THRESHOLD", underscoreIndex + 1) && static_cast<int>(modeChanged) == 1)
      {
        // dezactivam intreruperile pentru o logica fluenta
        noInterrupts();
        Serial.println(messageReceived.substring(0, 2).toInt());
        moistureThreshold = messageReceived.substring(0, 2).toInt();
        Serial.println(moistureThreshold);
        interrupts(); // enable
      }
    }
    // Functia principala de rulare
    Functionality(&client, messageReceived);


    // Daca clientul s-a deconectat , semnalam.
    if (!client.connected())
    {
      client.stop();
      PrintOnLcd("Deconectat!");
    }
  }
}
```





