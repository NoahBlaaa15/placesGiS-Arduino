# PlacesGiS Arduino Part
This is the Code for the ESP8266 for my placesGiS Project. 
Here is my main explanation: https://github.com/NoahBlaaa15/placesGiS
It makes use of promiscous mode to display presence information, you can find my code to it here: https://github.com/NoahBlaaa15/esp8266-promiscous-example

## Sending to Server
To send the collected data to a server I use http get requests.
```C
WiFiClient client;
HTTPClient http;
http.begin(client, addr);
http.GET();
http.end();
```

## Using Wifi as a presence/people sensor
I collect a list of MAC addresses and make sure i dont have doubles. I only allow to save 200 Mac at the time for memory purposes. 
After a certain time i send the length of the list to my Backend: https://github.com/NoahBlaaa15/placesGiS-Backend
```C
#define MAX_CLIENTS_TRACKED 200
int mac_known_count = 0;
static char macAddr[MAX_CLIENTS_TRACKED][18];
...
memcpy(macAddr[mac_known_count], macString, sizeof(macString));
mac_known_count = mac_known_count + 1;
```

## DHT22 and Light sensor
I also send the temperatur, humidity and light level.
For that i use the DHTEsp Library: https://github.com/beegee-tokyo/DHTesp
```C
dht.setup(4, DHTesp::DHT22);
float humidity = dht.getHumidity();
float temperature = dht.getTemperature();
float light = analogRead(A0);
```
