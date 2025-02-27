//This examtple code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include <Preferences.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include <WiFi.h>
#include <WiFiProvisioner.h>

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

    //initializeWifi();

    bool hasCorrectConfig = false;
    preferences.begin("wifi-provision", true);
    bool apMode = preferences.getBool("ap-mode", false);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    bool dhcpEnabled = preferences.getBool("dhcp-enabled", true);

    IPAddress localIp;
    IPAddress subnet;
    IPAddress gateway;

    if (!dhcpEnabled) {
      if (!localIp.fromString(preferences.getString("localip", ""))){
        hasCorrectConfig = false;
      }

      if (!subnet.fromString(preferences.getString("subnet", ""))){
        hasCorrectConfig = false;
      }

      if (!gateway.fromString(preferences.getString("gateway", ""))){
        hasCorrectConfig = false;
      }
      if (ssid.isEmpty()) {
        hasCorrectConfig = false;
      }
    }
    preferences.end();

    // todo some decission making
    if (!hasCorrectConfig  || 2 == 2) {

      WiFiProvisioner provisioner;

      provisioner.onSuccess(
          [](const char *ssid, const char *password, const char *input) {
          Serial.printf("Provisioning successful! Connected to SSID: %s\n", ssid);
          if (password) {
          Serial.printf("Password: %s\n", password);
          }
          //todo reset
          });

      provisioner.startProvisioning();
      ESP.restart();
    }


    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);

    if (!dhcpEnabled) {
        if (!WiFi.config(localIp, gateway, subnet)){
          delay(5000);
          ESP.restart();
        }
    }

    if (wifiConnect(ssid, password)) {
        delay(5000);
        ESP.restart();
    }

    wifiServer = new WiFiServer(port);
    wifiServer->begin();

}

long count = 1;

void loop() {

    if (WiFi.status() != WL_CONNECTED) {
        delay(5000);
        ESP.restart();
    }

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
    delay(10);
}
