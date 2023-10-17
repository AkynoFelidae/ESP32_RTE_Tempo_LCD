# EPS32_RTE_Tempo_LCD
Get cost color code (tempo) from EDF/RTE and display on the screen

Uses arduino toolchain and libraries
- Use WiFi in STA mode to connect to home network
- Synchronise NTP server to get current date and time
- Connect to RTE-France to the Tempo calendar API
- Display the color for the day and the next day

![color panels](https://github.com/dbocktaels/ESP32_RTE_Tempo_LCD/blob/main/ressource/color_panels.png)

# Hardware
ESP32 2424S102

![ESP32 2424S102](https://github.com/dbocktaels/ESP32_RTE_Tempo_LCD/blob/main/ressource/ESP32%202424S102.png)

# Required arduino libraries
- NTPClient from Fabrice Weinberg version=3.2.1 https://github.com/arduino-libraries/NTPClient
- LovyanGFX from lovyan03 version=1.1.9 https://github.com/lovyan03/LovyanGFX

# Configuration
require update for your Wifi network
```cpp
const char* ssid = "xxxxx";
const char* password = "xxxxx";
```

require you indentification code from RTE service
replace xxxxxxx== with your 64 base code
```cpp
const char * idRTE = "Basic xxxxxxx==";
```

## get your id
[Catalogue API - API Data RTE (rte-france.com)](https://data.rte-france.com/)
![tempo](https://github.com/dbocktaels/ESP32_RTE_Tempo_LCD/blob/main/ressource/RTE%20data%20tempo.png)

register for the service to get your id

![RTE_base64](https://github.com/dbocktaels/ESP32_RTE_Tempo_LCD/blob/main/ressource/dashboard_RTE_id.png)

example of data
```json
{
	"tempo_like_calendars": {
		"start_date": "2023-02-28T00:00:00+01:00",
		"end_date": "2023-03-03T00:00:00+01:00",
		"values": [
			{
				"start_date": "2023-03-02T00:00:00+01:00",
				"end_date": "2023-03-03T00:00:00+01:00",
				"value": "RED",
				"updated_date": "2023-03-01T10:20:00+01:00"
			},
			{
				"start_date": "2023-03-01T00:00:00+01:00",
				"end_date": "2023-03-02T00:00:00+01:00",
				"value": "RED",
				"updated_date": "2023-02-28T10:20:00+01:00"
			},
			{
				"start_date": "2023-02-28T00:00:00+01:00",
				"end_date": "2023-03-01T00:00:00+01:00",
				"value": "WHITE",
				"updated_date": "2023-02-27T10:20:00+01:00"
			}
		]
	}
}
```
```json
{"tempo_like_calendars":[{"start_date":"2016-03-12T00:00:00+01:00","end_date":"2016-03-13T00:00:00+01:00","values":{"start_date":"2016-03-12T00:00:00+01:00","end_date":"2016-03-13T00:00:00+01:00","value":"BLUE","updated_date":"2016-03-11T10:20:00+01:00"}}]}
```
