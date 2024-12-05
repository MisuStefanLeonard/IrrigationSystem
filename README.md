# IrrigationSystem
Irrigation system made using arduino uno and ESP 8266

# Descriere

Un sistem de irigatie programabil , care detecteaza umiditatea solului si porneste o pompa de apa pentru a-l uda . Acesta poate gestiona cantitatea apei , timpul cat sa curga si programabil la diferite ore. 


# Componente folosite

#### 1 x Arduino UNO 
#### 1 x ESP 8266
#### 1 x Pompa Apa
#### 1 x Rezervor apa
#### 1 x Senzor umiditate LM393 
#### 1 x LCD/OLED display
#### ? x Fire Legatura
#### ? x Rezitente 
#### 1 x Relay pompa


# Flow

#### - In functie de umiditatea solului care este masurata de senzorul de umiditate , pompa de apa va fi actionata prin intermediul WI-FI-ului (sau automat cand senzorul detecteaza  ), trimitandu-i o comanda care sa porneasca sau sa opreasca apa. Un rezervor de apa va fi atasat langa recipientul de pamant , de unde furtunul va trage apa spre pamant. Un ### ecran OLED / LCD  va afisa starea curenta de functionare , iar pana cand o comanda nu este terminata , alta comanda nu poate fi data catre al doilea micro-controller.
