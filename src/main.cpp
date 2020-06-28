//Arduino and ESP SDK
#include <HardwareSerial.h>
#include <user_interface.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "Arduino.h"
#include "osapi.h"
//WIFI structures
#include "structures.h"
//DHT Library
#include "DHTesp.h"

//Creating List of macs and a maxium Clients tracked, because ESP has few ram
#define MAX_CLIENTS_TRACKED 200

int mac_known_count = 0;
static char macAddr[MAX_CLIENTS_TRACKED][18];

//Used when sending data to server
const char deviceID[12] = "Mensa";

//Current WIFI Channel and different timers
uint8_t channel = 1;
ulong timerC = 0;
ulong timerW = 0;
ulong timerR = 0;

//Define the DHT Sensor Interface to read data
DHTesp dht;

//Function to decrypt WIFI Packets
void promiscCb(uint8_t *buf, uint16_t len)
{
    if (len == 60){
        //Decrypt WIFI Packet
        struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;

        //Find Mac in Packet
        uint8_t mac[6];
        int i=0;
        for (i=0; i<6; i++) {
            mac[i] = sniffer->buf[i+6];
        }

        //Convert Mac to readable string (hex => String)
        static char macString[18];
        sprintf(macString, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        if(mac_known_count > 0 && mac_known_count < 200){
            for (int j = 0; j < mac_known_count; ++j) {
                if(strcmp(macAddr[j],macString) == 0){
                    //Cancel Function if mac already in List
                    return;
                }
            }
        }else{
            //Clear Mac list if ram full
            memset(macAddr, 0, sizeof(macAddr));
            mac_known_count = 0;
        }

        //Add mac to List if not already in List and increment list size
        memcpy(macAddr[mac_known_count], macString, sizeof(macString));
        mac_known_count = mac_known_count + 1;
    }
}

//Config for WIFI Connection to send Data to the Server
void user_set_station_config(void) {
    char ssid[32] = "NSA#15";

    char password[64] = "_Snicklink";

    struct station_config stationConf;

    stationConf.bssid_set = 0;

    os_memcpy(&stationConf.ssid, ssid, 32);

    os_memcpy(&stationConf.password, password, 64);

    wifi_station_set_config(&stationConf);
}

void setup(){
    //Open Serial Comm.
    Serial.begin(9600);
    //Open Connection to the DHT Sensor
    dht.setup(4, DHTesp::DHT22);
    //Wait for DHT to acknowledge the connection
    delay(dht.getMinimumSamplingPeriod());
    //Test Connection
    Serial.printf("SDK version:%s", system_get_sdk_version());

    //Disable WIFI
    wifi_station_set_auto_connect(0);
    wifi_station_set_reconnect_policy(true);
    wifi_set_opmode(STATION_MODE);
    user_set_station_config();
    wifi_station_disconnect();
    wifi_set_channel(channel);

    //Enable Promiscous sniffing mode, make sure it is disabled first
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(promiscCb);
    wifi_promiscuous_enable(1);

}

void loop(){
    delay(10);
    if(millis() > timerC + 20000) {
        //Change Channel every 20s
        timerC = millis();
        channel++;
        if (channel == 15) {
            channel = 1;
        }
        wifi_set_channel(channel);
    }
    if(millis() > timerW + 120000) {
        //Send data every 2min
        timerW = millis();
        wifi_promiscuous_enable(0);
        wifi_station_connect();

        while(wifi_station_get_connect_status() != STATION_GOT_IP){
            delay(100);
        }

        //Create HTTP and WIFI Interface
        WiFiClient client;
        HTTPClient http;
        //Get sensor Values
        float humidity = dht.getHumidity();
        float temperature = dht.getTemperature();
        float light = analogRead(A0);
        //Make space in Memory for String 128 Characters Long
        char addr[128];
        //Add Values as GET Parameters
        sprintf(addr, "http://placesgis.herokuapp.com/?device=%s&users=%d&hum=%f&temp=%f&light=%f", deviceID, mac_known_count, humidity, temperature, light);
        //Open Tcp Connection
        http.begin(client, addr);
        //Make the HTTP Get request
        http.GET();
        //Close TCP Connection
        http.end();

        wifi_station_disconnect();
        wifi_promiscuous_enable(1);
    }
    if(millis() > timerR + 300000) {
        //Clear Mac list all 5min
        timerR = millis();
        memset(macAddr, 0, sizeof(macAddr));
        mac_known_count = 0;
    }
}
