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

#### - In functie de umiditatea solului care este masurata de senzorul de umiditate , pompa de apa va fi actionata prin intermediul WI-FI-ului (sau automat cand senzorul detecteaza ), trimitandu-i o comanda care sa porneasca sau sa opreasca apa. Un rezervor de apa va fi atasat langa recipientul de pamant , de unde furtunul va trage apa spre pamant. Un ### ecran OLED / LCD va afisa starea curenta de functionare.

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


