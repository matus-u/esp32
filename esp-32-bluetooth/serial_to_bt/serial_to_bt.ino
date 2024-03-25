//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include <Preferences.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include <WiFi.h>

WiFiServer *wifiServer;
int port = 8000;

Preferences preferences;

void serialPrint(String data) {
    Serial.println(data);
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

//    serialPrint("Parse Ip after: ");
//    serialPrint(first);
//    serialPrint(second);
//    serialPrint(third);
//    serialPrint(fourth);

    remove_zeroes(first);
    remove_zeroes(second);
    remove_zeroes(third);
    remove_zeroes(fourth);
    
//    serialPrint("Parse Ip after zeroes: ");
//    serialPrint(first);
//    serialPrint(second);
//    serialPrint(third);
//    serialPrint(fourth);

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
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    bool first = true;
    delay(3000);

    preferences.begin("wifi-app", false);
    while (true) {

        if (!first) {
            delay(20000);
        }

        first = false;

        serialPrint("@S?");
        bool parseResponse = true;
        String response = Serial.readStringUntil('\n');
        if (!response.startsWith("@S=")) {
            parseResponse = false;
//          serialPrint("unexpected response:");
//          serialPrint(response);
        }


        if (parseResponse) {
            response.remove(0,3);
        }

        if (parseResponse && !parseIp(response, localIp)) {
//            serialPrint("CANNOT PARSE IP");
            parseResponse = false;
        }

        if (parseResponse && !parseIp(response, gateway)) {
//            serialPrint("CANNOT PARSE GATEWAY");
            parseResponse = false;
        }

        if (parseResponse && !parseIp(response, subnet)) {
//            serialPrint("CANNOT PARSE SUBNET");
            parseResponse = false;
        }

        if (parseResponse && !parsePort(response)){
//            serialPrint("CANNOT PARSE PORT");
            parseResponse = false;
        }

        if (parseResponse && !parseSSID(response, ssid)) {
//            serialPrint("CANNOT PARSE SSID");
            parseResponse = false;
        }

        if (parseResponse && !parsePassword(response, password)){
//            serialPrint("CANNOT PARSE PASSWORD");
            parseResponse = false;
        }

        if (!parseResponse) {
            if (!localIp.fromString(preferences.getString("localip", ""))){
                continue;
            }

            if (!subnet.fromString(preferences.getString("subnet", ""))){
                continue;
            }

            if (!gateway.fromString(preferences.getString("gateway", ""))){
                continue;
            }

            ssid = preferences.getString("ssid", "");
            password = preferences.getString("password", "");
            port = preferences.getInt("port", 0);

            if ((ssid == "") || (port == 0)) {
                continue;
            }

        } else {
            serialPrint("@OK");
            preferences.putString("localip", localIp.toString());
            preferences.putString("subnet", subnet.toString());
            preferences.putString("gateway", gateway.toString());
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.putInt("port", port);
            preferences.end();
            serialPrint("Settings correctly saved -> restart in 30s...");
            delay(30000);
            ESP.restart();
        }
        
        if (!WiFi.config(localIp, gateway, subnet)){
//           serialPrint("CANNOT SETUP WIFI");
//           serialPrint(localIp.toString());
//           serialPrint(subnet.toString());
//           serialPrint(gateway.toString());
           continue;

        }
        if (wifiConnect(ssid, password)) {
            preferences.end();
            return;
        }
//        serialPrint("CANNOT CONNECT TO WIFI");
//        serialPrint(ssid);
//        serialPrint(password);
//        serialPrint(String(WiFi.status()));
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

    initializeWifi();
    wifiServer = new WiFiServer(port);
    wifiServer->begin();

}

long count = 1;

void loop() {

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
