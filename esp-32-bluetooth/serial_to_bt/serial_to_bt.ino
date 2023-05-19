//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include "MyBluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include <WiFi.h>

BluetoothSerial SerialBT;
WiFiServer wifiServer(80);

long lastDiscoverableTime = -1;

bool wifiModeEnabled = false;

void serialPrint(String data) {
    Serial.println(data);
}

const int DISCOVERABLE_LOGIC_PIN = 5;
const int LED_PIN = 2;

void setBtUndis()
{
    if (digitalRead(DISCOVERABLE_LOGIC_PIN) == LOW)
    {
        SerialBT.setUndiscoverable();
        digitalWrite(LED_PIN, HIGH);
    }
}
void setBtDis()
{
    lastDiscoverableTime = millis();
    if (digitalRead(DISCOVERABLE_LOGIC_PIN) == LOW) {
        SerialBT.setDiscoverable();
        digitalWrite(LED_PIN, LOW);
    }
}

String parseSSID(const String& msg)
{
    int indexOf = msg.indexOf("|");
    if (indexOf >= 6)
        return msg.substring(6,indexOf);
    return "";
}

String parsePassword(const String& msg)
{
    int indexOf = msg.indexOf("|");
    int lastIndex = msg.lastIndexOf("|");
    if ((indexOf >= 6) && (indexOf < lastIndex)) {
        return msg.substring(indexOf+1, lastIndex);
    }
    return "";
}

bool wifiConnect(const String& ssid, const String password) {

    WiFi.begin(ssid.c_str(), password.c_str());
    int i =0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        i++;
        if (i == 40) {
            return false;
        }
    } 
    return true;
}

bool parseIpSettings(const String& msg, IPAddress &addr)
{
    if (msg.length() <= 4)
        return false;

    int index = code.indexOf(",");
    if (index == -1)
        return false;
    
    String addr = code.substring(4,index);
    return addr.fromString(addr);
}

void initializeWifi() {

    String ssid = "";
    String password = "";

    IPAddress localIp;
    IPAddress subnet;
    IPAddress gateway;

    WiFi.disconnect(true, true);
    while (true) {
        delay(20);

        serialPrint("@IP?");
        if (!parseIpSettings(Serial.readStringUntil('\n'), localIp))
            continue;

        serialPrint("@SB?");
        if (!parseIpSettings(Serial.readStringUntil('\n'), subnet))
            continue;

        serialPrint("@GW?");
        if (!parseIpSettings(Serial.readStringUntil('\n'), gateway))
            continue;

        serialPrint("@WIFI?");
        code = Serial.readStringUntil('\n');

        if ((code.length() > 6) && (code.startsWith("@WIFI="))) {
            String ssid = parseSSID(rest);
            String password = parsePassword(rest);
            if ((ssid != "") && WiFi.config(localIp, gateway, subnet) && wifiConnect(ssid, password)
                return;
        }
    }

}

void initializeBluetooth() {
    String pin = "";
    String blName = "";
    while (pin == "" || blName == "") {
        delay(20);
        serialPrint("@W?");
        String code = Serial.readStringUntil('\n');

        if ((code.length() > 5) && (code.startsWith("@W="))) {
            int firstIndex = code.indexOf(",");
            int lastIndex = code.lastIndexOf(",");
            if ((firstIndex > 3) && (firstIndex < lastIndex)) {
                blName = code.substring(3,firstIndex);
                pin = code.substring(firstIndex+1, lastIndex);
            }

        }
    }

    SerialBT.begin(blName.c_str()); //Bluetooth device name
    SerialBT.setPin(pin.c_str());
}

void cmdProtocolFunc() {
    String code = Serial.readStringUntil('\n');
    if (code.startsWith("@Q0")) {
        setBtDis();
    } else if (code.startsWith("@Q1")) {
        ESP.restart();
    } else {
        SerialBT.write(reinterpret_cast<const unsigned char *>(code.c_str()), code.length());
        SerialBT.write('\n');
    }
}

void cmdProtocolFuncWifi(WiFiClient *client) {
    String code = Serial.readStringUntil('\n');
    if (code.startsWith("@Q1")) {
        ESP.restart();
    } 

    if (client && client->connected())
        client->write(reinterpret_cast<const unsigned char *>(code.c_str()), code.length());
        client->write('\n');
    }
}

void setup() {

    Serial.begin(9600);

    pinMode(LED_PIN, OUTPUT);

    if (!wifiModeEnabled)
    {
        pinMode(DISCOVERABLE_LOGIC_PIN, INPUT);

        digitalWrite(LED_PIN, LOW);
        initializeBluetooth();

        setBtUndis();
    }
    else {
        initializeWifi();
        wifiServer.begin();
    }

}

long count = 1;

void loop() {

    if (!wifiModeEnabled)
    {
        count++;

        if (((lastDiscoverableTime > 0) && (lastDiscoverableTime + 180000 < millis())) || (lastDiscoverableTime < 0 && ((count % 10000) == 0))) {
            lastDiscoverableTime = -1;
            setBtUndis();
            count = 1;
        }

        if (Serial.available()) {
            cmdProtocolFunc();
        }
        if (SerialBT.available()) {
            Serial.write(SerialBT.read());
        }
        delay(2);
    }
    else {
        WiFiClient client = wifiServer.available();

        if (client) {

            while (client.connected()) {

                if (Serial.available()) {
                    cmdProtocolFuncWifi(client);
                }

                if (client.available()>0) {
                    Serial.write(client.read());
                }

                delay(2);
            }

            client.stop();
            Serial.println("Client disconnected");

        } else {
            if (Serial.available()) {
                cmdProtocolFuncWifi(NULL);
            }
        }
        delay(2);
    }
}
