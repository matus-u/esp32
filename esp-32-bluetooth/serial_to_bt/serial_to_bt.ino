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
WiFiServer *wifiServer;
int port = 80;

long lastDiscoverableTime = -1;

bool wifiModeEnabled = false;

void serialPrint(String data) {
    Serial.println(data);
}

const int DISCOVERABLE_LOGIC_PIN = 5;
const int LED_PIN = 4;

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

bool parseSSID(String& msg, String& ssid)
{
    if (msg.length() > 21) {
        String ssidStr = msg.substring(0,20);
        ssid = remove0x80(ssidStr);
        msg.remove(0,21);
        if (ssid != "") {
            return true;
        }
    }
    return false;
}

String remove0x80(String& msg) {
    int index = msg.indexOf(char(0x80));
    if (index >= 0) {
        return msg.substring(0,index);
    }
    return msg;
}

String removeFirstPart(String& msg) {
    String first = "";
    int index = msg.indexOf(',');
    if (index >= 0) {
        first = msg.substring(0,index);
        msg.remove(0,index+1);
    }
    return first;
}

void remove_zeroes(String& msg) {
    if (msg[0] == '0' && msg[1] == '0') {
        msg.remove(0,2);
    } else {
        if (msg[0] == '0') {
            msg.remove(0,1);
        }
    }
}

bool parseIp(String& msg, IPAddress &addr)
{
    String first, second, third, fourth = "";
    first = removeFirstPart(msg);
    second = removeFirstPart(msg);
    third = removeFirstPart(msg);
    fourth = removeFirstPart(msg);

    remove_zeroes(first);
    remove_zeroes(second);
    remove_zeroes(third);
    remove_zeroes(fourth);

    if (first == "" || 
        second == "" || 
        third == "" ||
        fourth == "")
        return false;

    return addr.fromString(first+ "." + second + "." + third + "."+fourth);
}

bool parsePort(String& msg)
{
    String portStr = removeFirstPart(msg);
    port = portStr.toInt();
    return port != 0;
}

bool parsePassword(String& msg, String &pass)
{
    if (msg.length() > 20) {
        String passStr = msg.substring(0,20);
        pass = remove0x80(passStr);
        return true;
    }
    return false;
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

void initializeWifi() {

    String ssid = "";
    String password = "";

    IPAddress localIp;
    IPAddress subnet;
    IPAddress gateway;
    subnet.fromString("255.255.255.0");
    gateway.fromString("192.168.5.1");

    WiFi.disconnect(true, true);
    while (true) {
        delay(20);

        serialPrint("@S?");
        String response = Serial.readStringUntil('\n');

        if (!parseIp(response, localIp))
            continue;

        if (!parsePort(response))
            continue;

        if (!parseSSID(response, ssid))
            continue;

        if (!parsePassword(response, password))
            continue;

        if (WiFi.config(localIp, gateway, subnet) && wifiConnect(ssid, password)) {
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

    if (client && client->connected()) {
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
        wifiServer = new WiFiServer(port);
        wifiServer->begin();
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
        WiFiClient client = wifiServer->available();

        if (client) {

            while (client.connected()) {

                if (Serial.available()) {
                    cmdProtocolFuncWifi(&client);
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
