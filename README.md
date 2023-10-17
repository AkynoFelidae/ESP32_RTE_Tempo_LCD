# EPS32_RTE_Tempo_LCD
Get cost color code (tempo) from EDF/RTE and display on the screen

Uses arduino toolchain and libraries

# Hardware
ESP32 2424S102

![ESP32 2424S102](https://github.com/dbocktaels/ESP32_RTE_Tempo_LCD/blob/main/ressource/ESP32%202424S102.png)

# Required arduino libraries
- https://github.com/arduino-libraries/NTPClient
- https://github.com/lovyan03/LovyanGFX

# Configuration
require uspdae for your Wifi network
```cpp
const char* ssid = "xxxxx";
const char* password = "xxxxx";
```

require you indentification code from RTE service
```cpp
const char * idRTE = "Basic xxxxxxx==";
```
replace xxxxxxx== with your 64 base code
